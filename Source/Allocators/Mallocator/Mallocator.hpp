#pragma once

#include <bit>
#include <vector>

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

struct MallocHeader
{
    Size size;
};

template <typename T>
class MallocPtr : public Ptr<T>
{
  public:
    inline MallocPtr(T* ptr, Size size) : Ptr<T>(ptr), m_Header({size}) {}
    [[nodiscard]] Size GetSize() const { return m_Header.size; }

  private:
    MallocHeader m_Header;
};

template <typename T>
class MallocArrayPtr : public ArrayPtr<T>
{
  public:
    inline MallocArrayPtr(T* ptr, Size size, Size count) : ArrayPtr<T>(ptr, count), m_Header({size}) {}
    [[nodiscard]] Size GetSize() const { return m_Header.size; }

  private:
    MallocHeader m_Header;
};

using MallocatorSettings = AllocatorSettings<MallocatorPolicy>;
constexpr MallocatorSettings mallocatorDefaultSettings{};

template <MallocatorSettings Settings = mallocatorDefaultSettings>
class Mallocator : public Allocator
{
  private:
    static constexpr MallocatorPolicy Policy = Settings.policy;

    static constexpr bool DoubleFreePreventionIsEnabled = PolicyContains(Policy, MallocatorPolicy::DoubleFreePrevention);
    static constexpr bool NullDeallocCheckIsEnabled =
        PolicyContains(Policy, MallocatorPolicy::NullDeallocCheck) || DoubleFreePreventionIsEnabled;
    static constexpr bool NullAllocCheckIsEnabled     = PolicyContains(Policy, MallocatorPolicy::NullAllocCheck);
    static constexpr bool AllocationTrackingIsEnabled = PolicyContains(Policy, MallocatorPolicy::AllocationTracking);
    static constexpr bool SizeTrackingIsEnabled       = PolicyContains(Policy, MallocatorPolicy::SizeTracking);
    static constexpr bool NeedsMultithreading         = AllocationTrackingIsEnabled || SizeTrackingIsEnabled;
    static constexpr bool IsMultithreaded             = PolicyContains(Policy, MallocatorPolicy::Multithreaded) && NeedsMultithreading;

    using ThreadPolicy = MultithreadedPolicy<IsMultithreaded>;

    template <typename SyncPrimitive>
    using LockGuard = typename ThreadPolicy::template LockGuard<SyncPrimitive>;
    using Mutex     = typename ThreadPolicy::Mutex;

  public:
    // Prohibit default construction, moving and assignment
    // Mallocator()                  = delete;
    Mallocator(const Mallocator&) = delete;
    Mallocator(Mallocator&)       = delete;
    Mallocator(Mallocator&&)      = delete;
    Mallocator& operator=(const Mallocator&) = delete;
    Mallocator& operator=(Mallocator&&) = delete;

    Mallocator() : Allocator(0, "Mallocator", true) {}
    explicit Mallocator(const std::string& debugName) : Allocator(0, debugName, true) {}

    ~Mallocator() = default;

    template <Allocatable Object, typename... Args>
    NO_DISCARD MallocPtr<Object> New(Args&&... argList)
    {
        void* voidPtr = AllocateInternal(sizeof(Object));
        RETURN_VAL_IF_NULLPTR(voidPtr, MallocPtr<Object>(nullptr, 0));
        Object* objectPtr = new (voidPtr) Object(std::forward<Args>(argList)...);
        return MallocPtr<Object>(objectPtr, sizeof(Object));
    }

    template <Allocatable Object>
    void Delete(MallocPtr<Object>& ptr)
    {
        DeallocateInternal(ptr, ptr.GetSize());
        ptr->~Object();
    }

    template <Allocatable Object, typename... Args>
    NO_DISCARD MallocArrayPtr<Object> NewArray(const Size objectCount, Args&&... argList)
    {
        void* voidPtr = AllocateInternal(sizeof(Object) * objectCount);
        RETURN_VAL_IF_NULLPTR(voidPtr, MallocArrayPtr<Object>(nullptr, 0, 0));
        Object* objectPtr = Internal::ConstructArray<Object>(voidPtr, objectCount, std::forward<Args>(argList)...);
        return MallocArrayPtr<Object>(objectPtr, objectCount * sizeof(Object), objectCount);
    }

    template <Allocatable Object>
    void DeleteArray(MallocArrayPtr<Object>& ptr)
    {
        DeallocateInternal(ptr, ptr.GetSize());
        std::destroy_n(ptr.GetPtr(), ptr.GetCount());
    }

    NO_DISCARD void* Allocate(const Size size, const std::string& category = "",
                              const SourceLocation& sourceLocation = SourceLocation::current())
    {
        return AllocateInternalWithHeader(size, category, sourceLocation);
    }

