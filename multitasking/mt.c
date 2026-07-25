#include "mt.h"
#include "../libc/stdlib.h"

#define TASK_STACK_SIZE 4096

static uint16_t current_pid = 0;
static task_t tasks[TASK_NUM];

void mt_init()
{
    for (int pid = 0; pid < TASK_NUM; ++pid)
    {
        task_t task;
        task.pid = pid;
        task.state = MT_FREE_STATE;

        tasks[pid] = task;
    }
}

uint16_t mt_create_task(void (*entry_point)()) {
    for (int pid = 0; pid < TASK_NUM; ++pid) {
        if (tasks[pid].state == MT_FREE_STATE) {
            tasks[pid].state = MT_RUNNING_STATE;
            
            uint8_t *stack = malloc(TASK_STACK_SIZE); 
            uint32_t *stack_top = (uint32_t*)(stack + TASK_STACK_SIZE);
            
            tasks[pid].jmp_buf.esp = (uint32_t)stack_top;
            tasks[pid].jmp_buf.eip = (uint32_t)entry_point;
            tasks[pid].jmp_buf.ebp = 0;
            tasks[pid].jmp_buf.ebx = 0;
            tasks[pid].jmp_buf.esi = 0;
            tasks[pid].jmp_buf.edi = 0;
            
            return tasks[pid].pid;
        }
    }
    return -1;
}

void mt_delete_task(uint16_t pid)
{
    tasks[pid].state = MT_FREE_STATE;
}

void mt_switch()
{
    uint16_t old_pid = current_pid;
    uint16_t next_pid = current_pid;
    do {
        next_pid = (next_pid + 1) % TASK_NUM;
    } while (tasks[next_pid].state != MT_RUNNING_STATE && next_pid != current_pid);
    
    if (next_pid == current_pid) return; 
    
    if (save_here(&tasks[old_pid].jmp_buf) == 0) {
        current_pid = next_pid;
        jump_to(&tasks[current_pid].jmp_buf);
    }
}

void mt_start()
{
    jump_to(&tasks[0].jmp_buf);
}