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
#include "Source/Traits.hpp"
#include "Source/Utility/Alignment/Alignment.hpp"

namespace Memarena
{

template <typename T>
class PoolPtr : public Ptr<T>
{
    // Allow only StackAllocator to create a StackPtr by making constructors private
    template <PoolAllocatorPolicy policy>
    friend class PoolAllocator;

  private:
    inline explicit PoolPtr(T* ptr) : Ptr<T>(ptr) {}
};

template <typename T>
class PoolArrayPtr : public ArrayPtr<T>
{
    // Allow only StackAllocator to create a StackPtr by making constructors private
    template <PoolAllocatorPolicy policy>
    friend class PoolAllocator;

  private:
    inline explicit PoolArrayPtr(T* ptr, Size count) : ArrayPtr<T>(ptr, count) {}
};

#ifdef MEMARENA_DEBUG
constexpr PoolAllocatorPolicy defaultPoolAllocatorPolicy = PoolAllocatorPolicy::Default;
#else
constexpr PoolAllocatorPolicy defaultPoolAllocatorPolicy = PoolAllocatorPolicy::Release;
#endif

namespace Internal
{
struct Chunk
{
    Chunk* nextChunk;
};
} // namespace Internal

template <PoolAllocatorPolicy policy = defaultPoolAllocatorPolicy>
class PoolAllocator : public Allocator
{
    template <PoolAllocatorPolicy pmrPolicy>
    friend class PoolAllocatorPMR;

  private:
    static constexpr bool IsNullDeallocCheckEnabled    = PolicyContains(policy, PoolAllocatorPolicy::NullDeallocCheck);
    static constexpr bool IsAllocationSizeCheckEnabled = PolicyContains(policy, PoolAllocatorPolicy::AllocationSizeCheck);
    static constexpr bool IsSizeCheckEnabled           = PolicyContains(policy, PoolAllocatorPolicy::SizeCheck);
    static constexpr bool IsOwnershipCheckEnabled      = PolicyContains(policy, PoolAllocatorPolicy::OwnershipCheck);
    static constexpr bool IsUsageTrackingEnabled       = PolicyContains(policy, PoolAllocatorPolicy::SizeTracking);
    static constexpr bool IsGrowable                   = PolicyContains(policy, PoolAllocatorPolicy::Growable);
    static constexpr bool IsMultithreaded              = PolicyContains(policy, PoolAllocatorPolicy::Multithreaded);
    static constexpr bool IsAllocationTrackingEnabled  = PolicyContains(policy, PoolAllocatorPolicy::AllocationTracking);

    using ThreadPolicy = MultithreadedPolicy<IsMultithreaded, IsGrowable>;
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
        MEMARENA_ASSERT(objectSize >= sizeof(Chunk),
                        "Error: Object size must be larger than or equal the pointer size (%u) for the allocator '%s'", sizeof(void*),
                        GetDebugName().c_str());
    }

    ~PoolAllocator()
    {
        for (auto ptr : m_BlockPtrs)
        {
            m_BaseAllocator->DeallocateBase(ptr);
        };
    }

    template <Allocatable Object, typename... Args>
    NO_DISCARD PoolPtr<Object> New(Args&&... argList)
    {
        if constexpr (IsAllocationSizeCheckEnabled)
        {
            MEMARENA_ASSERT(m_ObjectSize == sizeof(Object),
                            "Error: Object size (%u) is not equal to the size specified at initialization (%u) for the allocator '%s'",
                            sizeof(Object), m_ObjectSize, GetDebugName().c_str());
        }

        void*   voidPtr = AllocateInternal();
        Object* ptr     = static_cast<Object*>(voidPtr);
        return PoolPtr<Object>(std::construct_at(ptr, std::forward<Args>(argList)...));
    }

    template <Allocatable Object, typename... Args>
    NO_DISCARD PoolArrayPtr<Object> NewArray(const Size objectCount, Args&&... argList)
    {
        if constexpr (IsAllocationSizeCheckEnabled)
        {
            MEMARENA_ASSERT(m_ObjectSize == sizeof(Object), // NOLINT
                            "Error: Object size (%u) is not equal to the size specified at initialization (%u) for the allocator '%s'",
                            sizeof(Object), m_ObjectSize, GetDebugName().c_str());
        }

        void*   voidPtr = AllocateArrayInternal(objectCount);
        Object* ptr     = static_cast<Object*>(voidPtr);
        return PoolArrayPtr<Object>(Internal::ConstructArray<Object>(ptr, objectCount, std::forward<Args>(argList)...), objectCount);
    }

    template <Allocatable Object>
    void Delete(PoolPtr<Object> ptr)
    {
        if constexpr (IsAllocationSizeCheckEnabled)
        {
            MEMARENA_ASSERT(m_ObjectSize == sizeof(Object),
                            "Error: Object size (%u) is not equal to the size specified at initialization (%u) for the allocator '%s'",
                            sizeof(Object), m_ObjectSize, GetDebugName().c_str());
        }
        DeallocateInternal(ptr.GetPtr()); // NOLINT
        ptr->~Object();
    }
    template <Allocatable Object>
    void DeleteArray(PoolArrayPtr<Object> ptr)
    {
        if constexpr (IsAllocationSizeCheckEnabled)
        {
            MEMARENA_ASSERT(m_ObjectSize == sizeof(Object),
                            "Error: Object size (%u) is not equal to the size specified at initialization (%u) for the allocator '%s'",
                            sizeof(Object), m_ObjectSize, GetDebugName().c_str());
        }
        DeallocateArrayInternal(ptr.GetPtr(), ptr.GetCount());
        std::destroy_n(ptr.GetPtr(), ptr.GetCount());
    }

