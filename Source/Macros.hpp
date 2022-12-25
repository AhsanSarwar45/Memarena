#pragma once

#ifdef MEMARENA_DEBUG
    #define MEMARENA_DEBUG_BREAK
#endif

#define NO_DISCARD_ALLOC_INFO "Not using the pointer returned will cause a soft memory leak!"

#define NO_DISCARD [[nodiscard(NO_DISCARD_ALLOC_INFO)]]

#define RETURN_IF_NULLPTR(ptr) \
    {                          \
        if (ptr == nullptr)    \
        {                      \
            return nullptr;    \
        }                      \
    }
#define RETURN_VAL_IF_NULLPTR(ptr, returnValue) \
    {                                           \
        if (ptr == nullptr)                     \
        {                                       \
            return returnValue;                 \
        }                                       \
    }
