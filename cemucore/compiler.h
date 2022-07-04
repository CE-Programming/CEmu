/*
 * Copyright (c) 2015-2021 CE Programming.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CEMUCORE_COMPILER_H
#define CEMUCORE_COMPILER_H

#ifndef __has_builtin
# define __has_builtin(x) 0
#endif

#ifndef __has_attribute
# define __has_attribute(x) 0
#endif

#if __has_builtin(__builtin_expect)
# define cemucore_unlikely(x) (__builtin_expect(x, 0))
# define cemucore_likely(x) (!cemucore_unlikely(!(x)))
#else
# define cemucore_unlikely
# define cemucore_likely
#endif
#if __has_builtin(__builtin_unpredictable)
# define cemucore_unpredictable __builtin_unpredictable
#else
# define cemucore_unpredictable
#endif

#if __has_builtin(__builtin_add_overflow)
# define cemucore_add_overflow __builtin_add_overflow
#else
# define cemucore_add_overflow(lhs, rhs, res)           \
    (*(res) = ((typeof(*(res))))(lhs) + (rhs),          \
     (rhs) < 0 ? *(res) > (lhs) : *(res) < (lhs))
#endif
#if __has_builtin(__builtin_sub_overflow)
# define cemucore_sub_overflow __builtin_sub_overflow
#else
# define cemucore_sub_overflow(lhs, rhs, res)           \
    (*(res) = (typeof(*(res)))(lhs) - (rhs),            \
     (rhs) < 0 ? *(res) < (lhs) : *(res) > (lhs))
#endif
#if __has_builtin(__builtin_mul_overflow)
# define cemucore_mul_overflow __builtin_mul_overflow
#else
# define cemucore_mul_overflow(lhs, rhs, res)          \
    (*(res) = (typeof(*(res)))(lhs) * (rhs),           \

#endif

#if __has_attribute(__fallthrough__)
# define cemucore_fallthrough __attribute__((__fallthrough__))
#else
# define cemucore_fallthrough (void)0
#endif
#if __has_attribute(__noreturn__)
# define cemucore_noreturn __attribute__((__noreturn__))
#else
# define cemucore_noreturn
#endif
#if __has_attribute(__malloc__)
# define cemucore_alloc __attribute__((__malloc__))
#else
# define cemucore_alloc
#endif

#ifdef __cplusplus
#define cemucore_unused(variable) (static_cast<void>(variable))
#else
#define cemucore_unused(variable) ((void)(variable))
#endif

#ifndef CEMUCORE_BYTE_ORDER
# ifdef __BYTE_ORDER__
#  define CEMUCORE_ORDER_LITTLE_ENDIAN __ORDER_LITTLE_ENDIAN__
#  define CEMUCORE_ORDER_BIG_ENDIAN    __ORDER_BIG_ENDIAN__
#  define CEMUCORE_BYTE_ORDER          __BYTE_ORDER__
# else
#  define CEMUCORE_ORDER_LITTLE_ENDIAN 0
#  define CEMUCORE_ORDER_BIG_ENDIAN    1
#  define CEMUCORE_BYTE_ORDER          CEMUCORE_ORDER_LITTLE_ENDIAN
# endif
#endif

#ifdef CEMUCORE_NOTHREADS
# define CEMUCORE_MAYBE_ATOMIC(type) type

# define cemucore_maybe_atomic_load(res, obj, order) cemucore_unused((res) = (obj))
# define cemucore_maybe_atomic_store(obj, desired, order) cemucore_unused((obj) = (desired))
# define cemucore_maybe_atomic_exchange(res, obj, desired, order) cemucore_unused(((res) = (obj), (obj) = (desired)))
# define cemucore_maybe_atomic_add(obj, arg, order) cemucore_unused((obj) += (arg))
# define cemucore_maybe_atomic_fetch_add(res, obj, arg, order) cemucore_unused(((res) = (obj), (obj) += (arg)))
# define cemucore_maybe_atomic_add_fetch(res, obj, arg, order) cemucore_unused((res) = (obj) += (arg))
# define cemucore_maybe_atomic_sub(obj, arg, order) cemucore_unused((obj) -= (arg))
# define cemucore_maybe_atomic_fetch_sub(res, obj, arg, order) cemucore_unused(((res) = (obj), (obj) -= (arg)))
# define cemucore_maybe_atomic_sub_fetch(res, obj, arg, order) cemucore_unused((res) = (obj) -= (arg))
# define cemucore_maybe_atomic_or(obj, arg, order) cemucore_unused((obj) |= (arg))
# define cemucore_maybe_atomic_fetch_or(res, obj, arg, order) cemucore_unused(((res) = (obj), (obj) |= (arg)))
# define cemucore_maybe_atomic_or_fetch(res, obj, arg, order) cemucore_unused((res) = (obj) |= (arg))
# define cemucore_maybe_atomic_xor(obj, arg, order) cemucore_unused((obj) ^= (arg))
# define cemucore_maybe_atomic_fetch_xor(res, obj, arg, order) cemucore_unused(((res) = (obj), (obj) ^= (arg)))
# define cemucore_maybe_atomic_xor_fetch(res, obj, arg, order) cemucore_unused((res) = (obj) ^= (arg))
# define cemucore_maybe_atomic_and(obj, arg, order) cemucore_unused((obj) &= (arg))
# define cemucore_maybe_atomic_fetch_and(res, obj, arg, order) cemucore_unused(((res) = (obj), (obj) &= (arg)))
# define cemucore_maybe_atomic_and_fetch(res, obj, arg, order) cemucore_unused((res) = (obj) &= (arg))

# define cemucore_maybe_mutex char
#define CEMUCORE_MAYBE_MUTEX_INITIALIZER 0
# define cemucore_maybe_mutex_lock(obj) (cemucore_unused(&(obj)), 0)
# define cemucore_maybe_mutex_unlock(obj) (cemucore_unused(&(obj)), 0)
#else
# include <pthread.h>
# include <stdatomic.h>

# define CEMUCORE_MAYBE_ATOMIC(type) _Atomic(type)

# define cemucore_maybe_atomic_load(res, obj, order) cemucore_unused((res) = atomic_load_explicit(&(obj), memory_order_##order))
# define cemucore_maybe_atomic_store(obj, desired, order) cemucore_unused(atomic_store_explicit(&(obj), (desired), memory_order_##order))
# define cemucore_maybe_atomic_exchange(res, obj, desired, order) cemucore_unused((res) = atomic_exchange_explicit(&(obj), (desired), memory_order_##order))
# define cemucore_maybe_atomic_add(obj, arg, order) cemucore_unused(atomic_fetch_add_explicit(&(obj), (arg), memory_order_##order))
# define cemucore_maybe_atomic_fetch_add(res, obj, arg, order) cemucore_unused((res) = atomic_fetch_add_explicit(&(obj), (arg), memory_order_##order))
# define cemucore_maybe_atomic_add_fetch(res, obj, arg, order) cemucore_unused((res) = atomic_fetch_add_explicit(&(obj), (arg), memory_order_##order) + (arg))
# define cemucore_maybe_atomic_sub(obj, arg, order) cemucore_unused(atomic_fetch_sub_explicit(&(obj), (arg), memory_order_##order))
# define cemucore_maybe_atomic_fetch_sub(res, obj, arg, order) cemucore_unused((res) = atomic_fetch_sub_explicit(&(obj), (arg), memory_order_##order))
# define cemucore_maybe_atomic_sub_fetch(res, obj, arg, order) cemucore_unused((res) = atomic_fetch_sub_explicit(&(obj), (arg), memory_order_##order) - (arg))
# define cemucore_maybe_atomic_or(obj, arg, order) cemucore_unused(atomic_fetch_or_explicit(&(obj), (arg), memory_order_##order))
# define cemucore_maybe_atomic_fetch_or(res, obj, arg, order) cemucore_unused((res) = atomic_fetch_or_explicit(&(obj), (arg), memory_order_##order))
# define cemucore_maybe_atomic_or_fetch(res, obj, arg, order) cemucore_unused((res) = atomic_fetch_or_explicit(&(obj), (arg), memory_order_##order) | (arg))
# define cemucore_maybe_atomic_xor(obj, arg, order) cemucore_unused(atomic_fetch_xor_explicit(&(obj), (arg), memory_order_##order))
# define cemucore_maybe_atomic_fetch_xor(res, obj, arg, order) cemucore_unused((res) = atomic_fetch_xor_explicit(&(obj), (arg), memory_order_##order))
# define cemucore_maybe_atomic_xor_fetch(res, obj, arg, order) cemucore_unused((res) = atomic_fetch_xor_explicit(&(obj), (arg), memory_order_##order) ^ (arg))
# define cemucore_maybe_atomic_and(obj, arg, order) cemucore_unused(atomic_fetch_and_explicit(&(obj), (arg), memory_order_##order))
# define cemucore_maybe_atomic_fetch_and(res, obj, arg, order) cemucore_unused((res) = atomic_fetch_and_explicit(&(obj), (arg), memory_order_##order))
# define cemucore_maybe_atomic_and_fetch(res, obj, arg, order) cemucore_unused((res) = atomic_fetch_and_explicit(&(obj), (arg), memory_order_##order) & (arg))

# define cemucore_maybe_mutex pthread_mutex_t
# define CEMUCORE_MAYBE_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER
# define cemucore_maybe_mutex_lock(obj) pthread_mutex_lock(&(obj))
# define cemucore_maybe_mutex_unlock(obj) pthread_mutex_unlock(&(obj))
#endif

#endif
