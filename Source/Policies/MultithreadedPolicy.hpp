#pragma once

#include <mutex>
#include <type_traits>

#include "Policies.hpp"

namespace Memarena
{

template <bool IsMultithreaded, bool IsRecursive = false>
class MultithreadedPolicy
{
  private:
    template <typename SyncPrimitive>
    class DummyGuard
    {
      public:
        explicit DummyGuard(const SyncPrimitive& syncPrimitive) {}
        void unlock() {}
    };

    class DummyMutex
    {
    };

  public:
    template <typename SyncPrimitive>
    using LockGuard = typename std::conditional<
        IsMultithreaded, typename std::conditional<IsRecursive, std::unique_lock<SyncPrimitive>, std::lock_guard<SyncPrimitive>>::type,
        DummyGuard<SyncPrimitive>>::type;

    using Mutex = typename std::conditional<IsMultithreaded, std::mutex, DummyMutex>::type;

    Mutex m_Mutex;
};
} // namespace Memarena