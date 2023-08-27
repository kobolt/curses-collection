#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <curses.h>

const int SPEED_MAX = 30;
const int SPEED_MIN = 0;

const int HPOS_MAX = 35;
const int HPOS_MIN = -35;

const int ROAD_HEIGHT = 19;
const int HORIZON_HEIGHT = 5;

#define COL_BLACK   1
#define COL_RED     2
#define COL_GREEN   3
#define COL_YELLOW  4
#define COL_BLUE    5
#define COL_MAGENTA 6
#define COL_CYAN    7
#define COL_WHITE   8

#define CARS_MAX 5

typedef struct car_s {
  float distance; /* Relative to Player */
  int row; /* Calculated */
  int speed; /* Constant */
  int hpos; /* Constant */
} car_t;

static void draw_road(int curve, int speed, bool draw_blank)
{
  static unsigned int ticks = 0;
  int row;
  int col;
  int road_start;
  int road_width;
  bool divider_toggle;

  curve += 36; /* Relative to center. */

  divider_toggle = ((ticks * speed) / SPEED_MAX) % 2;

  for (row = 0; row < ROAD_HEIGHT; row++) {
    road_start = (curve / (float)(ROAD_HEIGHT - 1)) * (ROAD_HEIGHT - 1 - row);
    road_width = ((row * 4) + 1);

    /* Left side of the road: */
    for (col = 0; col < road_start + 3; col++) {
      if (has_colors()) {
        attron(COLOR_PAIR(COL_GREEN));
      }
      if (draw_blank) {
        mvaddch(5 + row, col, ' ');
      } else {
        mvaddch(5 + row, col, '#');
      }
      if (has_colors()) {
        attroff(COLOR_PAIR(COL_GREEN));
      }
    }
    
    /* Lane divider: */
    if (row > 1 && (row % 2 == divider_toggle)) {
      if (has_colors()) {
        attron(COLOR_PAIR(COL_WHITE));
      }
      if (draw_blank) {
        mvaddch(5 + row, road_start + (road_width / 2) + 3, ' ');
      } else {
        mvaddch(5 + row, road_start + (road_width / 2) + 3, '|');
      }
      if (has_colors()) {
        attroff(COLOR_PAIR(COL_WHITE));
      }
    }

    /* Right side of the road: */
    for (col = road_start + road_width + 3; col < 80; col++) {
      if (has_colors()) {
        attron(COLOR_PAIR(COL_GREEN));
      }
      if (draw_blank) {
        mvaddch(5 + row, col, ' ');
      } else {
        mvaddch(5 + row, col, '#');
      }
      if (has_colors()) {
        attroff(COLOR_PAIR(COL_GREEN));
      }
    }
  }

  ticks++;
}

static void draw_horizon(int curve, int speed, bool draw_blank)
{
  int row;
  int col;
  static float direction = 0.0;
  char c;

  const char horizon[5][361] = {
    "........................................................................................................................................................................................................................................................................................................................................................................",
    ".............................................***.............................................................................................................................................................MMM........................................................................................................................................................",
    "....MMM.....................................*****...................................................MMMMMMMMM.......................................................................MMMM......MMMMMM......MMMMMMMMMM.................MMMM...................................................................MMM.........................................................",
    "...MMMMM...MMMMMM....MMMMM...................***..................................................MMMMMMMMMMMMM..........MMMM....................................MMMMMM..........MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM........MMMMMMMMMMMMMM....MMMMMMMMM...............MMMMMM...........................MMMMMMM.......................................................",
    "MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM................................................................MMMMMMMMMMMMMMMMM........MMMMMMM........MMMMMMMM............MMMMMMMMMMMMMM.....MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM...MMMMMMMMMMMM...MMMMMMMMMMMM...MMMMMMMMMMMMMMM...........MMMM..MMMMMMM.......................MMMMMM",
  };

  /* Calculate current direction: */
  direction += (((speed / (double)SPEED_MAX) * curve) / 20.0);
  if (direction < 0) direction = 360;
  if (direction > 360) direction = 0;

  /* Sky: */
  for (row = 0; row < 5; row++) {
    for (col = 0; col < 80; col++) {
      c = horizon[row][(col + (int)direction) % 360];
      if (has_colors()) {
        switch (c) {
        case '*':
          attron(COLOR_PAIR(COL_YELLOW));
          break;
        case 'M':
          attron(COLOR_PAIR(COL_GREEN));
          break;
        default:
          attron(COLOR_PAIR(COL_RED));
          break;
        }
      }

      if (draw_blank) {
        mvaddch(row, col, ' ');
      } else {
        mvaddch(row, col, c);
      }

      if (has_colors()) {
        switch (c) {
        case '*':
          attroff(COLOR_PAIR(COL_YELLOW));
          break;
        case 'M':
          attroff(COLOR_PAIR(COL_GREEN));
          break;
        default:
          attroff(COLOR_PAIR(COL_RED));
          break;
        }
      }
    }
  }
}

