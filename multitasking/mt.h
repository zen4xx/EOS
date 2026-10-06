#ifndef MT_H
#define MT_H

#include "../libc/stdint.h"

extern volatile char _is_current_task_foreground;

int task_create(void (*entry)(void *), void *arg, char is_foreground);
int task_create_ex(void (*entry)(void *), void *arg, uint64_t stack_size, char is_foreground);

void start_first_task(void);
void yield(void);
void task_exit(void);
void task_reap(void);

uint64_t schedule_handler(uint64_t rsp);

#endif