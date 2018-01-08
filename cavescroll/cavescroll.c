#include <stdlib.h>
#include <ncurses.h>
#include <time.h>
#include <unistd.h>



#define CAVE_WIDTH 80
#define CAVE_HEIGHT 24
#define CAVE_OPENING_MIN 3
#define CAVE_PADDING_TOP 2
#define CAVE_PADDING_BOTTOM 2

#define GRAPHICS_PLAYER '>'
#define GRAPHICS_AIR ' '
#define GRAPHICS_CAVE '#'
#define GRAPHICS_MONEY '$'
#define GRAPHICS_AMMO '+'
#define GRAPHICS_ENEMY '@'
#define GRAPHICS_SHOT '-'

typedef struct object_s {
  unsigned char type;
  int x, y;
  int removed;
  struct object_s *next;
} object_t;

typedef struct cave_s {
  int top;
  int bottom;
} cave_t;

typedef enum {
  ACTION_NONE,
  ACTION_MOVE_UP,
  ACTION_MOVE_DOWN,
  ACTION_SHOOT,
} action_t;

static cave_t cave[CAVE_WIDTH];
static object_t *objects = NULL;

static int score;
static int ammo;
static int player_x;
static int player_y;



static void object_spawn(unsigned char type)
{
  object_t *new, *curr;

  new = (object_t *)malloc(sizeof(object_t));

  if (objects == NULL) {
    objects = new;
  } else {
    for (curr = objects; curr->next != NULL; curr = curr->next)
      ;
    curr->next = new;
  }

  new->type = type;
  new->next = NULL;
  new->removed = 0;

  switch (type) {
  case GRAPHICS_MONEY:
  case GRAPHICS_AMMO:
  case GRAPHICS_ENEMY:
    new->x = CAVE_WIDTH - 1;
    new->y = cave[CAVE_WIDTH - 1].top + 
      (random() % (cave[CAVE_WIDTH - 1].bottom - cave[CAVE_WIDTH - 1].top));
    break;
  case GRAPHICS_SHOT:
    new->x = player_x + 1;
    new->y = player_y;
    break;
  }
}



static void object_free(object_t *object)
{
  object_t *curr, *prev;

  if (objects == NULL)
    return;

  prev = NULL;
  for (curr = objects; curr != NULL; curr = curr->next) {
    if (curr == object) {
      if (prev == NULL) {
        objects = curr->next;
      } else {
        prev->next = curr->next;
      }
      free(curr);
      return;
    }
    prev = curr;
  }
}



static void objects_cleanup(void)
{
  object_t *curr;

objects_cleanup_again:

  if (objects == NULL)
    return;

  for (curr = objects; curr != NULL; curr = curr->next) {
    if (curr->x < 0 || curr->x > CAVE_WIDTH) {
      object_free(curr);
      /* Need to re-initialize linked list. */
      goto objects_cleanup_again;
    }
  }
}



static void objects_free_all(void)
{
  object_t *curr, *next;

  if (objects == NULL)
    return;

  curr = objects;
  while (curr != NULL) {
    next = curr->next;
    free(curr);
    curr = next;
  }
}



static void objects_shift(void)
{
  object_t *curr;

  if (objects == NULL)
    return;

  for (curr = objects; curr != NULL; curr = curr->next)
    curr->x--;
}



static void objects_col_detect(void)
{
  object_t *curr_a, *curr_b;

  if (objects == NULL)
    return;

  for (curr_a = objects; curr_a != NULL; curr_a = curr_a->next) {
    if (curr_a->removed)
      continue;

    switch (curr_a->type) {
    case GRAPHICS_SHOT:
      if (mvinch(curr_a->y, curr_a->x) == GRAPHICS_CAVE) {
        curr_a->removed = 1;
      }
      for (curr_b = objects; curr_b != NULL; curr_b = curr_b->next) {
        if (curr_b->removed)
          continue;
        if (curr_a == curr_b)
          continue;
        if (curr_a->x == curr_b->x && curr_a->y == curr_b->y) {
          flash();
          if (curr_b->type == GRAPHICS_ENEMY)
            score += 5;
          curr_a->removed = 1;
          curr_b->removed = 1;
        }
      }
      break;

    case GRAPHICS_MONEY:
      if ((curr_a->x == player_x) && (curr_a->y == player_y)) {
        score++;
        curr_a->removed = 1;
      }
      break;

    case GRAPHICS_AMMO:
      if ((curr_a->x == player_x) && (curr_a->y == player_y)) {
        ammo++;
        curr_a->removed = 1;
      }
      break;
    }
  }
}



static void objects_move(void)
{
  object_t *curr;
  action_t action;

  if (objects == NULL)
    return;

  for (curr = objects; curr != NULL; curr = curr->next) {
    if (curr->removed)
      continue;

    if (curr->type == GRAPHICS_ENEMY) {
      action = ACTION_NONE;
      switch (random() % 5) {
      case 0:
        action = ACTION_MOVE_DOWN;
        break;
      case 1:
        action = ACTION_MOVE_UP;
        break;
      case 2:
        action = ACTION_NONE;
        break;
      default:
        if (curr->y > player_y) {
          action = ACTION_MOVE_UP;
        } else if (curr->y < player_y) {
          action = ACTION_MOVE_DOWN;
        }
        break;
      }

      if (curr->x < 0 || curr->x > (CAVE_WIDTH - 1))
        continue;

      if (action == ACTION_MOVE_DOWN) {
        curr->y++;
        if (curr->y >= cave[curr->x].bottom)
          curr->y--;
      } else if (action == ACTION_MOVE_UP) {
        curr->y--;
        if (curr->y < cave[curr->x].top)
          curr->y++;
      }
    }

    if (curr->type == GRAPHICS_SHOT) {
      curr->x++;
      objects_col_detect();
      curr->x++;
    }
  }

  objects_col_detect();
}



