#pragma once

#include <bit>

#include "Source/Allocator.hpp"
#include "Source/AllocatorData.hpp"
#include "Source/AllocatorUtils.hpp"
#include "Source/Assert.hpp"
#include "Source/Policies/MultithreadedPolicy.hpp"
#include "Source/Policies/Policies.hpp"
#include "Source/Utility/Alignment.hpp"

#define NO_DISCARD_ALLOC_INFO "Not using the pointer returned will cause a soft memory leak!"

namespace Memarena
{

template <LinearAllocatorPolicy policy = LinearAllocatorPolicy::Default>
class LinearAllocator : public Internal::Allocator
{
  private:
    using ThreadPolicy = MultithreadedPolicy<policy>;

    template <typename SyncPrimitive>
    using LockGuard = typename ThreadPolicy::template LockGuard<SyncPrimitive>;
    using Mutex     = typename ThreadPolicy::Mutex;

  public:
    // Prohibit default construction, moving and assignment
    LinearAllocator()                       = delete;
    LinearAllocator(const LinearAllocator&) = delete;
    LinearAllocator(LinearAllocator&)       = delete;
    LinearAllocator(LinearAllocator&&)      = delete;
    LinearAllocator& operator=(const LinearAllocator&) = delete;
    LinearAllocator& operator=(LinearAllocator&&) = delete;

    explicit LinearAllocator(const Size totalSize, const std::shared_ptr<MemoryManager> memoryManager = nullptr,
                             const std::string& debugName = "LinearAllocator")
        : Internal::Allocator(totalSize, memoryManager, debugName), m_StartAddress(std::bit_cast<UIntPtr>(GetStartPtr()))
    {
    }

    ~LinearAllocator() = default;

    template <typename Object, typename... Args>
    [[nodiscard(NO_DISCARD_ALLOC_INFO)]] Object* NewRaw(Args&&... argList)
    {
        void* voidPtr = Allocate<Object>();
        return new (voidPtr) Object(std::forward<Args>(argList)...);
    }

    template <typename Object, typename... Args>
    [[nodiscard(NO_DISCARD_ALLOC_INFO)]] Object* NewArrayRaw(const Size objectCount, Args&&... argList)
    {
        void* voidPtr = AllocateArray<Object>(objectCount);
        return Internal::ConstructArray<Object>(voidPtr, objectCount, std::forward<Args>(argList)...);
    }

    [[nodiscard(NO_DISCARD_ALLOC_INFO)]] void* Allocate(const Size size, const Alignment& alignment)
    {
        LockGuard<Mutex> guard(m_MultithreadedPolicy.m_Mutex);

        const UIntPtr baseAddress = m_StartAddress + m_CurrentOffset;

        UIntPtr alignedAddress = CalculateAlignedAddress(baseAddress, alignment);
        Padding padding        = alignedAddress - baseAddress;

        Size totalSizeAfterAllocation = m_CurrentOffset + padding + size;

        if constexpr (PolicyContains(policy, LinearAllocatorPolicy::SizeCheck))
        {
            MEMARENA_ASSERT(totalSizeAfterAllocation <= GetTotalSize(), "Error: The allocator %s is out of memory!\n",
                            GetDebugName().c_str());
        }

        SetCurrentOffset(totalSizeAfterAllocation);

        return std::bit_cast<void*>(alignedAddress);
    }

    template <typename Object>
    [[nodiscard(NO_DISCARD_ALLOC_INFO)]] void* Allocate()
    {
        return Allocate(sizeof(Object), AlignOf(alignof(Object)));
    }

    [[nodiscard(NO_DISCARD_ALLOC_INFO)]] void* AllocateArray(const Size objectCount, const Size objectSize, const Alignment& alignment)
    {
        const Size allocationSize = objectCount * objectSize;
        return Allocate(allocationSize, alignment);
    }

