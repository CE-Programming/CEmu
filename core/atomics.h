#ifndef ATOMICS_H
#define ATOMICS_H

/* hacky atomics */
#if defined (MULTITHREAD)
#ifndef __cplusplus
#if !defined(__STDC_NO_ATOMICS__)
 #include <stdatomic.h>
#else
 #define _Atomic(X) volatile X /* doesn't do anything, but makes me feel better... although if you are trying to do multithreading glhf */
#endif
#else
 #include <atomic>
 #define _Atomic(X) std::atomic<X>
#endif
#else
 #define _Atomic(X) X
#endif

#endif
