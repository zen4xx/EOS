#include "mt.h"
#include "../libc/stdlib.h"
#include "../libc/memory.h"
#include "../libc/string.h"
#include "../libc/stdint.h"
#include "../cpu/gdt.h"

#define MAX_TASKS        16
#define TASK_STACK_SIZE  4096
#define MIN_STACK_SIZE   1024

#define SCHED_INT        0x30

#define TASK_ALLOC(size) malloc(size)
#define TASK_FREE(ptr)   free(ptr)

enum {
    TASK_FREE = 0,
    TASK_READY,
    TASK_RUNNING,
    TASK_DEAD,
    TASK_BLOCKED
};

struct task {
    uint64_t kernel_rsp;
    uint64_t kernel_stack_top;
    int state;
    void *kernel_stack;
    void *user_stack;
    uint64_t stack_size;
    char is_fg;
    int argc;
    char **argv; // memory for arguments
};

struct irq_frame {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;  // offsets 0-56
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;     // offsets 64-112
    uint64_t rip, cs, rflags, rsp, ss;               // offsets 120-152
} __attribute__((packed));

static struct task tasks[MAX_TASKS];
static volatile int current_task = -1;
static volatile int num_fg_task = 0;
volatile int scheduler_enabled = 0;

extern void task_start(void);
extern void start_task(uint64_t rsp);

static inline uint64_t enter_critical(void)
{
    uint64_t flags;
    asm volatile(
        "pushfq\n\t"
        "cli\n\t"
        "popq %0"
        : "=r"(flags)
        :
        : "memory", "cc"
    );
    return flags;
}

static inline void exit_critical(uint64_t flags)
{
    asm volatile(
        "pushq %0\n\t"
        "popfq"
        :
        : "r"(flags)
        : "memory", "cc"
    );
}

static int pick_next(int cur)
{
    for (int i = 1; i <= MAX_TASKS; i++) {
        int idx = (cur + i) % MAX_TASKS;
        if (tasks[idx].state == TASK_READY) {
            return idx;
        }
    }
    return -1;
}

uint64_t schedule_handler(uint64_t rsp)
{
    if (!scheduler_enabled) {
        return rsp;
    }

    if (current_task >= 0) {
        tasks[current_task].kernel_rsp = rsp;

        if (tasks[current_task].state == TASK_RUNNING) {
            tasks[current_task].state = TASK_READY;
        }
    }

    int next = pick_next(current_task);

    if (next < 0) {
        if (current_task >= 0 && tasks[current_task].state != TASK_DEAD) {
            tasks[current_task].state = TASK_RUNNING;
            return tasks[current_task].kernel_rsp;
        }

        for (;;) {
            asm volatile("cli; hlt");
        }
    }

    current_task = next;
    tasks[current_task].state = TASK_RUNNING;

    tss_set_kernel_stack(tasks[current_task].kernel_stack_top);

    return tasks[current_task].kernel_rsp;
}

void yield(void)
{
    if (!scheduler_enabled) return;
    if (current_task < 0) return;

    asm volatile("int $0x30" ::: "memory");
}

void task_exit(void)
{
    asm volatile("cli" ::: "memory");

    if (current_task >= 0) {
        tasks[current_task].state = TASK_DEAD;
        if (tasks[current_task].is_fg == 1) {
            tasks[current_task].is_fg = 0;
            _is_current_task_foreground = 0;
            --num_fg_task;
        }
    }

    asm volatile("int $0x30" ::: "memory");

    for (;;) {
        asm volatile("cli; hlt");
    }
}

