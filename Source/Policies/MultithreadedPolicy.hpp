#pragma once

#include <mutex>

#include "Policies.hpp"

namespace Memarena
{

template <typename PolicyType>
struct PolicyWrapper
{
    PolicyType policy;

    constexpr PolicyWrapper(PolicyType _policy) : policy(_policy) {}
};

template <PolicyWrapper policyWrapper>
class MultithreadedPolicy
{
  private:
    template <typename SyncPrimitive>
    class DummyGuard
    {
      public:
        explicit DummyGuard(const SyncPrimitive& syncPrimitive) {}
    };

    class DummyMutex
    {
    };

  public:
    template <typename SyncPrimitive>
    using LockGuard = typename std::conditional<PolicyContains(policyWrapper.policy, AllocatorPolicy::Multithreaded),
                                                std::lock_guard<SyncPrimitive>, DummyGuard<SyncPrimitive>>::type;

    using Mutex =
        typename std::conditional<PolicyContains(policyWrapper.policy, AllocatorPolicy::Multithreaded), std::mutex, DummyMutex>::type;

    Mutex m_Mutex;
};
} // namespace Memarena