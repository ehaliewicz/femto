#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include "unistd.h"
#include "ncurses.h"

#include "signal.h"
#include "ctype.h"


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

int currow, curcol;
int scrolly, scrollx;

char* buffer_filename;

char* status = "";

WINDOW* buf_win;
WINDOW* stat_win;


void fatal(char *message) {
  fprintf(stderr,"fatal error: %s\n",message);
  exit(1);
}


#define STAT_BUF_SZ 64
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
    if(fp == NULL) {
      fatal("couldn't open file to read!\n");
    }
    fseek(fp, 0L, SEEK_END);
    buffer_size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    
    buffer = calloc(buffer_size, 1);
    
    fread(buffer, 1, buffer_size, fp);
    
    fclose(fp);
    
    cur1 = cur2 = 0;
    
    SET_STATUS("loaded %s", buffer_filename);
    
  } else {
    buffer = calloc(buffer_size, 1);
    cur1 = 0;
    cur2 = buffer_size;
    SET_STATUS("opened new file %s", buffer_filename);
  }
}

int save_buffer() {
  // create or open (overwrite-mode)
  FILE *fp = fopen(buffer_filename, "w+");
  if(fp == NULL) {
    SET_STATUS("couldn't open file to save!");
    return 0;
  } else {
    // save chars before cursor
    fwrite(buffer, 1, cur1, fp);
    // save chars after cursor
    fwrite(buffer+cur2, 1, buffer_size-cur2, fp);
    fclose(fp);
    SET_STATUS("buffer saved");
    return 1;
  }
}

