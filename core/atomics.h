#ifndef ATOMICS_H
#define ATOMICS_H

/* hacky atomics */
#if defined (HAS_MULTITHREAD)
#ifndef __cplusplus
#if !defined(__STDC_NO_ATOMICS__) && !defined(_MSC_VER)
 #include <stdatomic.h>
#else
 #define _Atomic(X) volatile X /* doesn't do anything, but makes me feel better... although if you are trying to do multithreading glhf */
 #define atomic_compare_exchange_strong(object, expected, desired) (*(object) == *(expected) ? (*(object) = (desired)) : (*(expected) = (desired)))
#endif
#else
 #include <atomic>
 #define _Atomic(X) std::atomic<X>
#endif
#else
 #define _Atomic(X) X
 #define atomic_compare_exchange_strong(object, expected, desired) (*(object) == *(expected) ? (*(object) = (desired)) : (*(expected) = (desired)))
#endif

#endif
