#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <curses.h>
#include <limits.h>
#include <locale.h>

#define TEXT_MAX 65536
#define SAVE_EXTENSION "scca"
#define PAGE_OFFSET_SKIP 10

static int allowed_char[UCHAR_MAX];
static unsigned char cipher[UCHAR_MAX];
static unsigned char text[TEXT_MAX] = {'\0'};
static int allowed_char_len;
static int cipher_pos = 0;
static int text_offset = 0;

static void cipher_init(void)
{
  unsigned char c;

  setlocale(LC_ALL, "");

  allowed_char_len = 0;
  for (c = 0; c < UCHAR_MAX; c++) {
    if (isupper(c)) {
      allowed_char[c] = allowed_char_len;
      allowed_char_len++;
    } else {
      allowed_char[c] = -1;
    }
    cipher[c] = ' ';
  }
}

static void cipher_erase(void)
{
  unsigned char c;
  for (c = 0; c < UCHAR_MAX; c++) {
    cipher[c] = ' ';
  }
}

static unsigned char cipher_applied(unsigned char plain)
{
  unsigned char c;

  if (isupper(plain)) {
    c = allowed_char[plain];
    if (cipher[c] == ' ') {
      return plain;
    } else {
      return cipher[c];
    }
  } else {
    return plain;
  }
}

static int text_read(char *filename)
{
  int c, n;
  FILE *fh;

  fh = fopen(filename, "r");
  if (! fh) {
    fprintf(stderr, "fopen() failed on file: %s\n", filename);
    return 1;
  }

  setlocale(LC_ALL, "");

  n = 0;
  while ((c = fgetc(fh)) != EOF) {
    if (n > TEXT_MAX)
      break;
    if (c == '\r')
      continue; /* CR causes issues, just strip it. */
    text[n] = toupper(c);
    n++;
  }

  fclose(fh);
  return 0;
}

static void text_save(char *old_filename)
{
  int i;
  FILE *fh;
  static char new_filename[PATH_MAX];

  snprintf(new_filename, PATH_MAX, "%s.%s", old_filename, SAVE_EXTENSION);

  erase();

  fh = fopen(new_filename, "w");
  if (fh == NULL) {
    mvprintw(0, 0, "Could not open file for writing: %s", new_filename);
  } else {
    for (i = 0; i < TEXT_MAX; i++) {
      if (text[i] == '\0')
        break;
      fputc(cipher_applied(text[i]), fh);
    }
    mvprintw(0, 0, "Deciphered text saved to: %s", new_filename);
  }
  fclose(fh);

  mvprintw(1, 0, "Press any key to contiue...");
  refresh();

  flushinp();
  getch(); /* Wait for keypress. */
  flushinp();
}

static void display_help(void)
{
  erase();
  mvprintw(0,  0, "Left:        Move cipher cursor left.");
  mvprintw(1,  0, "Right:       Move ciiper cursor right.");
  mvprintw(2,  0, "Up:          Scroll one line up.");
  mvprintw(3,  0, "Down:        Scroll one line down.");
  mvprintw(4,  0, "Page Up:     Scroll %d lines up.", PAGE_OFFSET_SKIP);
  mvprintw(5,  0, "Page Down:   Scroll %d lines down.", PAGE_OFFSET_SKIP);
  mvprintw(6,  0, "Space:       Erase cipher character.");
  mvprintw(7,  0, "[A-Z]:       Insert cipher character.");
  mvprintw(8,  0, "F1 / F5:     Display this help.");
  mvprintw(9,  0, "F2 / F6:     Display character frequency.");
  mvprintw(10, 0, "F3 / F7:     Reset cipher. (Erase all.)");
  mvprintw(11, 0, "F4 / F8:     Save deciphered text to file.");
  mvprintw(12, 0, "F10:         Quit");
  mvprintw(14, 0, "Press any key to contiue...");
  refresh();

  flushinp();
  getch(); /* Wait for keypress. */
  flushinp();
}

