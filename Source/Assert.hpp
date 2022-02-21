#pragma once

#include "Aliases.hpp"
#include "DebugBreak.hpp"

#ifdef MEMORY_MANAGER_ENABLE_ASSERTS
    #ifndef MEMORY_MANAGER_ASSERT
        #define MEMORY_MANAGER_ASSERT(x, ...)     \
            {                                     \
                if (!(x))                         \
                {                                 \
                    fprintf(stderr, __VA_ARGS__); \
                    DEBUG_BREAK();                \
                }                                 \
            }
    #endif
    #ifndef MEMORY_MANAGER_BARE_ASSERT
        #define MEMORY_MANAGER_BARE_ASSERT(x, ...) \
            {                                      \
                if (!(x))                          \
                {                                  \
                    DEBUG_BREAK();                 \
                }                                  \
            }

    #else
    #endif
#else
    #ifndef MEMORY_MANAGER_ASSERT
        #define MEMORY_MANAGER_ASSERT(x, ...)
    #endif
    #ifndef MEMORY_MANAGER_BARE_ASSERT
        #define MEMORY_MANAGER_BARE_ASSERT(x, ...)
    #endif
#endif //  QMBT_ENABLE_ASSERTS