static void draw_car(int curve, int row, int hpos, bool player, bool draw_blank)
{
  int col;
  int road_width;
  int road_start;

  if (row == -1) {
    return;
  }

  /* Compensate for distance: */
  road_width = ((row - HORIZON_HEIGHT) * 4);
  hpos = ((float)hpos / 80.0) * road_width;

  /* Compensate for curve: */
  road_start = (curve / (float)(ROAD_HEIGHT - 1)) *
    (ROAD_HEIGHT - 1 - (row - HORIZON_HEIGHT));
  hpos -= road_start;

  /* Size of the car depends on the row where it is drawn: */
  if (row > 16) { /* Large */
    col = 34 - hpos;

    /* Body: */
    if (player) {
      if (has_colors()) {
        attron(COLOR_PAIR(COL_MAGENTA));
      }
      if (draw_blank) {
        mvaddstr(row    , col + 1, "          ");
        mvaddstr(row + 1, col    , "            ");
        mvaddstr(row + 2, col    , "            ");
      } else {
        mvaddstr(row    , col + 1, "/%%%%%%%%\\");
        mvaddstr(row + 1, col    , "/%%%%%%%%%%\\");
        mvaddstr(row + 2, col    , "|%%%%%%%%%%|");
      }
      if (has_colors()) {
        attroff(COLOR_PAIR(COL_MAGENTA));
      }

    } else {
      if (has_colors()) {
        attron(COLOR_PAIR(COL_CYAN));
      }
      if (draw_blank) {
        mvaddstr(row    , col + 1, "          ");
        mvaddstr(row + 1, col    , "            ");
        mvaddstr(row + 2, col    , "            ");
      } else {
        mvaddstr(row    , col + 1, "/########\\");
        mvaddstr(row + 1, col    , "/##########\\");
        mvaddstr(row + 2, col    , "|##########|");
      }
      if (has_colors()) {
        attroff(COLOR_PAIR(COL_CYAN));
      }
    }

    /* Tyres: */
    if (has_colors()) {
      attron(COLOR_PAIR(COL_BLUE));
    }
    if (draw_blank) {
      mvaddch(row + 2, col + 1,  ' ');
      mvaddch(row + 2, col + 2,  ' ');
      mvaddch(row + 2, col + 9,  ' ');
      mvaddch(row + 2, col + 10, ' ');
    } else {
      mvaddch(row + 2, col + 1,  '@');
      mvaddch(row + 2, col + 2,  '@');
      mvaddch(row + 2, col + 9,  '@');
      mvaddch(row + 2, col + 10, '@');
    }
    if (has_colors()) {
      attroff(COLOR_PAIR(COL_BLUE));
    }

  } else if (row > 11) { /* Medium */
    col = 36 - hpos;

    /* Body: */
    if (has_colors()) {
      attron(COLOR_PAIR(COL_CYAN));
    }
    if (draw_blank) {
      mvaddstr(row    , col + 1, "      ");
      mvaddstr(row + 1, col    , "        ");
    } else {
      mvaddstr(row    , col + 1, "/####\\");
      mvaddstr(row + 1, col    , "|######|");
    }
    if (has_colors()) {
      attroff(COLOR_PAIR(COL_CYAN));
    }

    /* Tyres: */
    if (has_colors()) {
      attron(COLOR_PAIR(COL_BLUE));
    }
    if (draw_blank) {
      mvaddch(row + 1, col + 1,  ' ');
      mvaddch(row + 1, col + 6,  ' ');
    } else {
      mvaddch(row + 1, col + 1,  '@');
      mvaddch(row + 1, col + 6,  '@');
    }
    if (has_colors()) {
      attroff(COLOR_PAIR(COL_BLUE));
    }

  } else if (row > 8) { /* Small */
    col = 38 - hpos;

    /* Body: */
    if (has_colors()) {
      attron(COLOR_PAIR(COL_CYAN));
    }
    if (draw_blank) {
      mvaddstr(row, col, "   ");
    } else {
      mvaddstr(row, col, "###");
    }
    if (has_colors()) {
      attroff(COLOR_PAIR(COL_CYAN));
    }

  } else if (row > 4) { /* Tiny */
    col = 39 - hpos;

    /* Body: */
    if (has_colors()) {
      attron(COLOR_PAIR(COL_CYAN));
    }
    if (draw_blank) {
      mvaddch(row, col, ' ');
    } else {
      mvaddch(row, col, '#');
    }
    if (has_colors()) {
      attroff(COLOR_PAIR(COL_CYAN));
    }
  }
}

