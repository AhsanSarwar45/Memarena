#pragma once

#include "Policies/Policies.hpp"

namespace Memarena
{
constexpr auto GetDefaultBreakOnFailureSetting() -> bool
{
#ifdef MEMARENA_DEBUG
    return true;
#else
    return false;
#endif
}

constexpr auto GetDefaultFailureLoggingSetting() -> bool
{
#ifdef MEMARENA_DEBUG
    return true;
#else
    return false;
#endif
}

template <AllocatorPolicy Policy>
struct AllocatorSettings
{
    Policy policy                  = GetDefaultPolicy<Policy>();
    bool   breakOnFailureIsEnabled = GetDefaultBreakOnFailureSetting();
    bool   failureLoggingIsEnabled = GetDefaultFailureLoggingSetting();

    // constexpr AllocatorSettings() = default;
    // constexpr AllocatorSettings(const Policy policy = GetDefaultPolicy<Policy>(), const bool breakOnFailureIsEnabled = true,
    //                             const bool failureLoggingIsEnabled = true)
    //     : policy(policy), breakOnFailureIsEnabled(breakOnFailureIsEnabled), failureLoggingIsEnabled(failureLoggingIsEnabled)
    // {
    // }
    // constexpr AllocatorSettings(const bool breakOnFailureIsEnabled, const bool failureLoggingIsEnabled)
    //     : policy(GetDefaultPolicy<Policy>()), breakOnFailureIsEnabled(breakOnFailureIsEnabled),
    //     failureLoggingIsEnabled(failureLoggingIsEnabled)
    // {
    // }
};
} // namespace Memarena