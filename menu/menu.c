#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <signal.h>

#define ENTRY_LIMIT 255 /* Same as limit for exit code. */
#define TEXT_LIMIT 128

static char list[ENTRY_LIMIT][TEXT_LIMIT];
static int list_size      = 0;
static int scroll_offset  = 0;
static int selected_entry = 0;

static void update_screen(void)
{
  int n, i, maxy, maxx;
  int scrollbar_size, scrollbar_pos;

  getmaxyx(stdscr, maxy, maxx);
  erase();

  /* Draw text lines. */
  for (n = 0; n < maxy; n++) {
    if ((n + scroll_offset) >= list_size)
      break;

    if (n == (selected_entry - scroll_offset)) {
      attron(A_REVERSE);
      mvaddstr(n, 0, list[n + scroll_offset]);
      for (i = strlen(list[n + scroll_offset]); i < maxx - 2; i++)
        mvaddch(n, i, ' ');
      attroff(A_REVERSE);
    } else {
      mvaddstr(n, 0, list[n + scroll_offset]);
    }
  }

  /* Draw scrollbar. */
  if (list_size <= maxy)
    scrollbar_size = maxy;
  else
    scrollbar_size = maxy / (list_size / (double)maxy);

  scrollbar_pos = selected_entry / (double)list_size * (maxy - scrollbar_size);
  attron(A_REVERSE);
  for (i = 0; i <= scrollbar_size; i++)
    mvaddch(i + scrollbar_pos, maxx - 1, ' ');
  attroff(A_REVERSE);

  mvvline(0, maxx - 2, 0, maxy);

  /* Place cursor at end of selected line. */
  move(selected_entry - scroll_offset, maxx - 3);
}

static void exit_handler(void)
{
  endwin();
}

static void winch_handler(void)
{
  endwin(); /* To get new window limits. */
  update_screen();
  flushinp();
  keypad(stdscr, TRUE);
}

static void interrupt_handler(int signo)
{
  exit(0); /* Exit with code 0, always. */
}

int main(int argc, char *argv[])
{
  int c, maxy, maxx;
  char line[TEXT_LIMIT], *p;
  FILE *fh;

  if (argc != 2) {
    printf("Usage: %s <file with menu lines>\n", argv[0]);
    return 0;
  }

  fh = fopen(argv[1], "r");
  if (fh == NULL) {
    printf("Error: Unable to open menu file for reading.\n");
    return 0;
  }

  while (fgets(line, TEXT_LIMIT, fh) != NULL) {
    for (p = line; *p != '\0'; p++) {
      if ((*p == '\r') || (*p == '\n')) {
        *p = '\0';
        break;
      }
    }
    strncpy(list[list_size], line, TEXT_LIMIT);
    list_size++;
    if (list_size == ENTRY_LIMIT)
      break;
  }
  fclose(fh);

  signal(SIGINT, interrupt_handler);

  initscr();
  atexit(exit_handler);
  noecho();
  keypad(stdscr, TRUE);

  while (1) {
    update_screen();
    getmaxyx(stdscr, maxy, maxx);
    c = getch();

    switch (c) {
    case KEY_RESIZE:
      /* Use this event instead of SIGWINCH for better portability. */
      winch_handler();
      break;

    case KEY_UP:
      selected_entry--;
      if (selected_entry < 0)
        selected_entry++;
      if (scroll_offset > selected_entry) {
        scroll_offset--;
        if (scroll_offset < 0)
          scroll_offset = 0;
      }
      break;

    case KEY_DOWN:
      selected_entry++;
      if (selected_entry >= list_size)
        selected_entry--;
      if (selected_entry > maxy - 1) {
        scroll_offset++;
        if (scroll_offset > selected_entry - maxy + 1)
          scroll_offset--;
      }
      break;

    case KEY_NPAGE:
      scroll_offset += maxy / 2;
      while (maxy + scroll_offset > list_size)
        scroll_offset--;
      if (scroll_offset < 0)
        scroll_offset = 0;
      if (selected_entry < scroll_offset)
        selected_entry = scroll_offset;
      break;

    case KEY_PPAGE:
      scroll_offset -= maxy / 2;
      if (scroll_offset < 0)
        scroll_offset = 0;
      if (selected_entry > maxy + scroll_offset - 1)
        selected_entry = maxy + scroll_offset - 1;
      break;

    case KEY_ENTER:
    case '\n':
    case '\r':
      /* Need to start at 1 to differentiate between a valid selection
         and other kinds of exits, that returns 0. */
      return selected_entry + 1;

    case '\e': /* Escape */
    case 'Q':
    case 'q':
      return 0;
    }

  }

  return 0;
}