static void car_row_update(car_t *car)
{
  if (car->distance > 1500)      { car->row = -1; }
  else if (car->distance > 1200) { car->row = 6; }
  else if (car->distance > 1000) { car->row = 7; }
  else if (car->distance > 800)  { car->row = 8; }
  else if (car->distance > 600)  { car->row = 9; }
  else if (car->distance > 500)  { car->row = 10; }
  else if (car->distance > 400)  { car->row = 11; }
  else if (car->distance > 300)  { car->row = 12; }
  else if (car->distance > 200)  { car->row = 13; }
  else if (car->distance > 150)  { car->row = 14; }
  else if (car->distance > 100)  { car->row = 15; }
  else if (car->distance > 75)   { car->row = 16; }
  else if (car->distance > 50)   { car->row = 17; }
  else if (car->distance > 25)   { car->row = 18; }
  else if (car->distance > 10)   { car->row = 19; }
  else if (car->distance > 0)    { car->row = 20; }
  else if (car->distance > -10)  { car->row = 21; }
  else if (car->distance > -25)  { car->row = 22; }
  else if (car->distance > -50)  { car->row = 23; }
  else if (car->distance > -75)  { car->row = -1; }
  else { car->row = -1; }
}

static int curve_ahead(int speed)
{
  static int curve_internal = 0;
  static float distance = 0;

  distance += speed;
  if (distance > 100) {
    curve_internal += (1 - random() % 3);
    if (curve_internal > 30) {
      curve_internal = 30;
    } else if (curve_internal < -30) {
      curve_internal = -30;
    }
    distance = 0;
  }

  return curve_internal;
}

static void car_init(car_t *car, bool far_away)
{
  for (int i = 0; i < CARS_MAX; i++) {
    car->speed = 3 + (random() % 15);
    car->hpos = HPOS_MIN + (random() % (HPOS_MAX - HPOS_MIN));
    if (far_away) {
      car->distance = 1500 + (random() % 2500);
    } else {
      car->distance = 100 + (random() % 4000);
    }
  }
}

static bool car_collision(car_t *car, int hpos)
{
  if (car->distance > 10) {
    return false;
  }

  if (car->distance < -10) {
    return false;
  }

  if (abs(car->hpos - hpos) > 12) {
    return false;
  }

  return true;
}

static void display_help(const char *progname)
{
  fprintf(stdout, "Usage: %s <options>\n", progname);
  fprintf(stdout, "Options:\n"
    "  -h   Display this help.\n"
    "  -c   Disable colors.\n"
    "  -a   Disable ASCII symbols.\n");
}

