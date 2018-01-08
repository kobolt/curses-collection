#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curses.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "node.h"



#define DIFF_PROGRAM "diff"
#define DIFF_ARG "-up"
#define PAGER_PROGRAM "less"

#define COLOR_EQUAL            1
#define COLOR_DIFFERS          2
#define COLOR_ADDED            3
#define COLOR_MISSING          4
#define COLOR_SELECTED_EQUAL   5
#define COLOR_SELECTED_DIFFERS 6
#define COLOR_SELECTED_ADDED   7
#define COLOR_SELECTED_MISSING 8



static int scroll_offset  = 0;
static int selected_entry = 0;



static diff_node_t *diff_navi_node_get(diff_node_t *node, int node_no, int *size)
{
  diff_node_t *found;
  int i;

  if (node->type != DIFF_TYPE_ROOT) {
    *size = *size + 1;
  }

  if (*size == node_no) {
    return node; /* Match found, return self. */
  }

  if (node->expanded) {
    for (i = 0; i < node->no_of_subnodes; i++) {
      found = diff_navi_node_get(node->subnode[i], node_no, size);
      if (found != NULL)
        return found;
    }
  }

  return NULL;
}



static void diff_navi_list_expand(diff_node_t *node, int node_no, bool setting)
{
  int size;
  diff_node_t *found;

  size = 0;
  found = diff_navi_node_get(node, node_no, &size);
  if (found == NULL)
    return;

  switch (found->type) {
  case DIFF_TYPE_DIR_EQUAL:
  case DIFF_TYPE_DIR_DIFFERS:
  case DIFF_TYPE_DIR_ADDED:
  case DIFF_TYPE_DIR_MISSING:
    if (found->no_of_subnodes > 0) {
      found->expanded = setting;
    }
    break;
  default:
    break;
  }
}



static void diff_navi_call_program(diff_node_t *node, int node_no, char *root1, char *root2)
{
  diff_node_t *found;
  pid_t pid1, pid2;
  int pipe_fd[2];
  int size, status;
  char path1[PATH_MAX], path2[PATH_MAX], temp[PATH_MAX];

  size = 0;
  found = diff_navi_node_get(node, node_no, &size);
  if (found == NULL)
    return;

  switch (found->type) {
  case DIFF_TYPE_FILE_DIFFERS:
    endwin();

    pid1 = fork();
    if (pid1 == -1) {
      /* Nothing to do... */

    } else if (pid1 > 0) {
      waitpid(pid1, &status, 0);

    } else {

      pipe(pipe_fd);
      pid2 = fork();
      if (pid2 == -1) {
        _exit(0);

      } else if (pid2 > 0) {
        dup2(pipe_fd[0], STDIN_FILENO);
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        execlp(PAGER_PROGRAM, PAGER_PROGRAM, NULL);
        _exit(0);

      } else {
        snprintf(path1, PATH_MAX, "%s/%s",
          root1, diff_node_path(found, temp, PATH_MAX)),
        snprintf(path2, PATH_MAX, "%s/%s",
          root2, diff_node_path(found, temp, PATH_MAX)),

        dup2(pipe_fd[1], STDOUT_FILENO);
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        execlp(DIFF_PROGRAM, DIFF_PROGRAM, DIFF_ARG, path1, path2, NULL);
        _exit(0);
      }
    }

    flushinp();
    keypad(stdscr, TRUE);
    break;

  default:
    break;
  }
}



