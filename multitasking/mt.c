#include "mt.h"

static uint16_t current_pid = 0;
// static jmp_buf_t mt_jmp_buf;


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

uint16_t mt_create_task(void (*entry_point)())
{
    for (int pid = 0; pid < TASK_NUM; ++pid)
    {
        if (tasks[pid].state == MT_FREE_STATE)
        {
            tasks[pid].state = MT_RUNNING_STATE;
            save_here(&tasks[pid].jmp_buf);
            tasks[pid].jmp_buf.eip = (uint32_t)entry_point;

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
    if (current_pid == TASK_NUM) current_pid = 0;

    for (int pid = 0; pid < TASK_NUM; ++pid)
    {
        if (tasks[pid].state == MT_RUNNING_STATE)
        {
            // save_here(&mt_jmp_buf); TODO
        }
    }
}