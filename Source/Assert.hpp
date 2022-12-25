#pragma once

#include <iostream>

#include "Aliases.hpp"
#include "DebugBreak.hpp"

#define MEMARENA_HANDLE_ASSERT_FAILURE(breakOnFailureIsEnabled, failureLoggingIsEnabled, ...) \
    if constexpr (failureLoggingIsEnabled)                                                    \
    {                                                                                         \
        fprintf(stderr, __VA_ARGS__);                                                         \
    }                                                                                         \
    if constexpr (breakOnFailureIsEnabled)                                                    \
    {                                                                                         \
        DEBUG_BREAK();                                                                        \
    }

#define MEMARENA_ASSERT(predicate, ...)                                                                                      \
    {                                                                                                                        \
        if (!(predicate))                                                                                                    \
        {                                                                                                                    \
            MEMARENA_HANDLE_ASSERT_FAILURE(Settings.breakOnFailureIsEnabled, Settings.failureLoggingIsEnabled, __VA_ARGS__); \
        }                                                                                                                    \
    }

#define MEMARENA_DEFAULT_ASSERT(predicate, ...)                      \
    {                                                                \
        if (!(predicate))                                            \
        {                                                            \
            MEMARENA_HANDLE_ASSERT_FAILURE(true, true, __VA_ARGS__); \
        }                                                            \
    }

#define MEMARENA_ASSERT_RETURN(predicate, returnValue, ...)                                                                  \
    {                                                                                                                        \
        if (!(predicate))                                                                                                    \
        {                                                                                                                    \
            MEMARENA_HANDLE_ASSERT_FAILURE(Settings.breakOnFailureIsEnabled, Settings.failureLoggingIsEnabled, __VA_ARGS__); \
            return returnValue;                                                                                              \
        }                                                                                                                    \
    }

#define MEMARENA_ERROR(...) MEMARENA_ASSERT(false, __VA_ARGS__)
