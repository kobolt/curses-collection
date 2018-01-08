#ifndef _IFACE_H
#define _IFACE_H

#include "game.h"

typedef enum {
  ACTION_NONE,
  ACTION_EXIT,
  ACTION_RESIZE,
  ACTION_MOVE_LEFT,
  ACTION_MOVE_RIGHT,
  ACTION_SHOOT,
} iface_action_t;

void iface_init(void);
void iface_set_end_msg(char *msg);
iface_action_t iface_get_action(void);
void iface_redraw_screen(void);
void iface_flash_screen(void);
int iface_invalid_size(void);

#endif /* _IFACE_H */
