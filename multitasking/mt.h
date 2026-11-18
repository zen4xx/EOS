#ifndef MT_H
#define MT_H

#include "../libc/stdint.h"
#include "../fs/fd.h"

extern volatile char _is_current_task_foreground;

int task_create(void (*entry)(int, char **), int argc, char** argv, char is_foreground, char is_user_space);
int task_create_ex(void (*entry)(int, char**), int argc, char** argv, uint64_t stack_size, char is_foreground, char is_user_space, vfs_file_t *fd0, vfs_file_t *fd1, vfs_file_t *fd2);

void start_first_task(void);
void yield(void);
void task_exit(void);
void task_reap(void);
int  task_finished(int id);
void task_wait(int id);

fd_table_t* get_current_fd_table();

uint64_t schedule_handler(uint64_t rsp);

#endif