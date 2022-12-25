#pragma once

#include "Source/PCH.hpp"

#include "Source/Allocator.hpp"
#include "Source/AllocatorData.hpp"
#include "Source/AllocatorSettings.hpp"
#include "Source/AllocatorUtils.hpp"
#include "Source/Assert.hpp"
#include "Source/Macros.hpp"
#include "Source/Policies/BoundsCheckPolicy.hpp"
#include "Source/Policies/MultithreadedPolicy.hpp"
#include "Source/Policies/Policies.hpp"
#include "Source/Traits.hpp"
#include "Source/Utility/Alignment/Alignment.hpp"

#define MEMARENA_CHECK_ALLOCATION_SIZE(size, returnValue)                                                                                  \
    if constexpr (AllocationSizeCheckIsEnabled)                                                                                            \
    {                                                                                                                                      \
        MEMARENA_ASSERT_RETURN(m_ObjectSize == size, returnValue,                                                                          \
                               "Error: Object size (%u) is not equal to the size specified at initialization (%u) for the allocator '%s'", \
                               size, m_ObjectSize, GetDebugName().c_str());                                                                \
    }

namespace Memarena
{

using PoolAllocatorSettings = AllocatorSettings<PoolAllocatorPolicy>;
constexpr PoolAllocatorSettings poolAllocatorDefaultSettings{};

template <typename T>
class PoolPtr : public Ptr<T>
{
    // Allow only StackAllocator to create a StackPtr by making constructors private
    template <PoolAllocatorSettings Settings>
    friend class PoolAllocator;

  private:
    inline explicit PoolPtr(T* ptr) : Ptr<T>(ptr) {}
};

template <typename T>
class PoolArrayPtr : public ArrayPtr<T>
{
    // Allow only StackAllocator to create a StackPtr by making constructors private
    template <PoolAllocatorSettings Settings>
    friend class PoolAllocator;

  private:
    inline explicit PoolArrayPtr(T* ptr, Size count) : ArrayPtr<T>(ptr, count) {}
    template <typename U>
    inline explicit PoolArrayPtr(PoolArrayPtr<U> ptr) : ArrayPtr<T>(ptr.GetPtr(), ptr.GetCount())
    {
    }
};

namespace Internal
{
struct Chunk
{
    Chunk* nextChunk;
};
} // namespace Internal

template <PoolAllocatorSettings Settings = poolAllocatorDefaultSettings>
class PoolAllocator : public Allocator
{
    template <PoolAllocatorSettings PMRSettings>
    friend class PoolAllocatorPMR;

  private:
    static constexpr auto Policy = Settings.policy;

    static constexpr bool NullDeallocCheckIsEnabled     = PolicyContains(Policy, PoolAllocatorPolicy::NullDeallocCheck);
    static constexpr bool DoubleFreePreventionIsEnabled = PolicyContains(Policy, PoolAllocatorPolicy::DoubleFreePrevention);
    static constexpr bool AllocationSizeCheckIsEnabled  = PolicyContains(Policy, PoolAllocatorPolicy::AllocationSizeCheck);
    static constexpr bool SizeCheckIsEnabled            = PolicyContains(Policy, PoolAllocatorPolicy::SizeCheck);
    static constexpr bool OwnershipIsCheckEnabled       = PolicyContains(Policy, PoolAllocatorPolicy::OwnershipCheck);
    static constexpr bool UsageTrackingIsEnabled        = PolicyContains(Policy, PoolAllocatorPolicy::SizeTracking);
    static constexpr bool IsGrowable                    = PolicyContains(Policy, PoolAllocatorPolicy::Growable);
    static constexpr bool IsMultithreaded               = PolicyContains(Policy, PoolAllocatorPolicy::Multithreaded);
    static constexpr bool AllocationTrackingIsEnabled   = PolicyContains(Policy, PoolAllocatorPolicy::AllocationTracking);

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
        MEMARENA_ASSERT(objectSize >= sizeof(Chunk), "Error: Object size must be >= to the pointer size (%u) for the allocator '%s'",
                        sizeof(void*), GetDebugName().c_str());
        MEMARENA_ASSERT(objectsPerBlock > 0, "Error: Objects per block must be greater than 0 for the allocator '%s'",
                        GetDebugName().c_str());
        AllocateBlock();
    }

    ~PoolAllocator()
    {
        for (void* ptr : m_BlockPtrs)
        {
            m_BaseAllocator->DeallocateBase(ptr);
        };
    }

    template <Allocatable Object, typename... Args>
    NO_DISCARD Object* NewRaw(Args&&... argList)
    {
        MEMARENA_CHECK_ALLOCATION_SIZE(sizeof(Object), nullptr);
        void* voidPtr = AllocateInternal();
        RETURN_IF_NULLPTR(voidPtr);
        Object* ptr = static_cast<Object*>(voidPtr);
        return std::construct_at(ptr, std::forward<Args>(argList)...);
    }

    template <Allocatable Object, typename... Args>
    NO_DISCARD PoolPtr<Object> New(Args&&... argList)
    {
        return PoolPtr<Object>(NewRaw<Object>(std::forward<Args>(argList)...));
    }

