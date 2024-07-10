#ifndef ATOMICS_H
#define ATOMICS_H

/* hacky atomics */
#if defined (MULTITHREAD)
#ifndef __cplusplus
#if !defined(__STDC_NO_ATOMICS__) || (defined(_MSC_VER) && _MSC_VER >= 1935)
 #include <stdatomic.h>
 #define atomic8_fetch_and atomic_fetch_and
 #define atomic32_fetch_and atomic_fetch_and
 #define atomic8_fetch_and_explicit atomic_fetch_and_explicit
 #define atomic32_fetch_and_explicit atomic_fetch_and_explicit
 #define atomic8_fetch_or atomic_fetch_or
 #define atomic32_fetch_or atomic_fetch_or
 #define atomic8_fetch_or_explicit atomic_fetch_or_explicit
 #define atomic32_fetch_or_explicit atomic_fetch_or_explicit
#else
 #define _Atomic(X) volatile X /* doesn't do anything, but makes me feel better... although if you are trying to do multithreading glhf */
 #define atomic_load_explicit(object, order) (*(object))
static inline atomic8_fetch_and(volatile uint8_t *obj, uint8_t arg) {
    uint8_t result = *obj;
    *obj = result & arg;
    return result;
}
static inline atomic32_fetch_and(volatile uint32_t *obj, uint8_t arg) {
    uint32_t result = *obj;
    *obj = result & arg;
    return result;
}
static inline atomic8_fetch_or(volatile uint8_t *obj, uint8_t arg) {
    uint8_t result = *obj;
    *obj = result | arg;
    return result;
}
static inline atomic32_fetch_or(volatile uint32_t *obj, uint8_t arg) {
    uint32_t result = *obj;
    *obj = result | arg;
    return result;
}
 #define atomic8_fetch_and_explicit(object, arg, order) atomic8_fetch_and(object, arg)
 #define atomic32_fetch_and_explicit(object, arg, order) atomic32_fetch_and(object, arg)
 #define atomic8_fetch_or_explicit(object, arg, order) atomic8_fetch_or(object, arg)
 #define atomic32_fetch_or_explicit(object, arg, order) atomic32_fetch_or(object, arg)
#endif
#else
 #include <atomic>
 #define _Atomic(X) std::atomic<X>
#endif
#else
 #define _Atomic(X) X
 #define atomic_load_explicit(object, order) (*(object))
static inline atomic8_fetch_and(uint8_t *obj, uint8_t arg) {
    uint8_t result = *obj;
    *obj = result & arg;
    return result;
}
static inline atomic32_fetch_and(uint32_t *obj, uint8_t arg) {
    uint32_t result = *obj;
    *obj = result & arg;
    return result;
}
static inline atomic8_fetch_or(uint8_t *obj, uint8_t arg) {
    uint8_t result = *obj;
    *obj = result | arg;
    return result;
}
static inline atomic32_fetch_or(uint32_t *obj, uint8_t arg) {
    uint32_t result = *obj;
    *obj = result | arg;
    return result;
}
 #define atomic8_fetch_and_explicit(object, arg, order) atomic8_fetch_and(object, arg)
 #define atomic32_fetch_and_explicit(object, arg, order) atomic32_fetch_and(object, arg)
 #define atomic8_fetch_or_explicit(object, arg, order) atomic8_fetch_or(object, arg)
 #define atomic32_fetch_or_explicit(object, arg, order) atomic32_fetch_or(object, arg)
#endif

#endif
