#pragma once

#include <bit>
#include <experimental/source_location>

#include "Source/Allocator.hpp"
#include "Source/AllocatorData.hpp"
#include "Source/AllocatorUtils.hpp"
#include "Source/Assert.hpp"
#include "Source/Macros.hpp"
#include "Source/Policies/BoundsCheckPolicy.hpp"
#include "Source/Policies/MultithreadedPolicy.hpp"
#include "Source/Policies/Policies.hpp"
#include "Source/Utility/Alignment/Alignment.hpp"

namespace Memarena
{

namespace Internal
{
struct Chunk
{
    Chunk* nextChunk;
};
} // namespace Internal

template <PoolAllocatorPolicy policy = PoolAllocatorPolicy::Default>
class PoolAllocator : public Allocator
{
  private:
    static constexpr bool IsNullDeallocCheckEnabled   = PolicyContains(policy, PoolAllocatorPolicy::NullDeallocCheck);
    static constexpr bool IsNewSizeCheckEnabled       = PolicyContains(policy, PoolAllocatorPolicy::NewSizeCheck);
    static constexpr bool IsSizeCheckEnabled          = PolicyContains(policy, PoolAllocatorPolicy::SizeCheck);
    static constexpr bool IsOwnershipCheckEnabled     = PolicyContains(policy, PoolAllocatorPolicy::OwnershipCheck);
    static constexpr bool IsUsageTrackingEnabled      = PolicyContains(policy, PoolAllocatorPolicy::SizeTracking);
    static constexpr bool IsGrowable                  = PolicyContains(policy, PoolAllocatorPolicy::Growable);
    static constexpr bool IsMultithreaded             = PolicyContains(policy, PoolAllocatorPolicy::Multithreaded);
    static constexpr bool IsAllocationTrackingEnabled = PolicyContains(policy, PoolAllocatorPolicy::AllocationTracking);

    using ThreadPolicy = MultithreadedPolicy<IsMultithreaded>;
    using Chunk        = Internal::Chunk;

    template <typename SyncPrimitive>
    using LockGuard = typename ThreadPolicy::template LockGuard<SyncPrimitive>;
    using Mutex     = typename ThreadPolicy::Mutex;

  public:
    // Prohibit default construction, moving and assignment
    PoolAllocator()                     = delete;
    PoolAllocator(PoolAllocator&)       = delete;
    PoolAllocator(const PoolAllocator&) = delete;
    PoolAllocator(PoolAllocator&&)      = delete;
    PoolAllocator& operator=(const PoolAllocator&) = delete;
    PoolAllocator& operator=(PoolAllocator&&) = delete;

    explicit PoolAllocator(const Size objectSize, const Size objectsPerBlock, const std::string& debugName = "PoolAllocator",
                           std::shared_ptr<Allocator> baseAllocator = Allocator::GetDefaultAllocator())
        : Allocator(0, debugName), m_ObjectSize(objectSize), m_ObjectsPerBlock(objectsPerBlock), m_BlockSize(objectSize * objectsPerBlock),
          m_BaseAllocator(std::move(baseAllocator))
    {
        AllocateBlock();
        MEMARENA_ASSERT(objectSize > sizeof(Chunk), "Object size must be larger than the pointer size (%u) for the allocator '%s'",
                        sizeof(void*), GetDebugName().c_str())
    }

    ~PoolAllocator()
    {
        for (auto ptr : m_BlockPtrs)
        {
            m_BaseAllocator->DeallocateBase(ptr);
        };
    }

    template <typename Object, typename... Args>
    NO_DISCARD Object* New(Args&&... argList)
    {
        if constexpr (IsNewSizeCheckEnabled)
        {
            MEMARENA_ASSERT(m_ObjectSize == sizeof(Object),
                            "Object size (%u) is not equal to the size specified at initialization (%u) for the allocator '%s'",
                            sizeof(Object), m_ObjectSize, GetDebugName().c_str())
        }
        void* voidPtr = Allocate();
        return new (voidPtr) Object(std::forward<Args>(argList)...);
    }

    template <typename Object>
    void Delete(Object* ptr)
    {
        Deallocate(ptr);
        ptr->~Object();
    }

