#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include "unistd.h"
#include "ncurses.h"



// simple gap-buffer based text editor

// Ctrl-x s to save
// Ctrl-x c to quit

// Ctrl-c will also quit but might mess up your terminal (^:

int window_height;
int window_width;
int buffer_window_height;



int buffer_size;
char *buffer;
int cur1;
int cur2;

int currow;
int curcol;

int scrolly;
int scrollx;

char* buffer_filename;


char* status = "";

WINDOW* buf_win;
WINDOW* stat_win;


void fatal(char *message) {
  fprintf(stderr,"fatal error: %s\n",message);
  exit(1);
}
#define STAT_BUF_SZ 32
char status_buf[STAT_BUF_SZ];

#define SET_STATUS(msg, ...)                            \
  snprintf(status_buf, STAT_BUF_SZ, msg, ##__VA_ARGS__);  \
  status = status_buf;                            

#define CLEAR_STATUS() status = ""

void init_file_buffer(const char* filename) {
  currow = 0;
  curcol = 0;
  scrollx = 0;
  scrolly = 0;

  // if file exists, load buffer data and set buffer size
  if(access(filename, F_OK) != -1) {
    // file exists
    FILE* fp = fopen(filename, "r");
    fseek(fp, 0L, SEEK_END);
    buffer_size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    
    buffer = calloc(buffer_size, 1);
    
    fread(buffer, 1, buffer_size, fp);
    
    fclose(fp);
    
    cur1 = cur2 = 0;
    currow = curcol = 0;
    
    SET_STATUS("loaded %s", buffer_filename);
    
  } else {
    buffer = calloc(buffer_size, 1);
    cur1 = 0;
    cur2 = buffer_size;
    currow = curcol = 0;
    
    SET_STATUS("opened new file %s", buffer_filename);
  }
}

void save_buffer() {
  // create or open (overwrite-mode)
  FILE *fp = fopen(buffer_filename, "w+");
  if(fp == NULL) {
    SET_STATUS("couldn't open file to save!");
  } else {
    // save chars before cursor
    fwrite(buffer, 1, cur1, fp);
    // save chars after cursor
    fwrite(buffer+cur2, 1, buffer_size-cur2, fp);
    fclose(fp);
    SET_STATUS("buffer saved");
  }
}

void expand_buffer() {
  int newsize = buffer_size * 1.5;
  char* new_buf = calloc(newsize, 1);
  int cursor_size = newsize - buffer_size;
  
  int new_gap = newsize - buffer_size;
  
  // copy chars before cursor
  for(int i = 0; i < cur1; i++) {
    new_buf[i] = buffer[i];
  }
  
  // copy chars after cursor, giving us a new gap to work with
  for(int i = cur2; i < buffer_size; i++) {
    new_buf[i+new_gap] = buffer[i];
  }
  
  cur1 = cur1;
  cur2 = cur2+new_gap;
  free(buffer);
  buffer = new_buf;
  buffer_size = newsize;
}


void get_pos_from_cursor() {
  int col = 0;
  int row = 0;
  for(int i = 0; i < cur1; i++) {
    if(buffer[i] == '\n') {
      row++;
      col = 0;
    } else {
      col++;
    }
  }
  curcol = col;
  currow = row;
}

int get_col_from_cursor() {
  int col = 0;
  for(int i = 0; i < cur1; i++) {
    char c = buffer[i];
    if(c == '\n') {
      col = 0;
    } else {
      col++;
    }
  }
  return col;
}

int get_row_from_cursor() {
  int row = 0;
  for(int i = 0; i < cur1; i++) {
    char c = buffer[i];
    if(c == '\n') {
      row++;
    }
  }
  return row;
}



/*
             Hello, W..............orld!
Press right          ^             ^
             Hello, Wo..............rld!
Press left            ^             ^
             Hello, W..............orld!
Press backspace      ^             ^
             Hello, ................rld!
Press 0             ^               ^
             Hello, 0...............rld!
                     ^               ^
*/


void update_scroll() {
  if ((currow - scrolly) < 0) {
    while ((currow - scrolly) < 0) {
      scrolly--;
    }
  } else if ((currow - scrolly) >= buffer_window_height) {
    while ((currow - scrolly) >= buffer_window_height) {
      scrolly++;
    }
  }
  
  
  if ((curcol - scrollx) < 0) {
    while ((curcol - scrollx) < 0) {
      scrollx--;
    }
  } else if ((curcol - scrollx) >= window_width) {
    while ((curcol - scrollx) >= window_width) {
      scrollx++;
    }
  }
}

void insert_char(char c) {
  
  if(cur1 == buffer_size || cur1 == cur2) {
    expand_buffer();
    SET_STATUS("expanded buffer");
  }
  
  buffer[cur1++] = c;
  
  // track rows/columns
  if(c == '\n') {
    curcol = 0;
    currow++;
    scrollx = 0;
  } else {
    curcol++;
  }
  update_scroll();
}



void delete_char() {
  if(cur1 == 0) {
    SET_STATUS("reached beginning of buffer");
  } else {
    cur1--;
    char c = buffer[cur1];
    
    // track rows/columns
    if(c == '\n') {
      get_pos_from_cursor();
      scrollx = 0;
    } else {
      curcol--;
    }
    
    update_scroll();
    
  }
}

void cursor_left() {
  if(cur1 == 0 || cur2 == 0) {
    SET_STATUS("reached beginning of buffer");
  } else {
    char c = buffer[cur1-1];
    buffer[--cur2] = buffer[--cur1];
    
    // track rows/columns
    if(c == '\n') {
      get_pos_from_cursor();
      scrollx = 0;
    } else {
      curcol--;
    }
    
    update_scroll();
  }
}

void cursor_right() {
  if(cur1 == buffer_size || cur2 == buffer_size) {
    SET_STATUS("reached end of buffer");
  } else {

    buffer[cur1++] = buffer[cur2++];
    int c = buffer[cur1-1];
    
    
    
    // track rows/columns
    if(c == '\n') {
      currow++;
      curcol = 0;
      scrollx = 0;
    } else {
      curcol++;
    }
    update_scroll();
  }
}

void cursor_up() {
  int destrow = currow-1;
  int destcol = curcol;
  int lastrow = currow;
  int lastcol = curcol;

  // seek row
  while(currow > destrow) {
    cursor_left();
    if(lastrow == currow && lastcol == curcol) {
      SET_STATUS("reached beginning of buffer");
      return;
    }
    lastrow = currow;
    lastcol = curcol;
  }
  
  // seek column
  while(curcol > destcol) {
    cursor_left();
    if(lastrow == currow && lastcol == curcol) {
      SET_STATUS("reached beginning of buffer");
      return;
    }
    
    // if we went to the next line above, scroll back and exit
    if(currow != destrow) {
      cursor_right();
      return;
    }
    lastrow = currow;
    lastcol = curcol;
  }
}


void cursor_down() {
  int destrow = currow+1;
  int destcol = curcol;
  int lastrow = currow;
  int lastcol = curcol;
  
  int initrow = currow;
  
  // seek row
  while(currow < destrow) {
    cursor_right();
    if(lastrow == currow && lastcol == curcol) {
      SET_STATUS("reached end of buffer");
      return;
    }
    lastrow = currow;
    lastcol = curcol;
  }
  
  // seek column
  while(curcol < destcol) {
    cursor_right();
    if(lastrow == currow && lastcol == curcol) {
      SET_STATUS("reached end of buffer");
      return;
    }
    
    // if we went to the next line below, scroll back and exit
    if(currow != destrow) {
      cursor_left();
      return;
    }
    lastrow = currow;
    lastcol = curcol;
  }
  
}



void draw_buffer() {
  int dcol = 0;
  int drow = 0;
  
  
  void draw_char(char c) {
    if(drow >= scrolly && (drow-scrolly < buffer_window_height) && (dcol-scrollx < window_width)) {
      if(drow == currow) {
        if(dcol >= scrollx) {
          waddch(buf_win, c);
        }
      } else {
        waddch(buf_win, c);
      }
    }
      
    
    
    switch(c) {
    case '\n':
      dcol = 0;
      drow++;
      if(drow-scrolly > buffer_window_height) {
        return;
      }
      break;
    default:
      dcol++;
      break;
    }
  }
  
  wmove(buf_win, 0, 0);
  
  // draw before gap
  for(int i = 0; i < cur1; i++) {
    draw_char(buffer[i]);
  }
  
  // draw after gap
  for(int i = cur2; i < buffer_size; i++) {
    draw_char(buffer[i]);
  }

  // draw until bottom of window
  while(drow < window_height) {
    waddch(buf_win, '\n');
    drow++;
  }
}

void draw_status() {
  wmove(stat_win, 0, 0);
  wprintw(stat_win, "%s: %i,%i --- %s", buffer_filename, currow, curcol, status);
  // draw until end of window
  for(int i = strlen(status); i < window_width; i++) {
    wprintw(stat_win, " ");
  }
}


void cleanup() {
  endwin();
}

int escape = 0;

void handle_escape(int c) {
  switch(c) {
  case 's':
    save_buffer();
    break;
  case 'c':
    exit(1);
    break;
  default:
    CLEAR_STATUS();
    break;
  }
}

void handle_key(int c) {
  switch(c) {
  
  case KEY_UP:
    cursor_up();
    break;
  
  case KEY_DOWN:
    cursor_down();
    break;
  
  case KEY_LEFT:
  case 68:
    cursor_left();
    break;
  
  case KEY_RIGHT:
  case 67:
    cursor_right();
    break;
    
  case KEY_BACKSPACE:
  case 127:
    delete_char();
    break;
    
  case 24:
    SET_STATUS("escaping next key");
    escape = 1;
    break;
    
    
  default:
    insert_char(c);
    break;
  }
}

int main(int argc, char** argv) {
  // 1kB default buffer size for empty files
  buffer_size = 1024;
  // open file or create file of N bytes
  
  if(argc == 2) {
    // check for file
    buffer_filename = argv[1];
    init_file_buffer(buffer_filename);
  } else {
    printf("Usage\n");
    printf("edit filename\n");
    exit(1);
  }

  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);

  atexit(cleanup);
  
    
  // explanation for nodelay()/getch() hack

  // using wgetch(buf_win) in the main loop below caused the terminal to echo characters
  // pressed after a delete which fucks shit up

  // so i'm using getch() on the base window (stdscr)
  // but getch() causes a refresh on the stdscr (which is unused)
  // clearing everything so that the subwindows aren't displayed
  
  // so i'm making getch() not block
  // and running the main loop every ~13ms
  // this works well and uses up little cpu 
  // (also you get ~60fps editing :^) )
  
  nodelay(stdscr, TRUE);

  // get dimensions of terminal
  // this isn't that great because if you resize the terminal the
  // editor will most like break (yeah i never tested)
  getmaxyx(stdscr, window_height, window_width);
  buffer_window_height = window_height-1;
  
  buf_win = newwin(buffer_window_height, window_width, 0, 0);
  stat_win = newwin(1, window_width, window_height-1, 0);
  
  // set window colors
  start_color();
  init_pair(1, COLOR_WHITE, COLOR_BLACK);
  init_pair(2, COLOR_WHITE, COLOR_BLUE);
  wbkgd(buf_win, COLOR_PAIR(1));
  wbkgd(stat_win, COLOR_PAIR(2));
  
  
  
  while(1) {
    
    wmove(buf_win, currow-scrolly, curcol-scrollx);
    wrefresh(stat_win);
    wrefresh(buf_win);

    draw_buffer();
    draw_status();
    
    
    
    int c = getch();
    if(c != ERR) {
      // if getch got a ch
      CLEAR_STATUS();
      if(escape) {
        escape = 0;
        handle_escape(c);
      } else {
        handle_key(c);
      }
    }
    usleep(13000);
  }  
}