    template <typename Object>
    [[nodiscard(NO_DISCARD_ALLOC_INFO)]] void* AllocateArray(const Size objectCount)
    {
        return AllocateArray(objectCount, sizeof(Object), AlignOf(alignof(Object)));
    }

    /**
     * @brief Resets the allocator to its initial state. Since LinearAllocators dont support de-alllocating separate allocation, this is how
     * you clean the memory
     *
     */
    inline void Reset() { SetCurrentOffset(0); };

  private:
    void SetCurrentOffset(const Offset offset)
    {
        m_CurrentOffset = offset;
        SetUsedSize(offset);
    }

    ThreadPolicy m_MultithreadedPolicy;

    UIntPtr m_StartAddress;
    Offset  m_CurrentOffset = 0;
};

template <typename Object, LinearAllocatorPolicy policy = LinearAllocatorPolicy::Default>
class LinearAllocatorTemplated
{
  public:
    LinearAllocatorTemplated()                                = delete;
    LinearAllocatorTemplated(LinearAllocatorTemplated&)       = delete;
    LinearAllocatorTemplated(const LinearAllocatorTemplated&) = delete;
    LinearAllocatorTemplated(LinearAllocatorTemplated&&)      = delete;
    LinearAllocatorTemplated& operator=(const LinearAllocatorTemplated&) = delete;
    LinearAllocatorTemplated& operator=(LinearAllocatorTemplated&&) = delete;

    explicit LinearAllocatorTemplated(const Size totalSize, const std::shared_ptr<MemoryManager> memoryManager = nullptr,
                                      const std::string& debugName = "LinearAllocator")
        : m_LinearAllocator(totalSize, memoryManager, debugName)
    {
    }

    ~LinearAllocatorTemplated() = default;

    template <typename... Args>
    [[nodiscard(NO_DISCARD_ALLOC_INFO)]] Object* NewRaw(Args&&... argList)
    {
        return m_LinearAllocator.template NewRaw<Object>(std::forward<Args>(argList)...);
    }

    template <typename... Args>
    [[nodiscard(NO_DISCARD_ALLOC_INFO)]] Object* NewArrayRaw(const Size objectCount, Args&&... argList)
    {
        return m_LinearAllocator.template NewArrayRaw<Object>(objectCount, std::forward<Args>(argList)...);
    }

    [[nodiscard(NO_DISCARD_ALLOC_INFO)]] void* Allocate(const Size size, const Alignment& alignment)
    {
        return m_LinearAllocator.Allocate(size, alignment);
    }

    [[nodiscard(NO_DISCARD_ALLOC_INFO)]] void* Allocate() { return m_LinearAllocator.Allocate(sizeof(Object), AlignOf(alignof(Object))); }

    [[nodiscard(NO_DISCARD_ALLOC_INFO)]] void* AllocateArray(const Size objectCount, const Size objectSize, const Alignment& alignment)
    {
        return m_LinearAllocator.AllocateArray(objectCount, objectSize, alignment);
    }

    [[nodiscard(NO_DISCARD_ALLOC_INFO)]] void* AllocateArray(const Size objectCount)
    {
        return AllocateArray(objectCount, sizeof(Object), AlignOf(alignof(Object)));
    }
    /**
     * @brief Resets the allocator to its initial state. Any further allocations
     * will possibly overwrite all object allocated prior to calling this method.
     * So make sure to only call this when you don't need any objects previously
     * allocated by this allocator.
     *
     */
    inline void Reset() { m_LinearAllocator.Reset(); }

    [[nodiscard]] Size        GetUsedSize() const { return m_LinearAllocator.GetUsedSize(); }
    [[nodiscard]] Size        GetTotalSize() const { return m_LinearAllocator.GetTotalSize(); }
    [[nodiscard]] std::string GetDebugName() const { return m_LinearAllocator.GetDebugName(); }

  private:
    LinearAllocator<policy> m_LinearAllocator;
};
} // namespace Memarena