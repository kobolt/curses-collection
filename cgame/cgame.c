#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <ncurses.h>

#define GAME_ROWS 24
#define GAME_COLS 80

#define AMOUNT_OF_MONEY  20
#define AMOUNT_OF_MINES  10
#define AMOUNT_OF_CRATES 5

#define PAIR_YELLOW 1
#define PAIR_CYAN 2

typedef enum {
  DIRECTION_NONE,
  DIRECTION_LEFT,
  DIRECTION_RIGHT,
  DIRECTION_UP,
  DIRECTION_DOWN,
} direction_t;

typedef enum {
  MOVE_OK,
  MOVE_BLOCKED,
  MOVE_ON_MINE,
  MOVE_ON_MONEY,
  MOVE_ON_ENEMY,
} move_t;

typedef struct object_s {
  int y, x;
  char symbol;
  int attributes;
  int taken;
} object_t;

static object_t player;
static object_t enemy;
static object_t money[AMOUNT_OF_MONEY];
static object_t mines[AMOUNT_OF_MINES];
static object_t crates[AMOUNT_OF_CRATES];

static int money_collected;
static int steps_taken;
static int enemy_idle_count;

static void draw_object(object_t *object)
{
  if (object->attributes != 0 && has_colors())
    wattrset(stdscr, object->attributes);
  mvaddch(object->y, object->x, object->symbol);
  if (object->attributes != 0 && has_colors())
    wattrset(stdscr, A_NORMAL);
}

static void find_empty_spot(int *y, int *x)
{
  do {
    move((random() % (GAME_ROWS - 2)) + 1, (random() % (GAME_COLS - 2)) + 1);
  } while ((inch() & A_CHARTEXT) != ' ');
  getyx(stdscr, *y, *x);
}

static void init_game(void)
{
  int i;

  money_collected  = 0;
  steps_taken      = 0;
  enemy_idle_count = 0;

  srandom(time(NULL));

  player.symbol = '@';
  player.attributes = 0;
  find_empty_spot(&player.y, &player.x);
  draw_object(&player);

  enemy.symbol = 'M';
  enemy.attributes = A_BOLD | COLOR_PAIR(PAIR_YELLOW);
  find_empty_spot(&enemy.y, &enemy.x);
  draw_object(&enemy);

  for (i = 0; i < AMOUNT_OF_MONEY; i++) {
    money[i].symbol = '$';
    money[i].attributes = A_BOLD;
    money[i].taken = 0;
    find_empty_spot(&money[i].y, &money[i].x);
    draw_object(&money[i]);
  }

  for (i = 0; i < AMOUNT_OF_MINES; i++) {
    mines[i].symbol = '*';
    mines[i].attributes = COLOR_PAIR(PAIR_CYAN);
    find_empty_spot(&mines[i].y, &mines[i].x);
    draw_object(&mines[i]);
  }

  for (i = 0; i < AMOUNT_OF_CRATES; i++) {
    crates[i].symbol = '#';
    crates[i].attributes = COLOR_PAIR(PAIR_YELLOW);
    find_empty_spot(&crates[i].y, &crates[i].x);
    draw_object(&crates[i]);
  }
}

static direction_t direction_to_player(void)
{
  int y, x;

  y = enemy.y - player.y;
  x = enemy.x - player.x;

  if ((x < 0) && ((x <= (-y)) || (x >= y)) &&
     ((x <= y)                || (x >= (-y))))
    return DIRECTION_RIGHT;
  if ((x > 0) && ((x <= y) || (x >= (-y))) &&
     ((x <= (-y))          || (x >= y)))
    return DIRECTION_LEFT;
  if (y > 0)
    return DIRECTION_UP;
  if (y < 0)
    return DIRECTION_DOWN;

  return DIRECTION_NONE;
}

static direction_t random_direction(void)
{
  switch (random() % 4) {
  case 0:
    return DIRECTION_LEFT;
  case 1:
    return DIRECTION_RIGHT;
  case 2:
    return DIRECTION_UP;
  case 3:
    return DIRECTION_DOWN;
  }
  return DIRECTION_NONE;
}

static void move_object_actual(object_t *object, direction_t direction)
{
  switch (direction) {
  case DIRECTION_LEFT:
    object->x--;
    break;
  case DIRECTION_RIGHT:
    object->x++;
    break;
  case DIRECTION_UP:
    object->y--;
    break;
  case DIRECTION_DOWN:
    object->y++;
    break;
  default:
    break;
  }
}

