#pragma once

#include "Aliases.hpp"
#include "DebugBreak.hpp"

#ifdef MEMARENA_ENABLE_ASSERTS
    #include <iostream>
    #ifndef MEMARENA_ASSERT
        #define MEMARENA_ASSERT(x, ...)           \
            {                                     \
                if (!(x))                         \
                {                                 \
                    fprintf(stderr, __VA_ARGS__); \
                    DEBUG_BREAK();                \
                }                                 \
            }
    #endif
    #ifndef MEMARENA_BARE_ASSERT
        #define MEMARENA_BARE_ASSERT(x, ...) \
            {                                \
                if (!(x))                    \
                {                            \
                    DEBUG_BREAK();           \
                }                            \
            }

    #else
    #endif
#else
    #ifndef MEMARENA_ASSERT
        #define MEMARENA_ASSERT(x, ...) \
            {                           \
            }
    #endif
    #ifndef MEMARENA_BARE_ASSERT
        #define MEMARENA_BARE_ASSERT(x, ...) \
            {                                \
            }
    #endif
#endif //  QMBT_ENABLE_ASSERTS