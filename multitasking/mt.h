#ifndef MT_H
#define MT_H

#define TASK_NUM 10

#include "../libc/stdint.h"
#include "../libc/jmp.h"

#define MT_FREE_STATE 0
#define MT_RUNNING_STATE 1

typedef struct
{
    char state;
    uint16_t pid;
    jmp_buf_t jmp_buf;    
} task_t;

void mt_init();
uint16_t mt_create_task(void (*entry_point)()); // returns pid
void mt_delete_task(uint16_t pid);
void mt_switch();
void mt_start();


#endif