static void objects_draw(void)
{
  object_t *curr;

  if (objects == NULL)
    return;

  for (curr = objects; curr != NULL; curr = curr->next) {
    if (curr->removed)
      continue;
    mvaddch(curr->y, curr->x, curr->type);
  }
}



static void generate_cave(void)
{
  int i;

  /* Shift entire cave. */
  for (i = 0; i < (CAVE_WIDTH - 1); i++) {
    cave[i].top    = cave[i + 1].top;
    cave[i].bottom = cave[i + 1].bottom;
  }

  objects_shift();

  /* Next top. */
  switch (random() % 5) {
  case 0:
    cave[CAVE_WIDTH - 1].top--;
    if (cave[CAVE_WIDTH - 1].top < CAVE_PADDING_TOP) {
      cave[CAVE_WIDTH - 1].top++;
    }
    break;
  case 1:
    cave[CAVE_WIDTH - 1].top++;
    if (cave[CAVE_WIDTH - 1].top >
      cave[CAVE_WIDTH - 1].bottom - CAVE_OPENING_MIN) {
      cave[CAVE_WIDTH - 1].top--;
    }
    break;
  }

  /* Next bottom. */
  switch (random() % 5) {
  case 0:
    cave[CAVE_WIDTH - 1].bottom++;
    if (cave[CAVE_WIDTH - 1].bottom > CAVE_HEIGHT - CAVE_PADDING_BOTTOM) {
      cave[CAVE_WIDTH - 1].bottom--;
    }
    break;
  case 1:
    cave[CAVE_WIDTH - 1].bottom--;
    if (cave[CAVE_WIDTH - 1].bottom <=
      cave[CAVE_WIDTH - 1].top + CAVE_OPENING_MIN) {
      cave[CAVE_WIDTH - 1].bottom++;
    }
    break;
  }

  /* Possible item. */
  switch (random() % 20) {
  case 0:
    object_spawn(GRAPHICS_MONEY);
    break;
  case 1:
    object_spawn(GRAPHICS_AMMO);
    break;
  case 2:
    object_spawn(GRAPHICS_ENEMY);
    break;
  default:
    break;
  }
}



static void screen_update(void)
{
  int i, j;

  erase();

  /* Main cave. */
  for (i = 0; i < CAVE_WIDTH; i++) {
    for (j = 0; j < cave[i].top; j++) {
      mvaddch(j, i, GRAPHICS_CAVE);
    }
    for (j = cave[i].bottom; j < CAVE_HEIGHT; j++) {
      mvaddch(j, i, GRAPHICS_CAVE);
    }
  }

  /* HUD. */
  mvprintw(0, 0, "Score: %5d Ammo: %3d ", score, ammo);

  objects_draw();

  /* Position cursor on player. */
  mvaddch(player_y, player_x, GRAPHICS_PLAYER);
  move(player_y, player_x);

  refresh();
}



static void screen_end(void)
{
  endwin();
  printf("\nScore: %d\n", score);
}



static void screen_initialize(void)
{
  int maxy, maxx;

  initscr();

  getmaxyx(stdscr, maxy, maxx);
  if (maxx < CAVE_WIDTH || maxy < CAVE_HEIGHT) {
    endwin();
    printf("\nScreen must at least %dx%d.\n", CAVE_WIDTH, CAVE_HEIGHT);
    exit(0);
  }

  atexit(screen_end);
  noecho();
  keypad(stdscr, TRUE);
  timeout(0); /* Non-blocking mode */
}



static void data_initialize(void)
{
  int i;

  srandom(time(NULL));

  score = 0;
  ammo =  5;

  player_x = 3;
  player_y = CAVE_HEIGHT / 2;

  cave[CAVE_WIDTH - 1].top = CAVE_HEIGHT / 4;
  cave[CAVE_WIDTH - 1].bottom = cave[CAVE_WIDTH - 1].top * 3;

  for (i = 0; i < CAVE_WIDTH - 1; i++) {
    generate_cave();
  }

  atexit(objects_free_all);
}



int main(void)
{
  int c;
  action_t action;

  data_initialize();
  screen_initialize();

  screen_update();

  while (1) {

    action = ACTION_NONE;
    while ((c = getch()) != ERR) {
      switch (c) {
      case KEY_RESIZE:
        return 0; /* Not allowed! */

      case KEY_UP:
        action = ACTION_MOVE_UP;
        break;

      case KEY_DOWN:
        action = ACTION_MOVE_DOWN;
        break;

      case ' ':
        action = ACTION_SHOOT;
        break;

      case 'q':
      case 'Q':
      case '\e':
        exit(0);

      default:
        break;
      }
    }

    switch (action) {
    case ACTION_MOVE_UP:
      player_y--;
      break;

    case ACTION_MOVE_DOWN:
      player_y++;
      break;

    case ACTION_SHOOT:
      if (ammo > 0) {
        object_spawn(GRAPHICS_SHOT);
        ammo--;
      }
      break;

    default:
      break;
    }

    generate_cave();
    objects_move();

    /* Check for player death collisions, one step ahead. */
    c = mvinch(player_y, player_x + 1);
    if (c == GRAPHICS_CAVE || c == GRAPHICS_ENEMY) {
      flash();
      exit(0);
    }

    /* Redraw screen. */
    screen_update();
    usleep(100000);

    objects_cleanup();
  }

  return 0;
}