static move_t move_object_check(object_t *object, direction_t direction)
{
  int i, hit_y, hit_x;
  char hit_object;

  switch (direction) {
  case DIRECTION_LEFT:
    hit_y = object->y;
    hit_x = object->x - 1;
    break;
  case DIRECTION_RIGHT:
    hit_y = object->y;
    hit_x = object->x + 1;
    break;
  case DIRECTION_UP:
    hit_y = object->y - 1;
    hit_x = object->x;
    break;
  case DIRECTION_DOWN:
    hit_y = object->y + 1;
    hit_x = object->x;
    break;
  case DIRECTION_NONE:
  default:
    return MOVE_BLOCKED;
  }
  hit_object = mvinch(hit_y, hit_x) & A_CHARTEXT;

  if (object->symbol == '@') { /* Player. */
    switch (hit_object) {
    case '*':
      move_object_actual(object, direction);
      return MOVE_ON_MINE;
    case '$':
      move_object_actual(object, direction);
      return MOVE_ON_MONEY;
    case 'M':
      move_object_actual(object, direction);
      return MOVE_ON_ENEMY;
    case '#':
      for (i = 0; i < AMOUNT_OF_CRATES; i++) {
        if (hit_y == crates[i].y && hit_x == crates[i].x) {
          if (move_object_check(&crates[i], direction) == MOVE_OK) {
            move_object_actual(object, direction);
            return MOVE_OK;
          } else {
            return MOVE_BLOCKED;
          }
        }
      }
      return MOVE_BLOCKED;
    case ' ':
      move_object_actual(object, direction);
      return MOVE_OK;
    default:
      return MOVE_BLOCKED;
    }

  } else if (object->symbol == 'M') { /* Enemy. */
    if (hit_object == ' ' || hit_object == '@') {
      move_object_actual(object, direction);
      return MOVE_OK;
    } else {
      return MOVE_BLOCKED;
    }

  } else { /* Crates, etc. */
    if (hit_object == ' ') {
      move_object_actual(object, direction);
      return MOVE_OK;
    } else {
      return MOVE_BLOCKED;
    }
  }
}

static void draw_screen(void)
{
  int i;

  erase();

  box(stdscr, '|', '-');

  for (i = 0; i < AMOUNT_OF_MONEY; i++)
    if (! money[i].taken)
      draw_object(&money[i]);
  for (i = 0; i < AMOUNT_OF_MINES; i++)
    draw_object(&mines[i]);
  for (i = 0; i < AMOUNT_OF_CRATES; i++)
    draw_object(&crates[i]);
  draw_object(&player);
  draw_object(&enemy);

  move(player.y, player.x);

  refresh();
}

int main(void)
{
  int i, maxx, maxy, direction;
  char *done = NULL;

  initscr();
  if (has_colors()) {
    start_color();
    init_pair(PAIR_YELLOW, COLOR_YELLOW, COLOR_BLACK);
    init_pair(PAIR_CYAN,   COLOR_CYAN,   COLOR_BLACK);
  }
  noecho();
  keypad(stdscr, TRUE);
  getmaxyx(stdscr, maxy, maxx);

  if (maxy != GAME_ROWS || maxx != GAME_COLS) {
    endwin();
    fprintf(stderr, "Terminal must be %dx%d!\n", GAME_ROWS, GAME_COLS);
    return 1;
  }

  init_game();

  while (done == NULL) {
    draw_screen();

    /* Get player input. */
    direction = DIRECTION_NONE;
    switch (getch()) {
    case KEY_LEFT:
      direction = DIRECTION_LEFT;
      break;

    case KEY_RIGHT:
      direction = DIRECTION_RIGHT;
      break;

    case KEY_UP:
      direction = DIRECTION_UP;
      break;

    case KEY_DOWN:
      direction = DIRECTION_DOWN;
      break;

    case KEY_RESIZE:
      done = "Window resize detected, aborted...";
      break;

    case 'q':
    case 'Q':
    case '\e':
      done = "Quitting...";
      break;

    default:
      break;
    }

    /* Move player, and check for collisions. */
    switch (move_object_check(&player, direction)) {
    case MOVE_OK:
      steps_taken++;
      break;

    case MOVE_BLOCKED:
      break;

    case MOVE_ON_MINE:
      done = "You were killed by a mine!";
      break;

    case MOVE_ON_MONEY:
      for (i = 0; i < AMOUNT_OF_MONEY; i++) {
        if (player.y == money[i].y && player.x == money[i].x) {
          money_collected++;
          money[i].taken = 1;
        }
      }
      break;

    case MOVE_ON_ENEMY:
      done = "You moved onto the enemy!";
      break;
    }

    draw_screen(); /* To update the positions of the objects, not visible. */

    /* Move enemy, and check for collision with player. */
    if (move_object_check(&enemy, direction_to_player()) == MOVE_BLOCKED) {
      enemy_idle_count++;
      if (enemy_idle_count > 3) {
        if (move_object_check(&enemy, random_direction()) == MOVE_OK)
          enemy_idle_count = 0;
      }
    }

    if (player.y == enemy.y && player.x == enemy.x) {
      done = "You were killed by the enemy!";
      break;
    }

    if (money_collected == AMOUNT_OF_MONEY)
      done = "Well done! all money collected!";
  }

  draw_screen();
  endwin();

  fprintf(stderr, "%s\n", done);
  fprintf(stderr, "You collected %d$ of %d$.\n",
    money_collected, AMOUNT_OF_MONEY);
  fprintf(stderr, "You walked %d step%s.\n", 
    steps_taken, (steps_taken == 1) ? "" : "s");

  return 0;
}

