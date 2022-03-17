#pragma once

#include "Source/Allocator.hpp"

/**
 * @brief A templated allocator that can only be used to allocate memory for a
 * collection of the same type.
 *
 * @tparam Object
 */
template <typename Object, >
class PoolAllocator
{
  public:
    PoolAllocator()                     = delete;
    PoolAllocator(PoolAllocator&)       = delete;
    PoolAllocator(const PoolAllocator&) = delete;
    PoolAllocator(PoolAllocator&&)      = delete;
    PoolAllocator& operator=(const PoolAllocator&) = delete;
    PoolAllocator& operator=(PoolAllocator&&) = delete;

    explicit PoolAllocator(const Size totalSize, const std::string& debugName = "StackAllocator") : m_StackAllocator(totalSize, debugName)
    {
    }

    ~PoolAllocator() = default;

    /**
     * @brief Gets an address in the pool, constructs the object at the address and returns the address
     *
     * @return Object* The pointer to the newly allocated memory
     */
    void* Allocate();

    /**
     * @brief Allocates a new block of memory and calls the constructor
     * @details Allocation complexity is O(1)
     *
     * @tparam Object The type to be created
     * @tparam Args Variadic arguments
     * @param argList The arguments to the constructor of the type Object
     * @return Object* The pointer to the newly allocated and created object
     */
    template <typename... Args>
    Object* New(Args... argList)
    {
        void* address = Allocate();              // Allocate the raw memory and get a pointer to it
        return new (address) Object(argList...); // Call the placement new operator, which constructs the Object
    }

    /**
     * @brief Deallocates raw memory without calling any destructor
     * @details Deallocation complexity is O(1)
     *
     * @param ptr The pointer to the memory to be deallocated
     */
    void Deallocate(Object* ptr);

    /**
     * @brief Deallocates a pointer and calls the destructor
     * @details Deallocation complexity is O(1)
     *
     * @tparam Object The type of the passed pointer
     * @param ptr The pointer to the memory to be deallocated
     */
    void Delete(Object* ptr);

    inline Size GetUsedSize() const { return m_Data->UsedSize; }

  private:
    PoolAllocator(PoolAllocator&);
    Chunk* AllocateBlock(Size chunkSize);

  private:
    // Declaration order is important
    std::shared_ptr<AllocatorData> m_Data;

    Size m_BlockSize;
    Size m_ObjectSize;

    Chunk*              m_CurrentPtr = nullptr;
    std::vector<Chunk*> m_AllocatedBlocks;
};

template <typename Object, ResizePolicy Policy>
PoolAllocator<Object, Policy>::PoolAllocator(const char* debugName, Size blockSize)
    : m_Data(std::make_shared<AllocatorData>(debugName, 0)), m_BlockSize(blockSize), m_ObjectSize(sizeof(Object)),
      m_CurrentPtr(AllocateBlock(m_ObjectSize))
{
    QMBT_CORE_ASSERT(blockSize > 0, "Block size has to be more than 0!");

    MemoryTracker::GetInstance().Register(m_Data);

    m_AllocatedBlocks.push_back(m_CurrentPtr);
}

template <typename Object, ResizePolicy Policy>
PoolAllocator<Object, Policy>::~PoolAllocator()
{
    MemoryTracker::GetInstance().UnRegister(m_Data);
    for (auto& ptr : m_AllocatedBlocks)
    {
        free(ptr);
    }
}

template <typename Object, ResizePolicy Policy>
void* PoolAllocator<Object, Policy>::Allocate()
{

    // No chunks left in the current block, or no block
    // exists yet, Allocate a new one. If resize policy is fixed,
    // then log an error.
    if (m_CurrentPtr == nullptr)
    {
        if constexpr (Policy == ResizePolicy::Fixed)
        {
            LOG_CORE_ERROR("{0} out of memory!", m_Data->DebugName);
        }
        else
        {
            m_CurrentPtr = AllocateBlock(m_ObjectSize);
            m_AllocatedBlocks.push_back(m_CurrentPtr);
        }
    }

    // The return value is the current position of
    // the allocation pointer:
    Chunk* freeChunk = m_CurrentPtr;

    // Advance (bump) the allocation pointer to the next chunk.
    // When no chunks left, the `m_CurrentPtr` will be set to `nullptr`, and
    // this will cause allocation of a new block on the next request:
    m_CurrentPtr = m_CurrentPtr->next;

    m_Data->UsedSize += m_ObjectSize;
    LOG_CORE_INFO("{0} Allocated {1} bytes", m_Data->DebugName, m_ObjectSize);

    return freeChunk;
}
template <typename Object, ResizePolicy Policy>
void PoolAllocator<Object, Policy>::Deallocate(Object* ptr)
{
    // The freed chunk's next pointer points to the
    // current allocation pointer:
    reinterpret_cast<Chunk*>(ptr)->next = m_CurrentPtr;

    // And the allocation pointer is now set
    // to the returned (free) chunk:

    m_CurrentPtr = reinterpret_cast<Chunk*>(ptr);

    m_Data->UsedSize -= m_ObjectSize;
    LOG_CORE_INFO("{0} Deallocated {1} bytes", m_Data->DebugName, m_ObjectSize);
}

template <typename Object, ResizePolicy Policy>
void PoolAllocator<Object, Policy>::Delete(Object* ptr)
{
    ptr->~Object();  // Call the destructor on the object
    Deallocate(ptr); // Deallocate the pointer
}

template <typename Object, ResizePolicy Policy>
Chunk* PoolAllocator<Object, Policy>::AllocateBlock(Size chunkSize)
{
    QMBT_CORE_ASSERT(chunkSize > sizeof(Chunk), "Object size must be larger than pointer size");

    // The total memory (in Bytes), to be allocated
    Size blockSize = m_BlockSize * chunkSize;

    // The first chunk of the new block
    Chunk* blockBegin = reinterpret_cast<Chunk*>(malloc(blockSize));

    m_Data->TotalSize += blockSize;
    MemoryTracker::GetInstance().UpdateTotalSize(blockSize);

    // Once the block is allocated, we need to chain all
    // the chunks in this block:

    Chunk* chunk = blockBegin;

    for (int i = 0; i < m_BlockSize - 1; ++i)
    {
        chunk->next = reinterpret_cast<Chunk*>(reinterpret_cast<char*>(chunk) + chunkSize);
        chunk       = chunk->next;
    }

    chunk->next = nullptr;

    LOG_CORE_INFO("{0} Allocated block ({1} chunks)", m_Data->DebugName, m_BlockSize);

    return blockBegin;
}