    NO_DISCARD void* Allocate(const std::string& category = "", const SourceLocation& sourceLocation = SourceLocation::current())
    {
        LockGuard<Mutex> guard(m_MultithreadedPolicy.m_Mutex);

        if constexpr (IsGrowable)
        {
            if (m_CurrentPtr == nullptr)
            {
                AllocateBlock();
            }
        }
        else if constexpr (IsSizeCheckEnabled)
        {
            MEMARENA_ASSERT(m_CurrentPtr != nullptr, "Error: The allocator '%s' is out of memory!\n", GetDebugName().c_str());
        }

        void* freePtr = m_CurrentPtr;

        m_CurrentPtr = std::bit_cast<Chunk*>(m_CurrentPtr)->nextChunk;

        if constexpr (IsAllocationTrackingEnabled)
        {
            AddAllocation(m_ObjectSize, category, sourceLocation);
        }

        if constexpr (IsUsageTrackingEnabled)
        {
            IncreaseUsedSize(m_ObjectSize);
        }

        return freePtr;
    }

    template <typename Object>
    NO_DISCARD void* Allocate(const std::string& category = "", const SourceLocation& sourceLocation = SourceLocation::current())
    {
        return Allocate(category, sourceLocation);
    }

    void Deallocate(void* ptr)
    {
        LockGuard<Mutex> guard(m_MultithreadedPolicy.m_Mutex);

        if constexpr (IsNullDeallocCheckEnabled)
        {
            MEMARENA_ASSERT(ptr, "Error: Cannot deallocate nullptr in allocator %s!\n", GetDebugName().c_str());
        }

        const UIntPtr address = std::bit_cast<UIntPtr>(ptr);

        if constexpr (IsOwnershipCheckEnabled)
        {
            MEMARENA_ASSERT(OwnsAddress(address), "Error: The allocator %s does not own the pointer %d!\n", GetDebugName().c_str(),
                            address);
        }

        std::bit_cast<Chunk*>(ptr)->nextChunk = std::bit_cast<Chunk*>(m_CurrentPtr);

        if constexpr (IsAllocationTrackingEnabled)
        {
            AddDeallocation();
        }

        m_CurrentPtr = std::bit_cast<Chunk*>(ptr);
    }

  private:
    void AllocateBlock()
    {
        // The first chunk of the new block
        Internal::BaseAllocatorPtr<void> newBlockPtr = m_BaseAllocator->AllocateBase(m_BlockSize);

        // Once the block is allocated, we need to chain all the chunks in this block:
        Chunk* currentChunk = std::bit_cast<Chunk*>(newBlockPtr.GetPtr());

        for (int i = 0; i < m_ObjectsPerBlock - 1; ++i)
        {
            currentChunk->nextChunk = std::bit_cast<Chunk*>(std::bit_cast<UIntPtr>(currentChunk) + m_ObjectSize);
            currentChunk            = currentChunk->nextChunk;
        }

        currentChunk->nextChunk = nullptr;

        m_BlockPtrs.push_back(newBlockPtr);

        if constexpr (IsUsageTrackingEnabled)
        {
            SetUsedSize((m_BlockPtrs.size() - 1) * m_BlockSize);
        }

        UpdateTotalSize();

        m_CurrentPtr = newBlockPtr.GetPtr();
    }

    inline void DeallocateBlocks()
    {
        if constexpr (IsGrowable)
        {
            while (m_BlockPtrs.size() > 1)
            {
                FreeLastBlock();
            }
            UpdateTotalSize();
        }

        m_CurrentPtr = m_BlockPtrs[0].GetPtr();
    }

    inline void FreeLastBlock()
    {
        m_BaseAllocator->DeallocateBase(m_BlockPtrs.back());
        m_BlockPtrs.pop_back();
    }

    inline void UpdateTotalSize()
    {
        if constexpr (IsUsageTrackingEnabled)
        {
            SetTotalSize(m_BlockPtrs.size() * m_BlockSize);
        }
    }

    [[nodiscard]] bool OwnsAddress(UIntPtr address) const
    {
        UIntPtr startAddress = std::bit_cast<UIntPtr>(m_BlockPtrs[0].GetPtr());
        UIntPtr endAddress   = std::bit_cast<UIntPtr>(m_BlockPtrs.back().GetPtr()) + m_BlockSize;

        return address >= startAddress && address <= endAddress;
    }

    std::shared_ptr<Allocator>                    m_BaseAllocator;
    std::vector<Internal::BaseAllocatorPtr<void>> m_BlockPtrs;

    ThreadPolicy m_MultithreadedPolicy;

    void* m_CurrentPtr = nullptr;

    Size m_ObjectsPerBlock;
    Size m_ObjectSize;
    Size m_BlockSize;
};

// template <PoolAllocatorPolicy policy>
// function(PoolAllocator<>)->function(PoolAllocator<policy>);
} // namespace Memarena