int main(int argc, char *argv[])
{
  int c;
  bool disable_color = false;
  bool disable_ascii = false;
  int ch;
  int player_speed = 5;
  int player_hpos = 25;
  int cars_passed = 0;
  int curve = 0;
  bool quit = false;
  car_t cars[CARS_MAX];

  srandom(time(NULL));

  /* Initialize: */
  for (int i = 0; i < CARS_MAX; i++) {
    cars[i].distance = 0;
    car_init(&cars[i], false);
  }

  while ((c = getopt(argc, argv, "hca")) != -1) {
    switch (c) {
    case 'h':
      display_help(argv[0]);
      return EXIT_SUCCESS;

    case 'c':
      disable_color = true;
      break;

    case 'a':
      disable_ascii = true;
      break;

    case '?':
    default:
      display_help(argv[0]);
      return EXIT_FAILURE;
    }
  }

  if (disable_color && disable_ascii) {
    fprintf(stdout, "Disabling both color and ASCII makes no sense!\n");
    return EXIT_FAILURE;
  }

  initscr();
  noecho();
  keypad(stdscr, TRUE);
  timeout(0);

  if (has_colors()) {
    if (! disable_color) {
      start_color();
    }
    init_pair(COL_BLACK,   COLOR_WHITE, COLOR_BLACK);
    init_pair(COL_RED,     COLOR_BLACK, COLOR_RED);
    init_pair(COL_GREEN,   COLOR_BLACK, COLOR_GREEN);
    init_pair(COL_YELLOW,  COLOR_BLACK, COLOR_YELLOW);
    init_pair(COL_BLUE,    COLOR_BLACK, COLOR_BLUE);
    init_pair(COL_MAGENTA, COLOR_BLACK, COLOR_MAGENTA);
    init_pair(COL_CYAN,    COLOR_BLACK, COLOR_CYAN);
    init_pair(COL_WHITE,   COLOR_BLACK, COLOR_WHITE);
  }

  while (! quit) {
    /* Player input: */
    ch = getch();
    switch (ch) {
    case KEY_LEFT:
    case 'a':
      if (player_hpos < HPOS_MAX) {
        player_hpos++;
      }
      break;

    case KEY_RIGHT:
    case 'd':
      if (player_hpos > HPOS_MIN) {
        player_hpos--;
      }
      break;

    case KEY_UP:
    case 'w':
      if (player_speed < SPEED_MAX) {
        player_speed++;
      }
      break;

    case KEY_DOWN:
    case 's':
      if (player_speed > SPEED_MIN) {
        player_speed--;
      }
      break;

    case 'q':
      quit = true;
      break;

    default:
      break;
    }

    /* Position update of other cars: */
    for (int i = 0; i < CARS_MAX; i++) {
      cars[i].distance += (cars[i].speed - player_speed);

      /* Re-spawn car if it goes out of range: */
      if (cars[i].distance > 6000) {
        car_init(&cars[i], true);
      } else if (cars[i].distance < -100) {
        car_init(&cars[i], true);
        cars_passed++;
      }

      car_row_update(&cars[i]);
    }

    /* Collision check: */
    for (int i = 0; i < CARS_MAX; i++) {
      if (car_collision(&cars[i], player_hpos)) {
        endwin();
        fprintf(stdout, "Crashed!\n");
        quit = true;
      }
    }

    /* Draw screen: */
    erase();
    draw_horizon(curve, player_speed, disable_ascii);
    draw_road(curve, player_speed, disable_ascii);
    for (int i = 0; i < CARS_MAX; i++) {
      draw_car(curve, cars[i].row, cars[i].hpos, false, disable_ascii);
    }
    draw_car(curve, 20, player_hpos, true, disable_ascii); /* The player */
    mvprintw(6, 0, "Score: %d", cars_passed);
    mvprintw(7, 0, "%dkm/h", player_speed * 10);
    refresh();

    usleep(20000);

    /* Trajectory and road change: */
    curve = curve_ahead(player_speed);
  }

  endwin();
  fprintf(stdout, "Final score: %d\n", cars_passed);
  return EXIT_SUCCESS;
}

