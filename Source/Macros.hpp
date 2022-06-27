#pragma once

#ifdef MEMARENA_DEBUG
    #define MEMARENA_ENABLE_ASSERTS
    #define MEMARENA_DEBUG_BREAK
#endif

#define NO_DISCARD_ALLOC_INFO "Not using the pointer returned will cause a soft memory leak!"

#define NO_DISCARD [[nodiscard(NO_DISCARD_ALLOC_INFO)]]