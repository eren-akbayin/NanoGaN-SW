#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Creates the task. It blocks immediately and waits to be woken up. */
void SuperMarioTask_Init(void);

void SuperMarioTask_Wake(void);

#ifdef __cplusplus
}
#endif