static void display_frequency(void)
{
  int count[UCHAR_MAX];
  int i, y, x, maxy, maxx;
  unsigned char c;

  for (c = 0; c < UCHAR_MAX; c++) {
    count[c] = 0;
  }

  for (i = 0; i < TEXT_MAX; i++) {
    if (text[i] == '\0')
      break;
    count[text[i]]++;
  }

  erase();
  getmaxyx(stdscr, maxy, maxx);
  y = x = 0;
  for (c = 0; c < UCHAR_MAX; c++) {
    if (! isupper(c))
      continue;

    mvprintw(y, x, "%c: %d", c, count[c]);
    x += 10;
    if (x > (maxx - 10)) {
      x = 0;
      y++;
    }
  }
  mvprintw(y + 1, 0, "Press any key to contiue...");
  refresh();
  
  flushinp();
  getch(); /* Wait for keypress. */
  flushinp();
}

static void screen_init(void)
{
  initscr();
  atexit((void *)endwin);
  noecho();
  keypad(stdscr, TRUE);
}

static void screen_update(void)
{
  unsigned char c;
  int i, y, x, maxy, maxx, skip_newline;

  getmaxyx(stdscr, maxy, maxx);
  erase();

  /* Alphabet. */
  x = 0;
  for (c = 0; c < UCHAR_MAX; c++) {
    if (allowed_char[c] != -1) {
      mvaddch(0, x, c);
      x++;
      if (x > maxx)
        break;
    }
  }

  /* Cipher */
  x = 0;
  for (c = 0; c < UCHAR_MAX; c++) {
    mvaddch(1, x, cipher[c]);
    x++;
    if (x > maxx)
      break;
  }

  /* Upper Separation Line */
  mvhline(2, 0, ACS_HLINE, maxx);

  /* Text */
  skip_newline = text_offset;
  move(3, 0);
  for (i = 0; i < TEXT_MAX; i++) {
    if (text[i] == '\0')
      break;

    if (skip_newline > 0) {
      if (text[i] == '\n') {
        skip_newline--;
      }
      continue;
    }

    c = cipher_applied(text[i]);
    if (c != text[i]) {
      attron(A_REVERSE);
    }
    addch(c);
    if (c != text[i]) {
      attroff(A_REVERSE);
    }

    getyx(stdscr, y, x);
    if (y >= (maxy - 1)) {
      break;
    }
  }

  /* Lower Separation Line */
  mvhline(maxy - 1, 0, ACS_HLINE, maxx);

  move(1, cipher_pos);
  refresh();
}

static void screen_resize(void)
{
  endwin(); /* To get new window limits. */
  screen_update();
  flushinp();
  keypad(stdscr, TRUE);
  cipher_pos = 0;
}

int main(int argc, char *argv[])
{
  int c;

  if (argc != 2) {
    fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
    return 0;
  }

  cipher_init();
  if (text_read(argv[1]) != 0) {
    return 1;
  }
  screen_init();

  while (1) {
    screen_update();
    c = getch();

    switch (c) {
    case KEY_RESIZE:
      screen_resize();

    case KEY_LEFT:
      cipher_pos--;
      if (cipher_pos < 0)
        cipher_pos = 0;
      break;

    case KEY_RIGHT:
      cipher_pos++;
      if (cipher_pos > allowed_char_len)
        cipher_pos = allowed_char_len;
      break;

    case KEY_UP:
      text_offset--;
      if (text_offset < 0)
        text_offset = 0;
      break;

    case KEY_DOWN:
      text_offset++;
      /* NOTE: Nothing preventing infinite scrolling... */
      break;

    case KEY_PPAGE:
      text_offset -= PAGE_OFFSET_SKIP;
      if (text_offset < 0)
        text_offset = 0;
      break;
      
    case KEY_NPAGE:
      text_offset += PAGE_OFFSET_SKIP;
      /* NOTE: Nothing preventing infinite scrolling... */
      break;

    case KEY_F(1):
    case KEY_F(5):
      display_help();
      break;

    case KEY_F(2):
    case KEY_F(6):
      display_frequency();
      break;

    case KEY_F(3):
    case KEY_F(7):
      cipher_erase();
      break;

    case KEY_F(4):
    case KEY_F(8):
      text_save(argv[1]);
      break;

    case KEY_F(10):
      exit(0);

    case ' ':
      cipher[cipher_pos] = ' ';
      break;

    default:
      if (isalpha(c)) {
        cipher[cipher_pos] = toupper(c);
      }
      break;
    }
  }

  return 0;
}

