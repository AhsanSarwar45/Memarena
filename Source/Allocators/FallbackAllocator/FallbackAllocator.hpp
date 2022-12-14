#pragma once

#include <bit>
#include <type_traits>
#include <utility>
#include <vector>

#include "Source/Aliases.hpp"
#include "Source/Allocator.hpp"
#include "Source/AllocatorData.hpp"
#include "Source/AllocatorSettings.hpp"
#include "Source/AllocatorUtils.hpp"
#include "Source/Assert.hpp"
#include "Source/Macros.hpp"
#include "Source/Policies/MultithreadedPolicy.hpp"
#include "Source/Policies/Policies.hpp"
#include "Source/Traits.hpp"
#include "Source/Utility/Alignment/Alignment.hpp"

namespace Memarena
{

template <typename>
constexpr std::false_type ImplementsOwnsHelper(Int32);

template <typename T>
constexpr auto ImplementsOwnsHelper(int) -> decltype(std::declval<T>().Owns(), std::true_type{});

template <typename T>
using ImplementsOwns = decltype(ImplementsOwnsHelper<T>(0));

template <typename T>
concept PrimaryAllocatable = requires(T allocator, void* voidPtr, int* ptr)
{
    {
        allocator.Allocate(0)
        } -> std::same_as<void*>;
    {
        allocator.template NewRaw<int>(0)
        } -> std::same_as<int*>;
    allocator.Deallocate(voidPtr);
    allocator.Delete(ptr);
    {
        allocator.Owns(voidPtr)
        } -> std::same_as<bool>;
};

template <typename T>
concept FallbackAllocatable = requires(T allocator, void* voidPtr, int* ptr)
{

    {
        allocator.Allocate(0)
        } -> std::same_as<void*>;
    {
        allocator.template NewRaw<int>(0)
        } -> std::same_as<int*>;
    allocator.Deallocate(voidPtr);
    allocator.Delete(ptr);
};

// template <PrimaryAllocatable PrimaryAllocator>
// struct Set
// {
// };

// Set<StackAllocator> set;

using FallbackAllocatorSettings = AllocatorSettings<FallbackAllocatorPolicy>;
constexpr FallbackAllocatorSettings fallbackAllocatorDefaultSettings{};

/**
 * @brief A custom memory allocator that cannot deallocate individual allocations. To free allocations, you must
 *       free the entire arena by calling `Release`.
 *
 * @tparam policy
 */
template <PrimaryAllocatable PrimaryAllocatorType, FallbackAllocatable FallbackAllocatorType,
          FallbackAllocatorSettings Settings = fallbackAllocatorDefaultSettings>
class FallbackAllocator : public Allocator
{
  private:
    // static constexpr auto Policy = Settings.policy;

    // static constexpr bool UsageTrackingIsEnabled      = PolicyContains(Policy, FallbackAllocatorPolicy::SizeTracking);
    // static constexpr bool AllocationTrackingIsEnabled = PolicyContains(Policy, FallbackAllocatorPolicy::AllocationTracking);
    // static constexpr bool IsMultithreaded             = PolicyContains(Policy, FallbackAllocatorPolicy::Multithreaded);

    // using ThreadPolicy = MultithreadedPolicy<IsMultithreaded, IsGrowable>;

    // template <typename SyncPrimitive>
    // using LockGuard = typename ThreadPolicy::template LockGuard<SyncPrimitive>;
    // using Mutex     = typename ThreadPolicy::Mutex;

  public:
    // Prohibit default construction, moving and assignment
    FallbackAllocator()                         = delete;
    FallbackAllocator(const FallbackAllocator&) = delete;
    FallbackAllocator(FallbackAllocator&)       = delete;
    FallbackAllocator(FallbackAllocator&&)      = delete;
    FallbackAllocator& operator=(const FallbackAllocator&) = delete;
    FallbackAllocator& operator=(FallbackAllocator&&) = delete;

    explicit FallbackAllocator(std::shared_ptr<PrimaryAllocatorType>  primaryAllocator,
                               std::shared_ptr<FallbackAllocatorType> fallbackAllocator, const std::string& debugName = "FallbackAllocator")
        : Allocator(0, debugName), m_PrimaryAllocator(std::move(primaryAllocator)), m_FallbackAllocator(std::move(fallbackAllocator))
    {
    }

    ~FallbackAllocator(){

    };

