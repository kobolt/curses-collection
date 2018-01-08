#ifndef _GAME_H
#define _GAME_H

#include <stdbool.h>

#define GAME_MAP_HEIGHT 24
#define GAME_MAP_WIDTH 80

#define GAME_INVADERS 30
#define GAME_SHIELDS 8
#define GAME_SHIELD_PARTS 10

typedef struct game_state_s {
  long score;
  short wasted_shots;
  short player_pos; /* Only in X plane. */
  short invaders_pos_y, invaders_pos_x; /* Cluster position. */
  short invaders_moving_right;
  short shot_pos_y, shot_pos_x;
  short bomb_pos_y, bomb_pos_x;
  bool invader_alive[GAME_INVADERS];
  bool shield_intact[GAME_SHIELDS][GAME_SHIELD_PARTS];
} game_state_t;

extern game_state_t game_state;

void game_init(void);
int game_invaders_alive(void);
void game_fire_shot(void);
int game_handle_invaders(long cycle);
int game_handle_shot(void);
int game_handle_bombs(void);

#endif /* _GAME_H */
