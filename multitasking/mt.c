#include "../libc/stdlib.h"
#include "../libc/stdint.h"

#define MAX_TASKS        16
#define TASK_STACK_SIZE  4096
#define MIN_STACK_SIZE   1024

#define KERNEL_CS        0x08
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
    uint32_t esp;
    int state;
    void *stack;
    uint32_t stack_size;
};
struct irq_frame {
    uint32_t gs, fs, es, ds;

    /* pushad: low -> high */
    uint32_t edi, esi, ebp, esp_unused, ebx, edx, ecx, eax;

    /* CPU pushed */
    uint32_t eip, cs, eflags;
} __attribute__((packed));

static struct task tasks[MAX_TASKS];
static volatile int current_task = -1;
volatile int scheduler_enabled = 0;

extern void task_start(void);
extern void start_task(uint32_t esp);

static void zero32(uint32_t *ptr, uint32_t count)
{
    for (uint32_t i = 0; i < count; i++) {
        ptr[i] = 0;
    }
}

static inline uint32_t enter_critical(void)
{
    uint32_t flags;

    asm volatile(
        "pushfl\n\t"
        "cli\n\t"
        "popl %0"
        : "=r"(flags)
        :
        : "memory", "cc"
    );

    return flags;
}

static inline void exit_critical(uint32_t flags)
{
    asm volatile(
        "pushl %0\n\t"
        "popfl"
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

uint32_t schedule_handler(uint32_t esp)
{
    if (!scheduler_enabled) {
        return esp;
    }

    if (current_task >= 0) {
        tasks[current_task].esp = esp;

        if (tasks[current_task].state == TASK_RUNNING) {
            tasks[current_task].state = TASK_READY;
        }
    }

    int next = pick_next(current_task);

    if (next < 0) {
        if (current_task >= 0 && tasks[current_task].state != TASK_DEAD) {
            tasks[current_task].state = TASK_RUNNING;
            return tasks[current_task].esp;
        }

        for (;;) {
            asm volatile("cli; hlt");
        }
    }

    current_task = next;
    tasks[current_task].state = TASK_RUNNING;

    return tasks[current_task].esp;
}

void yield(void)
{
    if (!scheduler_enabled) {
        return;
    }

    if (current_task < 0) {
        return;
    }

    asm volatile(
        "int $0x30"
        :
        :
        : "memory"
    );
}

void task_exit(void)
{
    asm volatile("cli" ::: "memory");

    if (current_task >= 0) {
        tasks[current_task].state = TASK_DEAD;
    }

    asm volatile(
        "int $0x30"
        :
        :
        : "memory"
    );

    for (;;) {
        asm volatile("cli; hlt");
    }
}

int task_create_ex(void (*entry)(void *), void *arg, uint32_t stack_size)
{
    if (entry == 0) {
        return -1;
    }

    if (stack_size < MIN_STACK_SIZE) {
        stack_size = MIN_STACK_SIZE;
    }

    uint32_t alloc_size = stack_size + 16;

    uint32_t flags = enter_critical();

    int i;
    for (i = 0; i < MAX_TASKS; i++) {
        if (tasks[i].state == TASK_FREE) {
            break;
        }
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

    uint32_t base = (uint32_t)stack;
    uint32_t top = base + alloc_size;

    top &= ~15u;

    if ((top - base) < (sizeof(struct irq_frame) + 64)) {
        TASK_FREE(stack);
        exit_critical(flags);
        return -1;
    }

    struct irq_frame *f =
        (struct irq_frame *)(top - sizeof(struct irq_frame));

    zero32((uint32_t *)f, (uint32_t)(sizeof(struct irq_frame) / sizeof(uint32_t)));

    f->gs = KERNEL_DS;
    f->fs = KERNEL_DS;
    f->es = KERNEL_DS;
    f->ds = KERNEL_DS;
    /*
     *   ebx = entry
     *   ecx = arg
     */
    f->edi = 0;
    f->esi = 0;
    f->ebp = 0;
    f->ebx = (uint32_t)entry;
    f->edx = 0;
    f->ecx = (uint32_t)arg;
    f->eax = 0;
    f->esp_unused = (uint32_t)(f + 1);
    /*
     * 0x202:
     *   bit 1 = 1
     *   IF    = 1
     */
    f->eip = (uint32_t)task_start;
    f->cs = KERNEL_CS;
    f->eflags = 0x202;

    tasks[i].esp = (uint32_t)f;
    tasks[i].state = TASK_READY;
    tasks[i].stack = stack;
    tasks[i].stack_size = alloc_size;

    exit_critical(flags);

    return i;
}

int task_create(void (*entry)(void *), void *arg)
{
    return task_create_ex(entry, arg, TASK_STACK_SIZE);
}

void task_reap(void)
{
    uint32_t flags = enter_critical();

    for (int i = 0; i < MAX_TASKS; i++) {
        if (i == current_task) {
            continue;
        }

        if (tasks[i].state == TASK_DEAD && tasks[i].stack != 0) {
            TASK_FREE(tasks[i].stack);

            tasks[i].stack = 0;
            tasks[i].stack_size = 0;
            tasks[i].esp = 0;
            tasks[i].state = TASK_FREE;
        }
    }

    exit_critical(flags);
}

void start_first_task(void)
{
    uint32_t flags = enter_critical();

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

    start_task(tasks[first].esp);

    __builtin_unreachable();
}