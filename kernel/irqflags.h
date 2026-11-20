#ifndef IRQFLAGS_H
#define IRQFLAGS_H

#include "../cpu/types.h"

#ifdef EOS_HOST_TEST
static inline u64  irq_save(void)         { return 0; }
static inline void irq_restore(u64 flags) { (void)flags; }
#else
static inline u64 irq_save(void) {
    u64 flags;
    asm volatile("pushfq\n\tcli\n\tpopq %0" : "=r"(flags) :: "memory", "cc");
    return flags;
}
static inline void irq_restore(u64 flags) {
    asm volatile("pushq %0\n\tpopfq" :: "r"(flags) : "memory", "cc");
}
#endif

#endif /* IRQFLAGS_H */
