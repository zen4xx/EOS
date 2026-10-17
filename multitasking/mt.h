#ifndef MT_H
#define MT_H

#include "../libc/stdint.h"

extern volatile char _is_current_task_foreground;

int task_create(void (*entry)(int, char **), int argc, char** argv, char is_foreground, char is_user_space);
int task_create_ex(void (*entry)(int, char **), int argc, char** argv, uint64_t stack_size, char is_foreground, char is_user_space);

void start_first_task(void);
void yield(void);
void task_exit(void);
void task_reap(void);

uint64_t schedule_handler(uint64_t rsp);

#endif