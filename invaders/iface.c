#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <curses.h>
#include "iface.h"
#include "game.h"



#define INVADER_1_S1_R1 " /-\\ "
#define INVADER_1_S1_R2 "/x x\\"
#define INVADER_1_S1_R3 "/\\|/\\"

#define INVADER_1_S2_R1 " /-\\ "
#define INVADER_1_S2_R2 "/x x\\"
#define INVADER_1_S2_R3 "\\/|\\/"
 
#define INVADER_2_S1_R1 "\\___/"
#define INVADER_2_S1_R2 "|o o|"
#define INVADER_2_S1_R3 "\\---/"

#define INVADER_2_S2_R1 "/___\\"
#define INVADER_2_S2_R2 "|o o|"
#define INVADER_2_S2_R3 "\\---/"

#define INVADER_3_S1_R1 " ___ "
#define INVADER_3_S1_R2 "(- -)"
#define INVADER_3_S1_R3 "()-()"

#define INVADER_3_S2_R1 " ___ "
#define INVADER_3_S2_R2 "(o o)"
#define INVADER_3_S2_R3 "()-()"

#define PLAYER_R1 " _|_"
#define PLAYER_R2 "/___\\"

#define IFACE_END_MSG_MAX 80



static char iface_end_msg[IFACE_END_MSG_MAX];



static void iface_exit_handler(void)
{
  char *rating;

  if (game_state.score >= 3000)
    rating = "Perfect";
  else if (game_state.score > 2500)
    rating = "Very Good";
  else if (game_state.score > 2000)
    rating = "Good";
  else if (game_state.score > 1000)
    rating = "Mediocre";
  else if (game_state.score > 0)
    rating = "Poor";
  else
    rating = "Horrible";

  endwin();
  printf("\n%s\n", iface_end_msg);
  printf("%d invaders survived, and %d shots were wasted.\n",
    game_invaders_alive(), game_state.wasted_shots);
  printf("Score: %ld (%s)\n", game_state.score, rating);
}



void iface_init(void)
{
  initscr();
  atexit(iface_exit_handler);
  noecho();
  keypad(stdscr, TRUE);
  timeout(0); /* Non-blocking mode */

  if (iface_invalid_size()) {
    strncpy(iface_end_msg, 
      "Invalid size of terminal. (Must be atleast 80x24.)", IFACE_END_MSG_MAX);
    exit(0);
  }

  strncpy(iface_end_msg, "Abnormal termination.", IFACE_END_MSG_MAX);
}



void iface_set_end_msg(char *msg)
{
  strncpy(iface_end_msg, msg, IFACE_END_MSG_MAX);
}



iface_action_t iface_get_action(void)
{
  int c;
  iface_action_t action;

  action = ACTION_NONE; /* Default. */

  /* Read character buffer until empty. */
  while ((c = getch()) != ERR) {
    switch (c) {
    case KEY_RESIZE:
      return ACTION_RESIZE;
    case ' ':
    case KEY_UP:
      action = ACTION_SHOOT;
      break;
    case KEY_LEFT:
      action = ACTION_MOVE_LEFT;
      break;
    case KEY_RIGHT:
      action = ACTION_MOVE_RIGHT;
      break;
    case 'q':
    case 'Q':
    case '\e':
      return ACTION_EXIT;
    default:
      break;
    }
  }

  return action;
}



void iface_redraw_screen(void)
{
  int i, j, x, y;
  int maxy, maxx;

  erase();

  /* Draw invaders. */
  for (i = 0; i < GAME_INVADERS; i++) {
    if (game_state.invader_alive[i]) {
      y = game_state.invaders_pos_y + ((i / 10) * 4);
      x = game_state.invaders_pos_x + ((i % 10) * 7);
      if (i / 10 == 0) {
        if (game_state.invaders_pos_x % 2 == 0) {
          mvaddstr(y,     x, INVADER_1_S1_R1);
          mvaddstr(y + 1, x, INVADER_1_S1_R2);
          mvaddstr(y + 2, x, INVADER_1_S1_R3);
        } else {
          mvaddstr(y,     x, INVADER_1_S2_R1);
          mvaddstr(y + 1, x, INVADER_1_S2_R2);
          mvaddstr(y + 2, x, INVADER_1_S2_R3);
        }
      } else if (i / 10 == 1) {
        if (game_state.invaders_pos_x % 2 == 0) {
          mvaddstr(y,     x, INVADER_2_S1_R1);
          mvaddstr(y + 1, x, INVADER_2_S1_R2);
          mvaddstr(y + 2, x, INVADER_2_S1_R3);
        } else {
          mvaddstr(y,     x, INVADER_2_S2_R1);
          mvaddstr(y + 1, x, INVADER_2_S2_R2);
          mvaddstr(y + 2, x, INVADER_2_S2_R3);
        }
      } else {
        if (game_state.invaders_pos_x % 2 == 0) {
          mvaddstr(y,     x, INVADER_3_S1_R1);
          mvaddstr(y + 1, x, INVADER_3_S1_R2);
          mvaddstr(y + 2, x, INVADER_3_S1_R3);
        } else {
          mvaddstr(y,     x, INVADER_3_S2_R1);
          mvaddstr(y + 1, x, INVADER_3_S2_R2);
          mvaddstr(y + 2, x, INVADER_3_S2_R3);
        }
      }
    }
  }

  /* Draw shields. */
  for (i = 0; i < GAME_SHIELDS; i++) {
    for (j = 0; j < GAME_SHIELD_PARTS; j++) {
      if (game_state.shield_intact[i][j]) {
        y = (i / 4 == 0) ? GAME_MAP_HEIGHT - 4 : GAME_MAP_HEIGHT - 5;
        x = 12 + ((i % 4) * 15) + j;
        mvaddch(y, x, '#');
      }
    }
  }

  /* Draw shots and bombs. */
  if (game_state.shot_pos_y >= 0)
    mvaddch(game_state.shot_pos_y, game_state.shot_pos_x, '|');
  if (game_state.bomb_pos_y >= 0)
    mvaddch(game_state.bomb_pos_y, game_state.bomb_pos_x, '*');

  /* Draw player. */
  mvaddstr(GAME_MAP_HEIGHT - 2, game_state.player_pos - 2, PLAYER_R1);
  mvaddstr(GAME_MAP_HEIGHT - 1, game_state.player_pos - 2, PLAYER_R2);

  /* Draw borders. (Only if terminal size is too large.) */
  getmaxyx(stdscr, maxy, maxx);
  if (maxy > GAME_MAP_HEIGHT)
    mvhline(GAME_MAP_HEIGHT, 0, 0, GAME_MAP_WIDTH + 1);
  if (maxx > GAME_MAP_WIDTH)
    mvvline(0, GAME_MAP_WIDTH, 0, GAME_MAP_HEIGHT);

  /* Position cursor on player. */
  move(GAME_MAP_HEIGHT - 2, game_state.player_pos);

  refresh();
}



void iface_flash_screen(void)
{
  flash();
}



int iface_invalid_size(void)
{
  int maxy, maxx;

  getmaxyx(stdscr, maxy, maxx);

  if (maxy < GAME_MAP_HEIGHT)
    return 1;
  else if (maxx < GAME_MAP_WIDTH)
    return 1;
  else
    return 0;
}