    template <Allocatable Object, typename... Args>
    NO_DISCARD PoolArrayPtr<Object> NewArray(const Size objectCount, Args&&... argList)
    {
        MEMARENA_CHECK_ALLOCATION_SIZE(sizeof(Object), PoolArrayPtr<Object>(nullptr, 0));
        void* voidPtr = AllocateArrayInternal(objectCount);
        RETURN_VAL_IF_NULLPTR(voidPtr, PoolArrayPtr<Object>(nullptr, 0));
        Object* ptr = static_cast<Object*>(voidPtr);
        return PoolArrayPtr<Object>(Internal::ConstructArray<Object>(ptr, objectCount, std::forward<Args>(argList)...), objectCount);
    }

    template <Allocatable Object>
    void Delete(Object*& ptr)
    {
        MEMARENA_CHECK_ALLOCATION_SIZE(sizeof(Object), void());
        DeallocateInternal(ptr);
        ptr->~Object();
    }

    template <Allocatable Object>
    void Delete(PoolPtr<Object>& ptr)
    {
        MEMARENA_CHECK_ALLOCATION_SIZE(sizeof(Object), void());
        DeallocateInternal(ptr);
        ptr->~Object();
    }

    template <Allocatable Object>
    void DeleteArray(PoolArrayPtr<Object>& ptr)
    {
        MEMARENA_CHECK_ALLOCATION_SIZE(sizeof(Object), void());
        DeallocateInternal(ptr);
        std::destroy_n(ptr.GetPtr(), ptr.GetCount());
    }

    NO_DISCARD void* Allocate(const std::string& category = "", const SourceLocation& sourceLocation = SourceLocation::current())
    {
        return AllocateInternal(category, sourceLocation);
    }

    NO_DISCARD void* Allocate(const Size size, const std::string& category = "",
                              const SourceLocation& sourceLocation = SourceLocation::current())
    {
        MEMARENA_CHECK_ALLOCATION_SIZE(size, nullptr);
        return AllocateInternal(category, sourceLocation);
    }

    void Deallocate(void*& ptr) { DeallocateInternal(ptr); }

    [[nodiscard]] Size GetObjectSize() const { return m_ObjectSize; }

    [[nodiscard]] bool Owns(UIntPtr address) const
    {
        for (const auto& blockPtr : m_BlockPtrs)
        {
            const UIntPtr startAddress = std::bit_cast<UIntPtr>(blockPtr);
            const UIntPtr endAddress   = startAddress + m_BlockSize;
            if (address >= startAddress && address <= endAddress)
            {
                return true;
            }
        }
    }
    [[nodiscard]] bool Owns(void* ptr) const { return Owns(std::bit_cast<UIntPtr>(ptr)); }

  private:
    template <typename T>
    void DeallocateInternal(T*& ptr)
    {
        DeallocateVoidInternal(ptr);
        CheckDoubleFree(ptr);
    }

    template <typename T>
    void DeallocateInternal(PoolPtr<T>& ptr)
    {
        DeallocateVoidInternal(ptr.GetPtr());
        CheckDoubleFree(ptr);
    }

    template <typename T>
    void DeallocateInternal(PoolArrayPtr<T>& ptr)
    {
        DeallocateArrayInternal(ptr.GetPtr(), ptr.GetCount());
        CheckDoubleFree(ptr);
    }

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
        else if constexpr (SizeCheckIsEnabled)
        {
            MEMARENA_ASSERT_RETURN(m_CurrentPtr != nullptr, nullptr, "Error: The allocator '%s' is out of memory!\n",
                                   GetDebugName().c_str());
        }

        void*  freePtr      = m_CurrentPtr;
        Chunk* currentChunk = std::bit_cast<Chunk*>(m_CurrentPtr);
        m_CurrentPtr        = currentChunk->nextChunk;

        if constexpr (AllocationTrackingIsEnabled)
        {
            AddAllocation(m_ObjectSize, category, sourceLocation);
        }

        if constexpr (UsageTrackingIsEnabled)
        {
            IncreaseUsedSize(m_ObjectSize);
        }

