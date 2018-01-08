#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ncurses.h>

#define CODE_SLOTS 4
#define GUESS_SLOTS 10

typedef enum {
  CODE_NONE    = 0,
  CODE_ALPHA   = 1,
  CODE_BRAVO   = 2,
  CODE_CHARLIE = 3,
  CODE_DELTA   = 4,
  CODE_ECHO    = 5,
  CODE_FOXTROT = 6,
} code_t;

typedef enum {
  HINT_NONE = 0,
  HINT_CODE_AND_POSITION,
  HINT_CODE_ONLY,
} hint_t;

typedef struct guess_s {
  code_t code[CODE_SLOTS];
  hint_t hint[CODE_SLOTS];
} guess_t;

static const code_t code_to_char[] = {'.', 'A', 'B', 'C', 'D', 'E', 'F'};
static const hint_t hint_to_char[] = {' ', 'X', '/'};

static code_t hidden_code[CODE_SLOTS];
static guess_t guess[GUESS_SLOTS];
static int current_guess, current_slot;

static int guess_is_completed()
{
  int i;
  for (i = 0; i < CODE_SLOTS; i++) {
    if (guess[current_guess].code[i] == CODE_NONE)
      return 0;
  }
  return 1;
}

static int guess_is_correct()
{
  int i;
  for (i = 0; i < CODE_SLOTS; i++) {
    if (guess[current_guess].hint[i] != HINT_CODE_AND_POSITION)
      return 0;
  }
  return 1;
}

static void display_init(void)
{
  initscr();
  if (has_colors()) {
    start_color();
    init_pair(CODE_ALPHA,   COLOR_RED,     COLOR_BLACK);
    init_pair(CODE_BRAVO,   COLOR_GREEN,   COLOR_BLACK);
    init_pair(CODE_CHARLIE, COLOR_YELLOW,  COLOR_BLACK);
    init_pair(CODE_DELTA,   COLOR_BLUE,    COLOR_BLACK);
    init_pair(CODE_ECHO,    COLOR_MAGENTA, COLOR_BLACK);
    init_pair(CODE_FOXTROT, COLOR_CYAN,    COLOR_BLACK);
  }
  noecho();
  keypad(stdscr, TRUE);
}

static void display_update(char *end_msg)
{
  int i, j;

  erase();

  /* Draw area with hidden code. */
  mvaddstr(0, 0, "+---+---+---+---+");
  if (end_msg != NULL) {
    mvaddstr(1, 0, "|   |   |   |   |");
    for (i = 0; i < CODE_SLOTS; i++) {
      move(1, 2 + (i * 4));
      if (has_colors() && hidden_code[i] != CODE_NONE)
        attrset(A_BOLD | COLOR_PAIR(hidden_code[i]));
      addch(code_to_char[hidden_code[i]]);
      if (has_colors() && hidden_code[i] != CODE_NONE)
        attrset(A_NORMAL);
    }
    mvaddstr(1, 18, end_msg);
  } else {
    mvaddstr(1, 0, "|###|###|###|###|");
  }
  mvaddstr(2, 0, "+---+---+---+---+----+");

  /* Draw area with user guesses. */
  for (i = 0; i < GUESS_SLOTS; i++) {
    mvaddstr(3 + (i * 2), 0, "|   |   |   |   |    |");

    for (j = 0; j < CODE_SLOTS; j++) {
      move(3 + (i * 2), 2 + (j * 4));
      if (has_colors() && guess[i].code[j] != CODE_NONE)
        attrset(A_BOLD | COLOR_PAIR(guess[i].code[j]));
      addch(code_to_char[guess[i].code[j]]);
      if (has_colors() && guess[i].code[j] != CODE_NONE)
        attrset(A_NORMAL);
    }

    for (j = 0; j < CODE_SLOTS; j++) {
      move(3 + (i * 2), 17 + j);
      if (has_colors() && guess[i].hint[j] != HINT_NONE)
        attrset(A_BOLD);
      addch(hint_to_char[guess[i].hint[j]]);
      if (has_colors() && guess[i].hint[j] != HINT_NONE)
        attrset(A_NORMAL);
    }

    mvaddstr(4 + (i * 2), 0, "+---+---+---+---+----+");
  }

  /* Place cursor on current slot to select. */
  move(3 + (current_guess * 2), 2 + (current_slot * 4));

  refresh();
}

