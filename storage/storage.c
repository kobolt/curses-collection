#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curses.h>

#define TEXT_X_AREA 20
#define TEXT_MAX TEXT_X_AREA - 3
#define DATA_MAX 24 /* Best fit when using 80x24 terminal. */
#define DATA_DELIMITER "\t"

typedef struct data_s {
  double value;
  char text[TEXT_MAX + 1];
} data_t;

static void screen_init(int *max_y, int *max_x)
{
  initscr();
  atexit((void *)endwin);
  if (has_colors()) {
    start_color();
    init_pair(1, COLOR_RED,     COLOR_BLACK);
    init_pair(2, COLOR_GREEN,   COLOR_BLACK);
    init_pair(3, COLOR_BLUE,    COLOR_BLACK);
    init_pair(4, COLOR_YELLOW,  COLOR_BLACK);
    init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(6, COLOR_CYAN,    COLOR_BLACK);
    init_pair(7, COLOR_WHITE,   COLOR_BLACK);
  }
  noecho();
  getmaxyx(stdscr, *max_y, *max_x);
}

static void box_draw(int pattern, int start_y, int start_x,
  int size_y, int size_x)
{
  int y, x;
  for (y = 0; y < size_y; y++) {
    for (x = 0; x < size_x; x++) {
      if ((pattern % 14) > 6) {
        wattrset(stdscr, A_BOLD | COLOR_PAIR((pattern % 7) + 1));
      } else {
        wattrset(stdscr, COLOR_PAIR((pattern % 7) + 1));
      }
      mvaddch(start_y + y, start_x + x, pattern + 0x41);
    }
  }
  refresh();
}

static void text_draw(int pattern, char *text, int y, int x)
{
  if ((pattern % 14) > 6) {
    wattrset(stdscr, A_BOLD | COLOR_PAIR((pattern % 7) + 1));
  } else {
    wattrset(stdscr, COLOR_PAIR((pattern % 7) + 1));
  }
  mvprintw(y, x, "%c:%s", pattern + 0x41, text);
  refresh();
}

static int data_compare(const void *p1, const void *p2)
{
  return ((data_t *)p1)->value < ((data_t *)p2)->value;
}

int main(int argc, char *argv[])
{
  int i, max_y, max_x, start_y, start_x, size_y, size_x, horizontal;
  double sum, share, used, value;
  char line[128], *p;
  data_t data[DATA_MAX];

  for (i = 0; i < DATA_MAX; i++) {
    data[i].value = 0.0;
    data[i].text[0] = '\0';
  }
  strncpy(data[DATA_MAX - 1].text, "*OTHER*", TEXT_MAX);

  i = 0;
  while (fgets(line, sizeof(line), stdin) != NULL) {
    p = strtok(line, DATA_DELIMITER);
    if (p == NULL)
      continue;

    value = atof(p);

    p = strtok(NULL, DATA_DELIMITER);
    if (p == NULL)
      continue;

    if (i >= (DATA_MAX - 1)) {
      data[DATA_MAX - 1].value += value;
    } else {
      data[i].value = value;
      strncpy(data[i].text, p, TEXT_MAX);
      data[i].text[TEXT_MAX] = '\0';
      i++;
    }
  }

  /* Sort to get largest value first. */
  qsort(data, DATA_MAX, sizeof(data_t), data_compare);

  sum = 0.0;
  for (i = 0; i < DATA_MAX; i++) {
    if (data[i].value <= 0.0)
      break;
    sum += data[i].value;
  }

  if (sum <= 0.0) {
    return 1; /* Will divide by zero, abort. */
  }

  screen_init(&max_y, &max_x);
  max_x -= TEXT_X_AREA;

  used = 0.0;
  horizontal = start_y = start_x = 0;
  size_y = max_y;
  size_x = max_x;
  for (i = 0; i < DATA_MAX; i++) {
    if (data[i].value <= 0.0)
      break;

    share = data[i].value / (sum - used);

    if (horizontal) {
      size_x = max_x - start_x;
      size_y = share * (double)(max_y - start_y);
      box_draw(i, start_y, start_x, size_y, size_x);
      start_y += size_y;
      horizontal = 0;
    } else {
      size_y = max_y - start_y;
      size_x = share * (double)(max_x - start_x);
      box_draw(i, start_y, start_x, size_y, size_x);
      start_x += size_x;
      horizontal = 1;
    }

    used += data[i].value;
  }

  for (i = 0; i < DATA_MAX; i++) {
    if (data[i].value <= 0.0)
      break;
    text_draw(i, data[i].text, i, max_x + 1);
  }

  move(0,0);
  refresh();
  while (1) {
    sleep(1); /* Ctrl-C to quit! */
  }

  return 0;
}