// called when gap is exhausted
void expand_buffer() {
  // this gives us a gap of half the current buffer size
  int newsize = buffer_size * 1.5;
  char* new_buf = calloc(newsize, 1);
  
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

char next_char() {
  return buffer[cur2];
}

char prev_char() {
  return buffer[cur1-1];
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


int cursor_left() {
  if(cur1 == 0 || cur2 == 0) {
    SET_STATUS("reached beginning of buffer");
    return 0;
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
    return 1;
  }
}

int cursor_right() {
  if(cur1 == buffer_size || cur2 == buffer_size) {
    SET_STATUS("reached end of buffer");
    return 0;
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
    return 1;
  }
}

int cursor_up() {
  int destrow = currow-1;
  int destcol = curcol;
  int lastrow = currow;
  int lastcol = curcol;

  // seek row
  while(currow > destrow) {
    cursor_left();
    if(lastrow == currow && lastcol == curcol) {
      SET_STATUS("reached beginning of buffer");
      return 0;
    }
    lastrow = currow;
    lastcol = curcol;
  }
  
  // seek column
  while(curcol > destcol) {
    cursor_left();
    if(lastrow == currow && lastcol == curcol) {
      SET_STATUS("reached beginning of buffer");
      return 0;
    }
    
    // if we went to the next line above, scroll back and exit
    if(currow != destrow) {
      return cursor_right();
    }
    lastrow = currow;
    lastcol = curcol;
  }
  return 1;
}


int cursor_down() {
  int destrow = currow+1;
  int destcol = curcol;
  int lastrow = currow;
  int lastcol = curcol;
  
  // seek row
  while(currow < destrow) {
    cursor_right();
    if(lastrow == currow && lastcol == curcol) {
      SET_STATUS("reached end of buffer");
      return 0;
    }
    lastrow = currow;
    lastcol = curcol;
  }
  
  // seek column
  while(curcol < destcol) {
    cursor_right();
    if(lastrow == currow && lastcol == curcol) {
      SET_STATUS("reached end of buffer");
      return 0;
    }
    
    // if we went to the next line below, scroll back and exit
    if(currow != destrow) {
      return cursor_left();
    }
    lastrow = currow;
    lastcol = curcol;
  }
  return 1;
}


int prev_page() {
  for(int i = 0; i < (2*buffer_window_height); i++) {
    if(!cursor_up()) { return 0; }
  }
  return 1;
}

int next_page() {
  for(int i = 0; i < (2*buffer_window_height); i++) {
    if(!cursor_down()) { return 0; }
  }
  for(int i = 0; i < buffer_window_height; i++) {
    if(!cursor_up()) { return 0; }
  }
  return 1;
}

int insert_char(char c) {
  
  if(cur1 == buffer_size || cur1 == cur2) {
    expand_buffer();
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
  return 1;
}



int delete_char() {
  if(cur1 == 0) {
    SET_STATUS("reached beginning of buffer");
    return 0;
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
    return 1;
  }
}


int delete_char_forward() {
  if(cur2 == buffer_size) {
    SET_STATUS("reached end of buffer");
    return 0;
  } else {
    cur2++;
    return 1;
  }
}


int kill_line() {
  if(next_char() == '\n') { 
    if (!delete_char_forward()) { return 0;} 
  } else {
    while(next_char() != '\n') {
      if (!delete_char_forward()) { return 0; }
    }
  }
  return 1;
}

int is_whitespace(char c) {
  switch(c) {
  case ' ':
  case '\t':
  case '\v':
  case '\r':
  case '\n':
    return 1;
  default:
    return 0;
  }
}


int exit_editor() {
  free(buffer);
  exit(1);
  return 1;
}

int insert_newline() {
  return insert_char('\n');
}


int kill_word() {
  while(is_whitespace(next_char())) {
    if(!delete_char_forward()) { return 0; };
  }
  do {
    if(!delete_char_forward()) { return 0; };
  } while(!is_whitespace(next_char()));
  return 1;
}

int back_word() {
  do {
    if(!cursor_left()) { return 0; }
  } while (!is_whitespace(prev_char()));
  return 1;
}

int forward_word() {
  do {
    if(!cursor_right()) { return 0; }
  } while (!is_whitespace(next_char()));
  return 1;
}



void draw_buffer() {
  int dcol = 0;
  int drow = 0;
  

  
  void draw_char(char c) {
    if(drow >= scrolly &&
       (drow-scrolly < buffer_window_height) && (dcol-scrollx < window_width)) {
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

void setup_screen();


void refresh_editor() {
  wmove(buf_win, currow-scrolly, curcol-scrollx);
  wrefresh(stat_win);
  wrefresh(buf_win);

  draw_status();
  draw_buffer();
}

void handle_resize() {
  
  delwin(buf_win);
  delwin(stat_win);
  endwin();
  
  setup_screen();
  
  
  refresh_editor();
}


int escape = 0;
typedef int (*cmd_func)(void);

typedef struct {
  char* cmd_string;
  cmd_func func;
} cmd;

#define CMD_BUF_SIZE 128

char cmd_buf[CMD_BUF_SIZE];
int cmd_buf_ptr = 0;

// these commands are colemak oriented

cmd commands[] = {
  // {key combo, key function}
  {"C-k", kill_line},
  {"C-w", kill_word},
  {"C-l", back_word},
  {"C-u", forward_word},
  {"C-x c", exit_editor},
  {"C-x s", save_buffer},
  {"C-n", next_page},
  {"C-p", prev_page},
  {"C-j", insert_newline} // enter registers as Ctrl-j for some reason
};

#define NUM_CMDS (sizeof(commands) / sizeof(cmd)) 

void reset_command() {
  escape = 0;
  cmd_buf_ptr = 0;
  memset(cmd_buf, 0, CMD_BUF_SIZE);
}

void record_and_execute(char c) {
  cmd_buf[cmd_buf_ptr++] = c;
  if(cmd_buf_ptr >= CMD_BUF_SIZE) {
    return reset_command();
  } else {
    status = cmd_buf;
    
    for(unsigned int i = 0; i < NUM_CMDS; i++) {
      if(strcmp(commands[i].cmd_string, cmd_buf) == 0) {
        reset_command();
        commands[i].func();
        break;
      }
    }
  }
}



void handle_key(int c) {
  
  char* name;
  switch(c) {
  case KEY_UP:
    cursor_up();
    break;
  
  case KEY_DOWN:
    cursor_down();
    break;
  
  case KEY_LEFT:
    cursor_left();
    break;
  
  case KEY_RIGHT:
    cursor_right();
    break;
    
  case KEY_BACKSPACE:
  case 127:
    delete_char();
    break;
    
  default:
      
    name = keyname(c);
    
    if(name[0] == '^') {
      if(tolower(name[1]) == 'g') { return reset_command(); }
      if(escape) { record_and_execute(' '); }
      escape = 1;
      record_and_execute('C');
      record_and_execute('-');
      record_and_execute(tolower(name[1]));
    
    } else if (escape) {
      record_and_execute(' ');
      record_and_execute(tolower(name[0]));
      if(cmd_buf_ptr > 0) {
        SET_STATUS("%s is undefined.", cmd_buf);
        reset_command();
      }
    } else {
      insert_char(c);
      break;
    }
  }
}


void setup_screen() {
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  
  
  // explanation for nodelay()/getch() hack

  // using wgetch(buf_win) in the main loop below caused the terminal to echo characters
  // pressed after a delete which messes everything up

  // so i'm using getch() on the base window (stdscr)
  // but getch() causes a refresh on the stdscr (which is unused)
  // clearing everything so that the subwindows aren't displayed
  
  // so i'm making getch() not block
  // and running the main loop every ~13ms
  // this works well and uses up little cpu 
  // (also you get ~60fps editing :^) )
  
  nodelay(stdscr, TRUE);
  
  
  // get dimensions of terminal
  
  //window_height = LINES;
  //window_width = COLS;
  getmaxyx(stdscr, window_height, window_width);
  
  
  buffer_window_height = window_height-1;
  
  buf_win = newwin(buffer_window_height, window_width, 0, 0);
  stat_win = newwin(1, window_width, buffer_window_height, 0);
    
  
  
  // set window colors
  start_color();
  
  init_pair(1, COLOR_WHITE, COLOR_BLUE);
  
  
  wbkgd(stat_win, COLOR_PAIR(1));
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

  setup_screen();

  atexit(cleanup);

  
  
  while(1) {
    
    refresh_editor();
        
    int c = getch();
    if(c != ERR && c != KEY_RESIZE) {
      // if getch got a ch
      CLEAR_STATUS();
      handle_key(c);
    } else if (c == KEY_RESIZE) {
      handle_resize();
    }
    usleep(13000);
  }  
}
