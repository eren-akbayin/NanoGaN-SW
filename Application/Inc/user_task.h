#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Creates the task. It blocks immediately and waits to be woken up. */
void MainAppTask_Init(void);

/* Wakes the task up to do its work once. Call from the task that owns it
   (e.g. the main/default task). The task deletes itself when done. */
void MainAppTask_Wake(void);

extern uint8_t uSuperMario;

/* 0 = Super Mario Bros., 1 = Tetris Theme A, 2 = Imperial March, 3 = Pac-Man,
   4 = Indiana Jones */
extern uint8_t uSongSelect;

#ifdef __cplusplus
}
#endif
