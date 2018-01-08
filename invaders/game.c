#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include "game.h"



game_state_t game_state; /* Global instance. */



void game_init(void)
{
  int i, j;

  srand(time(NULL));

  game_state.score = 0;
  game_state.wasted_shots = 0;
  game_state.player_pos = GAME_MAP_WIDTH / 2;
  game_state.invaders_pos_x = game_state.invaders_pos_y = 0;
  game_state.invaders_moving_right = 1;
  game_state.shot_pos_x = game_state.shot_pos_y = -1;
  game_state.bomb_pos_x = game_state.bomb_pos_y = -1;

  for (i = 0; i < GAME_INVADERS; i++) {
    game_state.invader_alive[i] = true;
  }

  for (i = 0; i < GAME_SHIELDS; i++) {
    for (j = 0; j < GAME_SHIELD_PARTS; j++) {
      game_state.shield_intact[i][j] = true;
    }
  }
}



int game_invaders_alive(void)
{
  int i, n = 0;
  for (i = 0; i < GAME_INVADERS; i++) {
    if (game_state.invader_alive[i])
      n++;
  }
  return n;
}



void game_fire_shot(void)
{
  if (game_state.shot_pos_y >= 0)
    return; /* A shot is already being fired. */
    
  game_state.shot_pos_y = GAME_MAP_HEIGHT - 1;
  game_state.shot_pos_x = game_state.player_pos;
}



int game_handle_invaders(long cycle)
{
  int alive, i, j, k, x, y, shield_x;

  alive = game_invaders_alive();
  if (alive == 0)
    return 0;
  if (cycle % alive != 0)
    return 0;

  if (game_state.invaders_moving_right) {
    game_state.invaders_pos_x++;
    if (game_state.invaders_pos_x > (GAME_MAP_WIDTH - 68)) { /* (10*5)+(9*2) */
      game_state.invaders_pos_x--;
      game_state.invaders_pos_y++;
      game_state.invaders_moving_right = 0;
    }
  } else {
    game_state.invaders_pos_x--;
    if (game_state.invaders_pos_x < 0) {
      game_state.invaders_pos_x = 0;
      game_state.invaders_pos_y++;
      game_state.invaders_moving_right = 1;
    }
  }

  /* Check if invaders have reached the player or shields. */
  for (i = 0; i < GAME_INVADERS; i++) {
    if (game_state.invader_alive[i]) {
      y = game_state.invaders_pos_y + ((i / 10) * 4) + 2;

      if (y == (GAME_MAP_HEIGHT - 4)) {
        x = game_state.invaders_pos_x + ((i % 10) * 7);
        for (j = 0; j < (GAME_SHIELDS / 2); j++) {
          for (k = 0; k < GAME_SHIELD_PARTS; k++) {
            shield_x = 12 + ((j % 4) * 15) + k;
            if ((shield_x >= x) && (shield_x <= x + 4))
              game_state.shield_intact[j][k] = false;
          }
        }
      } else if (y == (GAME_MAP_HEIGHT - 5)) {
        x = game_state.invaders_pos_x + ((i % 10) * 7);
        for (j = GAME_SHIELDS / 2; j < GAME_SHIELDS; j++) {
          for (k = 0; k < GAME_SHIELD_PARTS; k++) {
            shield_x = 12 + ((j % 4) * 15) + k;
            if ((shield_x >= x) && (shield_x <= x + 4))
              game_state.shield_intact[j][k] = false;
          }
        }
      } else if (y >= (GAME_MAP_HEIGHT - 1)) {
        game_state.score -= 1000;
        return 1;
      }
    }
  }

  return 0;
}



int game_handle_shot(void)
{
  int i, j, x, y;

  if (game_state.shot_pos_y < 0)
    return 0; /* No shot is being fired. */

  game_state.shot_pos_y--;
  if (game_state.shot_pos_y < 0) { /* Out of range. */
    game_state.wasted_shots++;
    game_state.score -= 50;
    return 0;
  }

  /* Check for collision with invaders. */
  for (i = 0; i < GAME_INVADERS; i++) {
    if (game_state.invader_alive[i]) {
      y = game_state.invaders_pos_y + ((i / 10) * 4) + 2;
      x = game_state.invaders_pos_x + ((i % 10) * 7);
      if (game_state.shot_pos_y == y) {
        if ((game_state.shot_pos_x >= x) && 
            (game_state.shot_pos_x <= x + 4)) {
          game_state.invader_alive[i] = false;
          game_state.shot_pos_y = -1;
          game_state.score += 100;
          return 1;
        }
      }
    }
  }

  /* Check for collision with shields. */
  for (i = 0; i < GAME_SHIELDS; i++) {
    for (j = 0; j < GAME_SHIELD_PARTS; j++) {
      if (game_state.shield_intact[i][j]) {
        y = (i / 4 == 0) ? GAME_MAP_HEIGHT - 4 : GAME_MAP_HEIGHT - 5;
        x = 12 + ((i % 4) * 15) + j;
        if ((game_state.shot_pos_y == y) &&
            (game_state.shot_pos_x == x)) {
          game_state.shield_intact[i][j] = false;
          game_state.shot_pos_y = -1;
          game_state.wasted_shots++;
          game_state.score -= 10;
        }
      }
    }
  }

  return 0;
}



int game_handle_bombs(void)
{
  int i, j, x, y, invader;

  if (game_state.bomb_pos_y >= 0) { /* Bomb is being dropped. */
    game_state.bomb_pos_y++;

    if (game_state.bomb_pos_y > GAME_MAP_HEIGHT) { /* Out of range. */
      game_state.bomb_pos_y = -1;
      return 0;
    }

    /* Check for collision against player. */
    if (game_state.bomb_pos_y == GAME_MAP_HEIGHT) {
      if ((game_state.bomb_pos_x >= game_state.player_pos - 2) && 
          (game_state.bomb_pos_x <= game_state.player_pos + 2)) {
        game_state.score -= 500;
        return 1;
      }
    }

    /* Check for collision with shields. */
    for (i = 0; i < GAME_SHIELDS; i++) {
      for (j = 0; j < GAME_SHIELD_PARTS; j++) {
        if (game_state.shield_intact[i][j]) {
          y = (i / 4 == 0) ? GAME_MAP_HEIGHT - 4 : GAME_MAP_HEIGHT - 5;
          x = 12 + ((i % 4) * 15) + j;
          if ((game_state.bomb_pos_y == y) &&
              (game_state.bomb_pos_x == x)) {
            game_state.shield_intact[i][j] = false;
            game_state.bomb_pos_y = -1;
          }
        }
      }
    }

  } else { /* Check if a new bomb should be dropped. */

    if (game_invaders_alive() == 0)
      return 0;

    /* Pick a random invader that's still alive. */
    do {
      invader = rand() % GAME_INVADERS;
    } while (! game_state.invader_alive[invader]);

    /* Check if anyone below that invader is still alive. */
    if (invader < 10) {
      if (game_state.invader_alive[invader + 20])
        invader += 20;
      else if (game_state.invader_alive[invader + 10])
        invader += 10;
    } else if (invader < 20) {
      if (game_state.invader_alive[invader + 10])
        invader += 10;
    }

    game_state.bomb_pos_y = game_state.invaders_pos_y + 
      ((invader / 10) * 4) + 2;
    game_state.bomb_pos_x = game_state.invaders_pos_x + 
      ((invader % 10) * 7) + 2;
  }

  return 0;
}

