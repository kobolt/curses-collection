#include <string.h>
#include <unistd.h>
#include "game.h"
#include "iface.h"



void loop(void)
{
  long cycle;
  iface_action_t action;
  
  game_init();
  iface_init();

  iface_redraw_screen();
  for (cycle = 0;; cycle++) {

    action = iface_get_action();

    /* Perform player action. */
    switch (action) {
    case ACTION_EXIT:
      iface_set_end_msg("Player gave up!");
      return;

    case ACTION_RESIZE:
      if (iface_invalid_size()) {
        iface_set_end_msg("Invalid resize of terminal!");
        return;
      }
      break;

    case ACTION_SHOOT:
      game_fire_shot();
      break;

    case ACTION_MOVE_LEFT:
      game_state.player_pos--;
      if (game_state.player_pos < 2)
        game_state.player_pos = 2;
      break;

    case ACTION_MOVE_RIGHT:
      game_state.player_pos++;
      if (game_state.player_pos > (GAME_MAP_WIDTH - 3))
        game_state.player_pos = GAME_MAP_WIDTH - 3;
      break;

    case ACTION_NONE:
      break;
    }

    if (game_handle_invaders(cycle)) {
      iface_flash_screen();
      iface_set_end_msg("Player could not stop the invaders in time!");
      return;
    }

    if (game_handle_shot())
      iface_flash_screen();

    if (game_handle_bombs()) {
      iface_flash_screen();
      iface_set_end_msg("Player died from an invader's bomb!");
      return;
    }

    if (game_invaders_alive() == 0)
      break; /* Won. */

    iface_redraw_screen();

    usleep(50000);
  }

  iface_set_end_msg("Success! Invader attack has been stopped!");
  return;
}



int main(void)
{
  loop();
  return 0;
}

