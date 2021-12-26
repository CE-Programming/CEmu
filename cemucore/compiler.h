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
# define cemucore_maybe_atomic_load_explicit(obj, order) (*(obj))
# define cemucore_maybe_atomic_store_explicit(obj, desired, order) (*(obj) = (desired))
# define cemucore_maybe_atomic_exchange_explicit(obj, desired, order)   \
    ({ typeof(*(obj)) cemucore_maybe_atomic_value = *(obj);             \
        *(obj) = (desired);                                             \
        cemucore_maybe_atomic_value })
# define cemucore_maybe_atomic_fetch_or_explicit(obj, arg, order)       \
    ({ typeof(*(obj)) cemucore_maybe_atomic_value = *(obj);             \
        *(obj) |= (arg);                                                \
        cemucore_maybe_atomic_value })
#else
# include <stdatomic.h>
# define CEMUCORE_MAYBE_ATOMIC(type) _Atomic(type)
# define cemucore_maybe_atomic_load_explicit atomic_load_explicit
# define cemucore_maybe_atomic_store_explicit atomic_store_explicit
# define cemucore_maybe_atomic_exchange_explicit atomic_exchange_explicit
# define cemucore_maybe_atomic_fetch_or_explicit atomic_fetch_or_explicit
#endif

#endif