int task_create_ex(void (*entry)(int, char**), int argc, char** argv, uint64_t stack_size, char is_foreground, char is_user_space)
{
    if (entry == 0) return -1;

    if (stack_size < MIN_STACK_SIZE) {
        stack_size = MIN_STACK_SIZE;
    }

    uint64_t alloc_size = stack_size + 16;
    uint64_t flags = enter_critical();

    int i;
    for (i = 0; i < MAX_TASKS; i++) {
        if (tasks[i].state == TASK_FREE) break;
    }

    if (i == MAX_TASKS) {
        exit_critical(flags);
        return -1;
    }

    void *kstack = TASK_ALLOC(alloc_size);
    if (!kstack) {
        exit_critical(flags);
        return -2;
    }
    uint64_t ktop = ((uint64_t)kstack + stack_size) & ~15ULL;
    uint64_t utop = 0;
    void* ustack = NULL;
    if (is_user_space)
    {
        ustack = TASK_ALLOC(alloc_size);
        if (!ustack)
        {
            exit_critical(flags);
            return -2;
        }
        utop = ((uint64_t)ustack + stack_size) & ~15ULL;
    }

    struct irq_frame *f = (struct irq_frame *)(ktop - sizeof(struct irq_frame));
    memset(f, 0, sizeof(struct irq_frame));

    f->r12 = (uint64_t)entry;
    f->r13 = (uint64_t)argc;

    char **task_argv = NULL;

    if (argc > 0)
    {
        task_argv = malloc(argc * sizeof(char*));
        if (task_argv)
        {
            for (int i = 0; i < argc; ++i)
            {
                size_t len = strlen(argv[i]) + 1; // +1 for '\0'
                task_argv[i] = malloc(len);
                if (!task_argv[i])
                {
                    for (int j = 0; j < i; ++j) // delete everything that was just created
                    {
                        free(task_argv[j]);
                    }
                    free(task_argv);
                    task_argv = NULL;
                    goto argv_fail;
                }
                memcpy(task_argv[i], argv[i], len); // copying to new array
            }
        }
    }

argv_fail:



    f->r14 = (uint64_t)task_argv;

    f->rip = (uint64_t)task_start;
    
    if (is_user_space)
    {
        f->cs = USER_CS;
        f->ss = USER_SS;
        f->rsp = utop;
    }
    else {
        f->cs = KERNEL_CS;
        f->ss = KERNEL_DS;
        f->rsp = ktop;
    }
    
    f->rflags = 0x202; 
    

    tasks[i].kernel_rsp = (uint64_t)f;
    tasks[i].kernel_stack_top = ktop;
    tasks[i].state = TASK_READY;
    tasks[i].kernel_stack = kstack;
    tasks[i].user_stack = ustack;
    tasks[i].stack_size = alloc_size;
    tasks[i].argv = task_argv;
    tasks[i].argc = argc;

    if (num_fg_task == 0) {
        tasks[i].is_fg = is_foreground;
        if (is_foreground == 1) {
            _is_current_task_foreground = 1;
            ++num_fg_task;
        }
    } else {
        tasks[i].is_fg = 0;
    }
    exit_critical(flags);
    return i;
}

int task_create(void (*entry)(int, char**), int argc, char** argv, char is_foreground, char is_user_space)
{
    return task_create_ex(entry, argc, argv, TASK_STACK_SIZE, is_foreground, is_user_space);
}

void task_reap(void)
{
    uint64_t flags = enter_critical();

    for (int i = 0; i < MAX_TASKS; i++) {
        if (i == current_task) continue;
        
        if (tasks[i].state == TASK_DEAD) {
            if (tasks[i].kernel_stack) {
                TASK_FREE(tasks[i].kernel_stack);
                tasks[i].kernel_stack = 0;
            }
            if (tasks[i].user_stack) {
                TASK_FREE(tasks[i].user_stack);
                tasks[i].user_stack = 0;
            }
            tasks[i].stack_size = 0;
            tasks[i].kernel_rsp = 0;
            tasks[i].kernel_stack_top = 0;
            tasks[i].state = TASK_FREE;

            if (tasks[i].argv)
            {
                for (int j = 0; j < tasks[i].argc; ++j)
                {
                    free(tasks[i].argv[j]);
                }
                free(tasks[i].argv);
            }
        }
    }

    exit_critical(flags);
}


void start_first_task(void) {
    uint64_t flags = enter_critical();
    int first = pick_next(-1);

    if (first < 0) {
        exit_critical(flags);
        for (;;) {
            asm volatile("sti; hlt");
        }
    }

    current_task = first;
    tasks[first].state = TASK_RUNNING;
    scheduler_enabled = 1;

    start_task(tasks[first].kernel_rsp);
    __builtin_unreachable();
}