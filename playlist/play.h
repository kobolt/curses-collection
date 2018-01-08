#ifndef _PLAY_H
#define _PLAY_H

void play_finished_handler_install(void (*handler)(void));
void play_set_program(char *program);
void play_cancel(void);
void play_execute(char *arg);
void play_block(void);
void play_unblock(void);

#endif /* _PLAY_H */
