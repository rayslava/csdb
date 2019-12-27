#pragma once

#define GCC_BUILD (defined(__GNUC__) && !defined(__clang__))

#ifdef GCC_BUILD
#define likely(x)      __builtin_expect(!!(x), 1)
#define unlikely(x)    __builtin_expect(!!(x), 0)
#else
#define likely(x)      (x)
#define unlikely(x)    (x)
#endif