    template <typename Object>
    NO_DISCARD void* Allocate(const std::string& category = "", const SourceLocation& sourceLocation = SourceLocation::current())
    {
        return AllocateInternalWithHeader(sizeof(Object), category, sourceLocation);
    }

    NO_DISCARD void* AllocateArray(const Size objectCount, const Size objectSize, const std::string& category = "",
                                   const SourceLocation& sourceLocation = SourceLocation::current())
    {
        return AllocateInternalWithHeader(objectCount * objectSize, category, sourceLocation);
    }

    template <typename Object>
    NO_DISCARD void* AllocateArray(const Size objectCount, const std::string& category = "",
                                   const SourceLocation& sourceLocation = SourceLocation::current())
    {
        return AllocateArray(objectCount, sizeof(Object), category, sourceLocation);
    }

    void Deallocate(void*& ptr) { DeallocateInternalWithHeader(ptr); }
    void DeallocateArray(void*& ptr) { Deallocate(ptr); }

    NO_DISCARD void* AllocateBase(const Size size) final { return Allocate(size); }
    void             DeallocateBase(void* ptr) final { Deallocate(ptr); }

  private:
    NO_DISCARD void* AllocateInternal(const Size size, const std::string& category = "",
                                      const SourceLocation& sourceLocation = SourceLocation::current(), Padding padding = 0)
    {
        void* ptr = malloc(padding + size);

        if constexpr (NullAllocCheckIsEnabled)
        {
            MEMARENA_ASSERT_RETURN(ptr != nullptr, nullptr, "Error: The allocator '%s' couldn't allocate any memory!\n",
                                   GetDebugName().c_str());
        }

        {
            LockGuard<Mutex> guard(m_MultithreadedPolicy.m_Mutex);

            if constexpr (AllocationTrackingIsEnabled)
            {
                AddAllocation(size, category, sourceLocation);
            }
            if constexpr (SizeTrackingIsEnabled)
            {
                IncreaseTotalSize(size);
                IncreaseUsedSize(size);
            }
        }

        UIntPtr address       = std::bit_cast<UIntPtr>(ptr);
        void*   allocationPtr = std::bit_cast<void*>(address + padding);

        return allocationPtr;
    }

    void* AllocateInternalWithHeader(const Size size, const std::string& category = "",
                                     const SourceLocation& sourceLocation = SourceLocation::current())
    {
        const Padding padding = ExtendPaddingForHeader(0, alignof(size), sizeof(MallocHeader));
        void*         ptr     = AllocateInternal(size, category, sourceLocation, padding);
        Internal::AllocateHeader<MallocHeader>(ptr, size);
        return ptr;
    }

    template <typename Object>
    void DeallocateInternal(Ptr<Object>& ptr, Size size)
    {
        DeallocateInternal(static_cast<void*>(ptr.GetPtr()), size);

        if constexpr (DoubleFreePreventionIsEnabled)
        {
            ptr.Reset();
        }
    }

    void DeallocateInternalWithHeader(void*& ptr)
    {
        const UIntPtr address        = std::bit_cast<UIntPtr>(ptr);
        auto [header, headerAddress] = Internal::GetHeaderFromAddress<MallocHeader>(address);
        const Padding padding        = ExtendPaddingForHeader(0, alignof(header.size), sizeof(MallocHeader));

        // We can't call `free(ptr)` because `ptr` not the same as the one returned by malloc, since we added padding for header
        // So we subtract that padding to get the original pointer
        const UIntPtr mallocAddress = address - padding;
        void*         mallocPtr     = std::bit_cast<void*>(mallocAddress);
        DeallocateInternal(mallocPtr, header.size);

        if constexpr (DoubleFreePreventionIsEnabled)
        {
            ptr = nullptr;
        }
    }

    void DeallocateInternal(void* ptr, Size size)
    {
        if constexpr (NullDeallocCheckIsEnabled)
        {
            MEMARENA_ASSERT_RETURN(ptr, void(), "Error: Cannot deallocate nullptr in allocator '%s'!\n", GetDebugName().c_str());
        }

        free(ptr);

        {
            LockGuard<Mutex> guard(m_MultithreadedPolicy.m_Mutex);

            if constexpr (AllocationTrackingIsEnabled)
            {
                AddDeallocation();
            }
            if constexpr (SizeTrackingIsEnabled)
            {

                DecreaseTotalSize(size);
                DecreaseUsedSize(size);
            }
        }
    }

    void AllocateHeader(void* ptr) {}

    ThreadPolicy m_MultithreadedPolicy;
};
} // namespace Memarena