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
    #ifdef MEMARENA_ENABLE_ASSERTS_RETURN
        #ifndef MEMARENA_ASSERT_RETURN
            #define MEMARENA_ASSERT_RETURN(x, returnValue, ...) \
                {                                               \
                    if (!(x))                                   \
                    {                                           \
                        return returnValue                      \
                    }                                           \
                }
        #endif
    #else
        #ifndef MEMARENA_ASSERT_RETURN
            #define MEMARENA_ASSERT_RETURN(x, returnValue, ...) MEMARENA_ASSERT(x, __VA_ARGS__)
        #endif
    #endif
#else
    #ifndef MEMARENA_ASSERT
        #define MEMARENA_ASSERT(x, ...) \
            {                           \
            }
    #endif
    #ifndef MEMARENA_ASSERT_RETURN
        #define MEMARENA_ASSERT_RETURN(x, returnValue, ...) \
            {                                               \
            }
    #endif
#endif