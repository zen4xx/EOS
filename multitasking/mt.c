#include "mt.h"
#include "../libc/stdlib.h"
#include "../libc/memory.h"
#include "../libc/stdint.h"

#define MAX_TASKS        16
#define TASK_STACK_SIZE  4096
#define MIN_STACK_SIZE   1024

#define KERNEL_CS        0x18
#define KERNEL_DS        0x10

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
    uint64_t rsp;        
    int state;
    void *stack;
    uint64_t stack_size;
    char is_fg;
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
        tasks[current_task].rsp = rsp;

        if (tasks[current_task].state == TASK_RUNNING) {
            tasks[current_task].state = TASK_READY;
        }
    }

    int next = pick_next(current_task);

    if (next < 0) {
        if (current_task >= 0 && tasks[current_task].state != TASK_DEAD) {
            tasks[current_task].state = TASK_RUNNING;
            return tasks[current_task].rsp;
        }

        for (;;) {
            asm volatile("cli; hlt");
        }
    }

    current_task = next;
    tasks[current_task].state = TASK_RUNNING;

    return tasks[current_task].rsp;
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

int task_create_ex(void (*entry)(void *), void *arg, uint64_t stack_size, char is_foreground)
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

    void *stack = TASK_ALLOC(alloc_size);
    if (stack == 0) {
        exit_critical(flags);
        return -1;
    }

    uint64_t base = (uint64_t)stack;
    uint64_t top = base + alloc_size;
    
    /* 16-byte stack alignment is required in 64-bit mode */
    top &= ~15ULL;

    if ((top - base) < (sizeof(struct irq_frame) + 64)) {
        TASK_FREE(stack);
        exit_critical(flags);
        return -1;
    }

    struct irq_frame *f = (struct irq_frame *)(top - sizeof(struct irq_frame));
    memset(f, 0, sizeof(struct irq_frame));

    f->r12 = (uint64_t)entry;
    f->r13 = (uint64_t)arg;

    f->rip = (uint64_t)task_start;
    f->cs = KERNEL_CS;
    f->rflags = 0x202; 

    f->rsp = top;
    f->ss = KERNEL_DS;

    tasks[i].rsp = (uint64_t)f;
    tasks[i].state = TASK_READY;
    tasks[i].stack = stack;
    tasks[i].stack_size = alloc_size;

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

int task_create(void (*entry)(void *), void *arg, char is_foreground)
{
    return task_create_ex(entry, arg, TASK_STACK_SIZE, is_foreground);
}

void task_reap(void)
{
    uint64_t flags = enter_critical();

    for (int i = 0; i < MAX_TASKS; i++) {
        if (i == current_task) continue;

        if (tasks[i].state == TASK_DEAD && tasks[i].stack != 0) {
            TASK_FREE(tasks[i].stack);
            tasks[i].stack = 0;
            tasks[i].stack_size = 0;
            tasks[i].rsp = 0;
            tasks[i].state = TASK_FREE;
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

    start_task(tasks[first].rsp);
    __builtin_unreachable();
}