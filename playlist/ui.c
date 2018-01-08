#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <ncurses.h>
#include <unistd.h>
#include <signal.h>
#include "play.h"
#include "list.h"

#define FILTER_LIMIT 50

static int scroll_offset  = 0;
static int selected_entry = 0;
static char file_filter[FILTER_LIMIT] = {'\0'};
static int help_hint_active = 1; /* Display at startup. */
static int help_window_active = 0;
static WINDOW *help_window = NULL;

static void update_screen(void)
{
  int n, i, maxy, maxx, playing;
  char *entry;

  getmaxyx(stdscr, maxy, maxx);
  maxy -= 2;
  erase();

  /* Draw entries. */
  for (n = 0; n < maxy; n++) {
    if ((n + scroll_offset) >= list_size_get())
      break;

    entry = list_get(n + scroll_offset, &playing);
    if (entry == NULL)
      break;

    if (n == (selected_entry - scroll_offset)) {
      attron(A_REVERSE);
      if (playing)
        attron(A_BOLD);
      mvaddnstr(n, 0, entry, maxx);
      for (i = strlen(entry); i < maxx; i++)
        mvaddch(n, i, ' ');
      if (playing)
        attroff(A_BOLD);
      attroff(A_REVERSE);
    } else {
      if (playing)
        attron(A_BOLD);
      mvaddnstr(n, 0, entry, maxx);
      if (playing)
        attroff(A_BOLD);
    }
  }

  /* Draw status bar and filter input, and let cursor rest there. */
  mvhline(maxy, 0, 0, maxx);
  mvprintw(maxy + 1, 0, "> %s", file_filter);

  if (help_hint_active) {
    attron(A_REVERSE);
    mvaddstr(maxy, 0, "Press F1 for help.");
    attroff(A_REVERSE);
  }

  if (help_window_active) {
    if (help_window == NULL) {
      help_window = subwin(stdscr, 13, maxx - 6, 3, 3);
    }
    werase(help_window);
    mvwaddstr(help_window, 1, 1, "F1 - Help");
    mvwaddstr(help_window, 2, 1, "F5 - Go to top (Same as Home key)");
    mvwaddstr(help_window, 3, 1, "F6 - Go to bottom (Same as End key)");
    mvwaddstr(help_window, 4, 1, "F7 - Move entry up");
    mvwaddstr(help_window, 5, 1, "F8 - Move entry down");
    mvwaddstr(help_window, 6, 1, "F9 - Shuffle list (Also if filtered)");
    mvwaddstr(help_window, 7, 1, "Escape - Quit");
    mvwaddstr(help_window, 9, 1,
      "Use Up/Down Arrow keys and Page Up/Down to scroll and move cursor.");
    mvwaddstr(help_window, 10, 1,
      "Use Left or Right Arrow keys to select entry under cursor.");
    mvwaddstr(help_window, 11, 1,
      "Use characters to enter a filter, and press Enter to apply it.");
    box(help_window, ACS_VLINE, ACS_HLINE);
  } else {
    if (help_window != NULL) {
      delwin(help_window);
      help_window = NULL;
    }
  }
}

static void exit_handler(void)
{
  play_cancel();
  list_destroy();
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

static void finished_handler(void)
{
  list_request_next();
  update_screen();
}

int main(int argc, char *argv[])
{
  int c, maxy, maxx, len;
  struct sigaction sa;

  if (argc != 3) {
    printf("Usage: %s <playlist file> <program>\n", argv[0]);
    return 1;
  }
  
  if (list_import_file(argv[1]) != 0) {
    printf("Error: Unable to open playlist file.\n");
    return 1;
  }

  sa.sa_handler = interrupt_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGINT, &sa, NULL);

  play_finished_handler_install(finished_handler);
  play_set_program(argv[2]);

  initscr();
  atexit(exit_handler);
  noecho();
  keypad(stdscr, TRUE);

  list_request(0); /* Start right away at startup. */

  while (1) {
    update_screen();
    getmaxyx(stdscr, maxy, maxx);
    maxy -= 2;
    c = getch();

    /* Don't close the help if interrupted. */
    if (c > 0) {
      help_hint_active = help_window_active = 0;
    }

    switch (c) {
    case KEY_RESIZE:
      /* Use this event instead of SIGWINCH for better portability. */
      winch_handler();
      break;

    case KEY_F(10):
    case KEY_F(1):
      help_window_active = 1;
      break;

    case KEY_F(7):
      list_swap(selected_entry, selected_entry - 1);
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

    case KEY_F(8):
      list_swap(selected_entry, selected_entry + 1);
    case KEY_DOWN:
      selected_entry++;
      if (selected_entry >= list_size_get())
        selected_entry--;
      if (selected_entry > maxy - 1) {
        scroll_offset++;
        if (scroll_offset > selected_entry - maxy + 1)
          scroll_offset--;
      }
      break;

    case KEY_NPAGE:
      scroll_offset += maxy / 2;
      while (maxy + scroll_offset > list_size_get())
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

    case KEY_HOME:
    case KEY_F(5):
      scroll_offset = 0;
      selected_entry = 0;
      break;

    case KEY_END:
    case KEY_F(6):
      if (list_size_get() - maxy > 0)
        scroll_offset = list_size_get() - maxy;
      selected_entry = list_size_get() - 1;
      break;

    case KEY_F(9):
      list_shuffle();
      break;

    case KEY_LEFT:
    case KEY_RIGHT:
      list_request(selected_entry);
      break;

    case KEY_ENTER:
    case '\n':
    case '\r':
      if (list_filter_apply(file_filter) == 0) {
        scroll_offset = 0;
        selected_entry = 0;
      }
      break;

    case '\e': /* Escape */
      return 0;

    case KEY_BACKSPACE:
    case 0x7f:
      len = strlen(file_filter);
      if (len > 0)
        file_filter[len - 1] = '\0';
      break;

    default:
      if (! isprint(c))
        break;
      len = strlen(file_filter);
      if (len <= FILTER_LIMIT - 2)
        file_filter[len] = c;
      break;
    }
  }

  return 0;
}