        return freePtr;
    }

    NO_DISCARD void* AllocateArrayInternal(const Size objectCount, const std::string& category = "",
                                           const SourceLocation& sourceLocation = SourceLocation::current())
    {
        if constexpr (SizeCheckIsEnabled)
        {
            MEMARENA_ASSERT_RETURN(objectCount <= m_ObjectsPerBlock, nullptr,
                                   "Error: Allocation object count (%u) must be <= to objects per block (%u) for allocator '%s'!\n",
                                   objectCount, m_BlockSize, GetDebugName().c_str());
        }

        LockGuard<Mutex> guard(m_MultithreadedPolicy.m_Mutex);

        // To allocate an array, all the chunks must be consecutive. So we search the blocks and try to find such a sequence
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
            else if constexpr (SizeCheckIsEnabled)
            {
                MEMARENA_ASSERT_RETURN(m_CurrentPtr != nullptr, nullptr, "Error: The allocator '%s' is out of memory!\n",
                                       GetDebugName().c_str());
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

        if constexpr (AllocationTrackingIsEnabled)
        {
            AddAllocation(m_ObjectSize * objectCount, category, sourceLocation);
        }

        if constexpr (UsageTrackingIsEnabled)
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

    void DeallocateVoidInternal(void* ptr)
    {
        LockGuard<Mutex> guard(m_MultithreadedPolicy.m_Mutex);

        if (!CheckPtr(ptr))
        {
            return;
        }

        Chunk* chunk     = std::bit_cast<Chunk*>(ptr);
        chunk->nextChunk = std::bit_cast<Chunk*>(m_CurrentPtr);
        m_CurrentPtr     = ptr;

        if constexpr (AllocationTrackingIsEnabled)
        {
            AddDeallocation();
        }

        if constexpr (UsageTrackingIsEnabled)
        {
            DecreaseUsedSize(m_ObjectSize);
        }
    }

    void DeallocateArrayInternal(void* ptr, Size objectCount)
    {
        LockGuard<Mutex> guard(m_MultithreadedPolicy.m_Mutex);

        if (!CheckPtr(ptr))
        {
            return;
        }

        const UIntPtr startAddress = std::bit_cast<UIntPtr>(ptr);
        const UIntPtr lastAddress  = startAddress + m_ObjectSize * (objectCount - 1);

        Chunk* lastChunk     = std::bit_cast<Chunk*>(lastAddress);
        lastChunk->nextChunk = std::bit_cast<Chunk*>(m_CurrentPtr);
        m_CurrentPtr         = ptr;

        if constexpr (AllocationTrackingIsEnabled)
        {
            AddDeallocation();
        }

        if constexpr (UsageTrackingIsEnabled)
        {
            DecreaseUsedSize(m_ObjectSize * objectCount);
        }
    }

    void AllocateBlock()
    {
        // The first chunk of the new block
        void* newBlockPtr = m_BaseAllocator->AllocateBase(m_BlockSize);

        // Once the block is allocated, we need to chain all the chunks in this block:
        Chunk* currentChunk = std::bit_cast<Chunk*>(newBlockPtr);

        for (int i = 0; i < m_ObjectsPerBlock - 1; ++i)
        {
            currentChunk->nextChunk = std::bit_cast<Chunk*>(std::bit_cast<UIntPtr>(currentChunk) + m_ObjectSize);
            currentChunk            = currentChunk->nextChunk;
        }

        currentChunk->nextChunk = nullptr;

        m_BlockPtrs.push_back(newBlockPtr);

        if constexpr (UsageTrackingIsEnabled)
        {
            SetUsedSize((m_BlockPtrs.size() - 1) * m_BlockSize);
        }

        UpdateTotalSize();

        m_CurrentPtr = newBlockPtr;
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

        m_CurrentPtr = m_BlockPtrs[0];
    }

    inline bool CheckPtr(void* ptr)
    {
        if constexpr (NullDeallocCheckIsEnabled)
        {
            MEMARENA_ASSERT_RETURN(ptr != nullptr, false, "Error: Cannot deallocate nullptr in allocator %s!\n", GetDebugName().c_str());
        }

        const UIntPtr address = std::bit_cast<UIntPtr>(ptr);

        if constexpr (OwnershipIsCheckEnabled)
        {
            MEMARENA_ASSERT_RETURN(Owns(address), false, "Error: The allocator %s does not own the pointer %d!\n", GetDebugName().c_str(),
                                   address);
        }

        return true;
    }

    inline void FreeLastBlock()
    {
        m_BaseAllocator->DeallocateBase(m_BlockPtrs.back());
        m_BlockPtrs.pop_back();
    }

    inline void UpdateTotalSize()
    {
        if constexpr (UsageTrackingIsEnabled)
        {
            SetTotalSize(m_BlockPtrs.size() * m_BlockSize);
        }
    }

    template <typename T>
    inline void CheckDoubleFree(T*& ptr)
    {
        if constexpr (DoubleFreePreventionIsEnabled)
        {
            ptr = nullptr;
        }
    }

    template <typename T>
    inline void CheckDoubleFree(PoolPtr<T>& ptr)
    {
        if constexpr (DoubleFreePreventionIsEnabled)
        {
            ptr.Reset();
        }
    }

    template <typename T>
    inline void CheckDoubleFree(PoolArrayPtr<T>& ptr)
    {
        if constexpr (DoubleFreePreventionIsEnabled)
        {
            ptr.Reset();
        }
    }

    std::shared_ptr<Allocator> m_BaseAllocator;
    std::vector<void*>         m_BlockPtrs;

    ThreadPolicy m_MultithreadedPolicy;

    void* m_CurrentPtr = nullptr;

    Size m_ObjectsPerBlock;
    Size m_ObjectSize;
    Size m_BlockSize;
};

// template <PoolAllocatorPolicy policy>
// function(PoolAllocator<>)->function(PoolAllocator<policy>);
} // namespace Memarena