    NO_DISCARD PoolPtr<void> Allocate(const std::string& category = "", const SourceLocation& sourceLocation = SourceLocation::current())
    {
        return PoolPtr<void>(AllocateInternal(category, sourceLocation));
    }

    NO_DISCARD PoolArrayPtr<void> AllocateArray(const Size objectCount, const std::string& category = "",
                                                const SourceLocation& sourceLocation = SourceLocation::current())
    {
        return PoolArrayPtr<void>(AllocateArrayInternal(objectCount, category, sourceLocation), objectCount);
    }

    void DeallocateArray(PoolArrayPtr<void> ptr) { DeallocateArrayInternal(ptr.GetPtr(), ptr.GetCount()); }

    template <typename Object>
    NO_DISCARD PoolPtr<void> Allocate(const std::string& category = "", const SourceLocation& sourceLocation = SourceLocation::current())
    {
        return Allocate(category, sourceLocation);
    }

    void Deallocate(PoolPtr<void> ptr) { DeallocateInternal(ptr.GetPtr()); }

    [[nodiscard]] Size GetObjectSize() const { return m_ObjectSize; }

  private:
    NO_DISCARD
    void* AllocateInternal(const std::string& category = "", const SourceLocation& sourceLocation = SourceLocation::current())
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

        void*  freePtr      = m_CurrentPtr;
        Chunk* currentChunk = std::bit_cast<Chunk*>(m_CurrentPtr);
        m_CurrentPtr        = currentChunk->nextChunk;

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

    void DeallocateInternal(void* ptr)
    {
        LockGuard<Mutex> guard(m_MultithreadedPolicy.m_Mutex);

        CheckPtr(ptr);

        Chunk* chunk     = std::bit_cast<Chunk*>(ptr);
        chunk->nextChunk = std::bit_cast<Chunk*>(m_CurrentPtr);
        m_CurrentPtr     = ptr;

        if constexpr (IsAllocationTrackingEnabled)
        {
            AddDeallocation();
        }

        if constexpr (IsUsageTrackingEnabled)
        {
            DecreaseUsedSize(m_ObjectSize);
        }
    }

    NO_DISCARD void* AllocateArrayInternal(const Size objectCount, const std::string& category = "",
                                           const SourceLocation& sourceLocation = SourceLocation::current())
    {
        if constexpr (IsSizeCheckEnabled)
        {
            MEMARENA_ASSERT(objectCount <= m_ObjectsPerBlock,
                            "Error: Allocation object count (%u) must be <= to objects per block (%u) for allocator '%s'!\n", objectCount,
                            m_BlockSize, GetDebugName().c_str());
        }

        LockGuard<Mutex> guard(m_MultithreadedPolicy.m_Mutex);

        Chunk* currentChunk           = std::bit_cast<Chunk*>(m_CurrentPtr);
        Chunk* startingChunk          = currentChunk;
        Size   consecutiveChunksFound = 1;

        while (consecutiveChunksFound < objectCount)
        {

            if constexpr (IsGrowable)
            {
                if (currentChunk == nullptr)
                {
                    AllocateBlock();
                    // We know for sure that the newly allocated block has the required number of consecutive chunks
                    // So we can just return the starting pointer of the new block
                    startingChunk = std::bit_cast<Chunk*>(m_CurrentPtr);
                    break;
                }
            }
            else if constexpr (IsSizeCheckEnabled)
            {
                MEMARENA_ASSERT(m_CurrentPtr != nullptr, "Error: The allocator '%s' is out of memory!\n", GetDebugName().c_str());
            }

            const UIntPtr nextChunkAddress       = std::bit_cast<UIntPtr>(currentChunk->nextChunk);
            const UIntPtr proceedingChunkAddress = std::bit_cast<UIntPtr>(currentChunk) + m_ObjectSize;
            if (nextChunkAddress == proceedingChunkAddress)
            {
                consecutiveChunksFound++;
            }
            else
            {
                startingChunk          = currentChunk->nextChunk;
                consecutiveChunksFound = 1;
            }
            currentChunk = currentChunk->nextChunk;
        }

        if constexpr (IsAllocationTrackingEnabled)
        {
            AddAllocation(m_ObjectSize * objectCount, category, sourceLocation);
        }

        if constexpr (IsUsageTrackingEnabled)
        {
            IncreaseUsedSize(m_ObjectSize * objectCount);
        }

        const UIntPtr startingAddress = std::bit_cast<UIntPtr>(startingChunk);
        if (startingAddress == std::bit_cast<UIntPtr>(m_CurrentPtr))
        {
            const UIntPtr endAddress = startingAddress + m_ObjectSize * objectCount;
            m_CurrentPtr             = std::bit_cast<void*>(endAddress);
        }

        return std::bit_cast<void*>(startingChunk);
    }

    void DeallocateArrayInternal(void* ptr, Size objectCount)
    {
        LockGuard<Mutex> guard(m_MultithreadedPolicy.m_Mutex);

        CheckPtr(ptr);

        const UIntPtr startAddress = std::bit_cast<UIntPtr>(ptr);
        const UIntPtr lastAddress  = startAddress + m_ObjectSize * (objectCount - 1);

        Chunk* lastChunk     = std::bit_cast<Chunk*>(lastAddress);
        lastChunk->nextChunk = std::bit_cast<Chunk*>(m_CurrentPtr);
        m_CurrentPtr         = ptr;

        if constexpr (IsAllocationTrackingEnabled)
        {
            AddDeallocation();
        }

        if constexpr (IsUsageTrackingEnabled)
        {
            DecreaseUsedSize(m_ObjectSize * objectCount);
        }
    }

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

    inline void CheckPtr(void* ptr)
    {
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