static void diff_navi_list_draw(diff_node_t *node, int line_no, int node_no, int selected)
{
  int size, maxy, maxx, pos, depth;
  diff_node_t *found;

  size = 0;
  found = diff_navi_node_get(node, node_no, &size);
  if (found == NULL)
    return;

  getmaxyx(stdscr, maxy, maxx);

  if (selected)
    attron(A_REVERSE);

  /* Color. */
  switch (found->type) {
  case DIFF_TYPE_FILE_EQUAL:
  case DIFF_TYPE_DIR_EQUAL:
    if (selected) {
      attrset(COLOR_PAIR(COLOR_SELECTED_EQUAL));
    } else {
      attrset(COLOR_PAIR(COLOR_EQUAL));
    }
    break;

  case DIFF_TYPE_FILE_DIFFERS:
  case DIFF_TYPE_DIR_DIFFERS:
    if (selected) {
      attrset(COLOR_PAIR(COLOR_SELECTED_DIFFERS));
    } else {
      attrset(COLOR_PAIR(COLOR_DIFFERS));
    }
    break;

  case DIFF_TYPE_FILE_ADDED:
  case DIFF_TYPE_DIR_ADDED:
    if (selected) {
      attrset(COLOR_PAIR(COLOR_SELECTED_ADDED));
    } else {
      attrset(COLOR_PAIR(COLOR_ADDED));
    }
    break;

  case DIFF_TYPE_FILE_MISSING:
  case DIFF_TYPE_DIR_MISSING:
    if (selected) {
      attrset(COLOR_PAIR(COLOR_SELECTED_MISSING));
    } else {
      attrset(COLOR_PAIR(COLOR_MISSING));
    }
    break;

  default:
    break;
  }

  if (found->name == NULL) {
    for (pos = 0; pos < maxx - 2; pos++)
      mvaddch(line_no, pos, ' ');

  } else {

    pos = 0;

    /* Depth indicator. */
    depth = diff_node_depth(found);
    while (depth-- > 1) {
      mvaddch(line_no, pos++, ' ');
      mvaddch(line_no, pos++, ' ');
    }

    /* Type indicator. */
    switch (found->type) {
    case DIFF_TYPE_FILE_EQUAL:
    case DIFF_TYPE_DIR_EQUAL:
      mvaddch(line_no, pos++, '=');
      break;

    case DIFF_TYPE_FILE_DIFFERS:
    case DIFF_TYPE_DIR_DIFFERS:
      mvaddch(line_no, pos++, '*');
      break;

    case DIFF_TYPE_FILE_ADDED:
    case DIFF_TYPE_DIR_ADDED:
      mvaddch(line_no, pos++, '+');
      break;

    case DIFF_TYPE_FILE_MISSING:
    case DIFF_TYPE_DIR_MISSING:
      mvaddch(line_no, pos++, '-');
      break;

    default:
      break;
    }

    /* File/directory name. */
    mvaddstr(line_no, pos, found->name);
    pos += strlen(found->name);

    /* Slash for directory. */
    switch (found->type) {
    case DIFF_TYPE_DIR_EQUAL:
    case DIFF_TYPE_DIR_DIFFERS:
    case DIFF_TYPE_DIR_ADDED:
    case DIFF_TYPE_DIR_MISSING:
      mvaddch(line_no, pos++, '/');
      break;

    default:
      break;
    }

    /* Arrow when not expanded. */
    if (! found->expanded) {
      mvaddch(line_no, pos++, ' ');
      mvaddch(line_no, pos++, '-');
      mvaddch(line_no, pos++, '>');
    }

    /* Padding. */
    for (; pos < maxx - 2; pos++)
      mvaddch(line_no, pos, ' ');
  }

  if (selected)
    attroff(A_REVERSE);

  attrset(COLOR_PAIR(COLOR_EQUAL));
}



static int diff_navi_list_size(diff_node_t *node)
{
  int i, size;

  if (node->type == DIFF_TYPE_ROOT) {
    size = 0;
  } else {
    size = 1;
  }

  if (node->expanded) {
    for (i = 0; i < node->no_of_subnodes; i++) {
      size += diff_navi_list_size(node->subnode[i]);
    }
  }

  return size;
}



static void diff_navi_update_screen(diff_node_t *node)
{
  int n, i, maxy, maxx;
  int scrollbar_size, scrollbar_pos;
  int list_size;

  list_size = diff_navi_list_size(node);

  getmaxyx(stdscr, maxy, maxx);
  erase();

  /* Draw text lines. */
  for (n = 0; n < maxy; n++) {
    if ((n + scroll_offset) >= list_size)
      break;

    if (n == (selected_entry - scroll_offset)) {
      diff_navi_list_draw(node, n, n + scroll_offset + 1, 1);
    } else {
      diff_navi_list_draw(node, n, n + scroll_offset + 1, 0);
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



static void diff_navi_exit_handler(void)
{
  endwin();
}



static void diff_navi_winch_handler(diff_node_t *node)
{
  endwin(); /* To get new window limits. */
  diff_navi_update_screen(node);
  flushinp();
  keypad(stdscr, TRUE);
}



void diff_navi_loop(diff_node_t *node, char *root1, char *root2)
{
  int c, maxy, maxx, list_size;

  initscr();
  atexit(diff_navi_exit_handler);
  if (has_colors()) {
    start_color();
    init_pair(COLOR_EQUAL,            COLOR_WHITE,   COLOR_BLACK);
    init_pair(COLOR_DIFFERS,          COLOR_MAGENTA, COLOR_BLACK);
    init_pair(COLOR_ADDED,            COLOR_BLUE,    COLOR_BLACK);
    init_pair(COLOR_MISSING,          COLOR_RED,     COLOR_BLACK);
    init_pair(COLOR_SELECTED_EQUAL,   COLOR_BLACK,   COLOR_WHITE);
    init_pair(COLOR_SELECTED_DIFFERS, COLOR_MAGENTA, COLOR_WHITE);
    init_pair(COLOR_SELECTED_ADDED,   COLOR_BLUE,    COLOR_WHITE);
    init_pair(COLOR_SELECTED_MISSING, COLOR_RED,     COLOR_WHITE);
  }
  noecho();
  keypad(stdscr, TRUE);

  while (1) {
    list_size = diff_navi_list_size(node);
    diff_navi_update_screen(node);
    getmaxyx(stdscr, maxy, maxx);
    c = getch();

    switch (c) {
    case KEY_RESIZE:
      diff_navi_winch_handler(node);
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

    case KEY_LEFT:
    case '-':
      diff_navi_list_expand(node, selected_entry + 1, false);
      break;

    case KEY_RIGHT:
    case '+':
      diff_navi_list_expand(node, selected_entry + 1, true);
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
      /* For ease of use, attempt to expand with "Enter" as well: */
      diff_navi_list_expand(node, selected_entry + 1, true);
      diff_navi_call_program(node, selected_entry + 1, root1, root2);
      break;

    case '\e': /* Escape */
    case 'Q':
    case 'q':
      return;
    }
  }
}



