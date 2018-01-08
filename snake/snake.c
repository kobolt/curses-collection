#include <stdlib.h>
#include <ncurses.h>
#include <time.h>
#include <unistd.h>

typedef enum {
  DIRECTION_UP,
  DIRECTION_DOWN,
  DIRECTION_LEFT,
  DIRECTION_RIGHT,
} direction_t;

typedef struct direction_queue_s {
  direction_t direction;
  struct direction_queue_s *next, *prev;
} direction_queue_t;

static direction_queue_t *queue_head = NULL, *queue_tail = NULL;
static int score;
static int pos_y;
static int pos_x;
static int apple_y;
static int apple_x;

static void queue_insert(direction_t direction, int remove_tail)
{
  direction_queue_t *new, *temp;

  new = (direction_queue_t *)malloc(sizeof(direction_queue_t));
  if (new == NULL)
    exit(1);
  new->direction = direction;

  new->next = NULL;
  if (queue_head != NULL)
    queue_head->next = new;
  new->prev = queue_head;
  queue_head = new;
  if (queue_tail == NULL)
    queue_tail = new;

  if (remove_tail && queue_tail != NULL) {
    temp = queue_tail;
    queue_tail = queue_tail->next;
    queue_tail->prev = NULL;
    free(temp);
  }
}

static void update_screen(void)
{
  int current_y, current_x;
  direction_queue_t *current;

  erase();

  box(stdscr, '|', '-');

  current_y = pos_y;
  current_x = pos_x;
  for (current = queue_head; current != NULL; current = current->prev) {
    mvaddch(current_y, current_x, '#');
    switch (current->direction) {
    case DIRECTION_UP:
      current_y++;
      break;
    case DIRECTION_DOWN:
      current_y--;
      break;
    case DIRECTION_LEFT:
      current_x++;
      break;
    case DIRECTION_RIGHT:
      current_x--;
      break;
    }
  }

  mvaddch(apple_y, apple_x, '@');

  move(pos_y, pos_x);

  refresh();
}

static void place_apple(void)
{
  int maxx, maxy;
  getmaxyx(stdscr, maxy, maxx);
  do {
    move((random() % (maxy - 2)) + 1, (random() % (maxx - 2)) + 1); 
  } while (inch() != ' ');
  getyx(stdscr, apple_y, apple_x);
}

static void exit_handler(void)
{
  direction_queue_t *current, *last = NULL;

  if (queue_tail != NULL) {
    for (current = queue_tail; current != NULL; current = current->next) {
      if (last != NULL)
        free(last);
      last = current->prev;
    }
    if (last != NULL)
      free(last);
  }

  endwin();
  printf("\nScore: %d\n", score);
}

int main(void)
{
  int c, maxy, maxx;
  direction_t current_direction, original_direction;

  /* Initialize curses. */
  initscr();
  atexit(exit_handler);
  noecho();
  keypad(stdscr, TRUE);
  timeout(0); /* Non-blocking mode */

  getmaxyx(stdscr, maxy, maxx);
  srandom(time(NULL));

  /* Set initial variables. */
  current_direction = DIRECTION_RIGHT;
  queue_insert(DIRECTION_RIGHT, 0);
  queue_insert(DIRECTION_RIGHT, 0);
  queue_insert(DIRECTION_RIGHT, 0);
  score = 0;
  pos_y = maxy / 2;
  pos_x = maxx / 2;
  place_apple();

  update_screen();

  while (1) {

    /* Read character buffer until empty. */
    original_direction = current_direction;
    while ((c = getch()) != ERR) {
      switch (c) {
      case KEY_RESIZE:
        return 0; /* Not allowed! */
      case KEY_UP:
        if (original_direction != DIRECTION_DOWN)
          current_direction = DIRECTION_UP;
        break;
      case KEY_DOWN:
        if (original_direction != DIRECTION_UP)
          current_direction = DIRECTION_DOWN;
        break;
      case KEY_LEFT:
        if (original_direction != DIRECTION_RIGHT)
          current_direction = DIRECTION_LEFT;
        break;
      case KEY_RIGHT:
        if (original_direction != DIRECTION_LEFT)
          current_direction = DIRECTION_RIGHT;
        break;
      case 'q':
      case 'Q':
      case '\e':
        exit(0);
      }
    }

    /* Move player. */
    switch (current_direction) {
    case DIRECTION_UP:
      pos_y--;
      break;
    case DIRECTION_DOWN:
      pos_y++;
      break;
    case DIRECTION_LEFT:
      pos_x--;
      break;
    case DIRECTION_RIGHT:
      pos_x++;
      break;
    }

    /* Check for collisions. */
    c = mvinch(pos_y, pos_x);
    if (c == ' ') { /* Normal progression. */
      queue_insert(current_direction, 1);
    } else if (c == '@') { /* Expand. */
      queue_insert(current_direction, 0);
      score++;
      place_apple();
    } else { /* Died. */
      flash();
      exit(0);
    }

    /* Redraw screen. */
    update_screen();
    usleep(75000);
  }

  return 0;
}

