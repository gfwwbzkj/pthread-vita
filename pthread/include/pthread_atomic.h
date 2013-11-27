//Sony Computer Entertainment Confidential

#ifndef _H_pthread_atomic_psp
#define _H_pthread_atomic_psp

#include <stdint.h>

/* Atomic operations */

// Lifted from sceaatomic/include/sceaatomic.h

static inline long ATOMIC_CAS(volatile long* ptr, long compare, long swap)
{
    int32_t old;
    do
    {
        old = __ldrex((volatile int*)ptr);
        if ( old != compare )
            break;
    } while ( 0 != __strex((unsigned int)swap, (volatile int*)ptr) );
    return old;
}

#define ATOMIC_LOAD_NULLIFY_PTR(addr)	((void *)ATOMIC_LW_SW((volatile long *)(addr), (long)NULL))

static inline long ATOMIC_LW_SW(volatile long *addr, long value)
{
    long t1, t2 = *addr;
    do {
        t1 = t2;
        t2 = ATOMIC_CAS(addr, t1, value);
    } while (t1 != t2);
    return t1;
}


static inline long ATOMIC_ADD(long *addr, long value)
{
    long t1, t2 = *addr;
    do {
        t1 = t2;
        t2 = ATOMIC_CAS((long *)addr, t1, t1+value);
    } while (t1 != t2);
    return t1 + value;
}


static inline long ATOMIC_EXCHANGE_ADD(long *addr, long value)
{
    long t1, t2 = *addr;
    do {
        t1 = t2;
        t2 = ATOMIC_CAS((long *)addr, t1, t1+value);
    } while (t1 != t2);
    return t1;
}

/* Bit support */

static inline int test_and_set_bit(long nr, unsigned long *addr) 
{
    unsigned long *m = ((unsigned long *) addr) + (nr >> 5);
    unsigned long bit = (1U << (nr & 0x1F));
    long t1, t2 = *addr;
    do {
        t1 = t2;
        t2 = ATOMIC_CAS((long *)m, t1, t1|bit);
    } while (t1 != t2);
    return (t1 & bit) != 0;
}

static inline int test_and_clear_bit(long nr, unsigned long *addr) 
{
    unsigned long *m = ((unsigned long *) addr) + (nr >> 5);
    unsigned long bit = (1U << (nr & 0x1F));
    long t1, t2 = *addr;
    do {
        t1 = t2;
        t2 = ATOMIC_CAS((long *)m, t1, t1^bit);
    } while (t1 != t2);
    return (t1 & bit) != 0;
}


#endif /* _H_pthread_atomic_psp */