    template <Allocatable Object, typename... Args>
    NO_DISCARD Object* NewRaw(Args&&... argList)
    {
        Object* ptr = m_PrimaryAllocator->template NewRaw<Object>(std::forward<Args>(argList)...);
        if (ptr == nullptr)
        {
            ptr = m_FallbackAllocator->template NewRaw<Object>(std::forward<Args>(argList)...);
        }

        return ptr;
    }

    // template <Allocatable Object, typename... Args>
    // NO_DISCARD Object* NewArrayRaw(const Size objectCount, Args&&... argList)
    // {
    //     void* voidPtr = AllocateArray<Object>(objectCount);
    //     RETURN_IF_NULLPTR(voidPtr);
    //     return Internal::ConstructArray<Object>(voidPtr, objectCount, std::forward<Args>(argList)...);
    // }

    template <Allocatable Object>
    void Delete(Object*& ptr)
    {
        const UIntPtr address = std::bit_cast<UIntPtr>(ptr);

        if (m_PrimaryAllocator->Owns(address))
        {
            m_PrimaryAllocator->Delete(ptr);
        }

        else if constexpr (ImplementsOwns<FallbackAllocatorType>{})
        {
            if (m_FallbackAllocator->Owns(address))
            {
                m_FallbackAllocator->Delete(ptr);
            }
            else
            {
                MEMARENA_ERROR("Error: The allocator %s does not own the pointer %d!\n", GetDebugName().c_str(), address);
            }
        }
        else
        {
            m_FallbackAllocator->Delete(ptr);
        }
    }

    [[nodiscard]] bool Owns(UIntPtr ptr) const
    {
        if constexpr (ImplementsOwns<FallbackAllocatorType>{})
        {
            return m_PrimaryAllocator->Owns(ptr) || m_PrimaryAllocator->Owns(ptr);
        }
        else
        {
            return m_PrimaryAllocator->Owns(ptr);
        }
    }
    [[nodiscard]] bool Owns(void* ptr) const
    {
        const UIntPtr address = std::bit_cast<UIntPtr>(ptr);
        return Owns(address);
    }

    // template <Allocatable Object>
    // void DeleteArrayRaw(Object* ptr)
    // {
    //     const Size objectCount = DeallocateArray(ptr, sizeof(Object));
    //     std::destroy_n(ptr, objectCount);
    // }

    NO_DISCARD void* Allocate(const Size size, const Alignment& alignment = defaultAlignment, const std::string& category = "",
                              const SourceLocation& sourceLocation = SourceLocation::current())
    {
        void* ptr = m_PrimaryAllocator->Allocate(size, alignment, category, sourceLocation);
        if (ptr != nullptr)
        {
            ptr = m_FallbackAllocator->Allocate(size, alignment, category, sourceLocation);
        }

        return ptr;
    }

    template <typename Object>
    NO_DISCARD void* Allocate(const std::string& category = "", const SourceLocation& sourceLocation = SourceLocation::current())
    {
        return Allocate(sizeof(Object), alignof(Object), category, sourceLocation);
    }

    NO_DISCARD void* AllocateArray(const Size objectCount, const Size objectSize, const Alignment& alignment,
                                   const std::string& category = "", const SourceLocation& sourceLocation = SourceLocation::current())
    {
        const Size allocationSize = objectCount * objectSize;
        return Allocate(allocationSize, alignment, category, sourceLocation);
    }

    template <typename Object>
    NO_DISCARD void* AllocateArray(const Size objectCount, const std::string& category = "",
                                   const SourceLocation& sourceLocation = SourceLocation::current())
    {
        return AllocateArray(objectCount, sizeof(Object), alignof(Object), category, sourceLocation);
    }

    void Deallocate(void*& ptr)
    {
        const UIntPtr address = std::bit_cast<UIntPtr>(ptr);

        if (m_PrimaryAllocator->Owns(address))
        {
            m_PrimaryAllocator->Deallocate(ptr);
        }

        else if constexpr (ImplementsOwns<FallbackAllocatorType>{})
        {
            if (m_FallbackAllocator->Owns(address))
            {
                m_FallbackAllocator->Deallocate(ptr);
            }
            else
            {
                MEMARENA_ERROR("Error: The allocator %s does not own the pointer %d!\n", GetDebugName().c_str(), address);
            }
        }
        else
        {
            m_FallbackAllocator->Deallocate(ptr);
        }
    }

    void DeallocateArray(void*& ptr) { Deallocate(ptr); }

  private:
    std::shared_ptr<PrimaryAllocatorType>  m_PrimaryAllocator;
    std::shared_ptr<FallbackAllocatorType> m_FallbackAllocator;
};
} // namespace Memarena