static void evaluate_and_give_hints(void)
{
  int i, j, code_only = 0, code_and_pos = 0;
  int guess_covered[CODE_SLOTS], hidden_covered[CODE_SLOTS];

  /* Check for correct code and position. */
  for (i = 0; i < CODE_SLOTS; i++) {
    if (guess[current_guess].code[i] == hidden_code[i]) {
      code_and_pos++;
      guess_covered[i] = 1;
      hidden_covered[i] = 1;
    } else {
      guess_covered[i] = 0;
      hidden_covered[i] = 0;
    }
  }

  /* Check for any remaining correct codes. */
  for (i = 0; i < CODE_SLOTS; i++) {
    if (guess_covered[i])
      continue;
    for (j = 0; j < CODE_SLOTS; j++) {
      if (hidden_covered[j])
        continue;
      if (guess[current_guess].code[i] == hidden_code[j]) {
        hidden_covered[j] = 1;
        code_only++;
        break;
      }
    }
  }

  i = 0;
  while (code_and_pos-- > 0)
    guess[current_guess].hint[i++] = HINT_CODE_AND_POSITION;
  while (code_only-- > 0)
    guess[current_guess].hint[i++] = HINT_CODE_ONLY;
}

static void hidden_code_randomize(void)
{
  int i;
  srandom(time(NULL));
  for (i = 0; i < CODE_SLOTS; i++)
    hidden_code[i] = (random() % CODE_FOXTROT) + 1;
}

int main(void)
{
  int user_done, user_quit;

  memset(guess, 0, sizeof(guess_t) * GUESS_SLOTS);
  display_init();
  hidden_code_randomize();

  for (current_guess = GUESS_SLOTS - 1;
       current_guess >= 0;
       current_guess--) {

    /* Wait for user to make a valid guess. */
    current_slot = user_done = user_quit = 0;
    while ((! user_done) && (! user_quit)) {
      display_update(NULL);

      switch (getch()) {
      case KEY_LEFT:
        current_slot--;
        if (current_slot < 0)
          current_slot = 0;
        break;

      case KEY_RIGHT:
        current_slot++;
        if (current_slot >= CODE_SLOTS)
          current_slot = CODE_SLOTS - 1;
        break;

      case KEY_UP:
        guess[current_guess].code[current_slot]++;
        if (guess[current_guess].code[current_slot] > CODE_FOXTROT)
          guess[current_guess].code[current_slot] = CODE_ALPHA;
        break;

      case KEY_DOWN:
        guess[current_guess].code[current_slot]--;
        if (guess[current_guess].code[current_slot] == CODE_NONE ||
            guess[current_guess].code[current_slot] > CODE_FOXTROT)
          guess[current_guess].code[current_slot] = CODE_FOXTROT;
        break;

      case KEY_ENTER:
      case '\n':
      case ' ':
        if (guess_is_completed())
          user_done = 1;
        break;

      case 'q':
      case 'Q':
      case '\e':
        user_quit = 1;
        break;

      default:
        break;
      }
    }

    if (user_quit)
      break;

    evaluate_and_give_hints();

    if (guess_is_correct())
      break;
  }

  if (guess_is_correct()) {
    display_update("Congratulations!");
  } else {
    if (user_quit) {
      display_update("Aborted.");
    } else {
      display_update("Too late!");
    }
  }

  if (! user_quit)
    getch(); /* Press any key... */

  endwin();
  return 0;
}

