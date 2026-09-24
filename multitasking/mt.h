#ifndef MT_H
#define MT_H

#include "../libc/stdint.h"

int task_create(void (*entry)(void *), void *arg);
int task_create_ex(void (*entry)(void *), void *arg, uint32_t stack_size);

void start_first_task(void);
void yield(void);
void task_exit(void);
void task_reap(void);

uint32_t schedule_handler(uint32_t esp);


#endif