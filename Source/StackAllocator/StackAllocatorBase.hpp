#pragma once

#include <memory>
#include <string>

#include "Source/TypeAliases.hpp"

namespace Memarena
{

class MemoryManager;
struct AllocatorData;

/**
 * @brief A custom memory allocator which allocates in a stack-like manner.
 * All the memory will be allocated up-front. This means it will have
 * zero allocations during runtime. This also means that it will take the same
 * amount of memory whether it is full or empty. Allocations and Deallocations
 * also need to be done in a stack-like manner. It is the responsibility of the
 * user to make sure that deallocations happen in an order that is the reverse
 * of the allocation order. If a pointer p1 that was not allocated last is deallocated,
 * future allocations will overwrite the memory of all allocations that were made
 * between the allocation and deallocation of p1.
 *
 * Space complexity is O(N*H) --> O(N) where H is the Header size and N is the number of allocations
 * Allocation and deallocation complexity: O(1)
 */
class StackAllocatorBase
{
  public:
    /**
     * @brief Resets the allocator to its initial state. Any further allocations
     * will possibly overwrite all object allocated prior to calling this method.
     * So make sure to only call this when you don't need any objects previously
     * allocated by this allocator.
     *
     */
    void Reset();

    Size        GetUsedSize() const;
    Size        GetTotalSize() const;
    std::string GetDebugName() const;

  protected:
    /**
     * @brief Constructs a new Stack Allocator object. This is where the entire memory of the allocator
     * is allocated. If a Memarena instance is provided, it registers itself to the manager to
     * allow memory tracking.
     *
     * @param totalSize This will be allocated up-front. Even if the stack allocator is empty, it will
     * consume this amount of memory.
     * @param memoryManager The Memarena instance that will keep track of this allocator's memory.
     * If no value or a nullptr is passed, this allocator's memory won't be tracked
     * @param debugName The name that will appear in logs and any editor.
     */
    StackAllocatorBase(const Size totalSize, const std::shared_ptr<MemoryManager> memoryManager, const std::string& debugName);

    /**
     * @brief Destroys the Stack Allocator object. Also frees up all the memory. If a Memarena instance
     * is provided, it un-registers itself from the manager.
     */
    ~StackAllocatorBase();

    void SetCurrentOffset(Offset offset);

    bool OwnsAddress(UIntPtr address);

  protected:
    std::shared_ptr<MemoryManager> m_MemoryManager; // Pointer to the memory manager that this allocator will report the memory usage to
    std::shared_ptr<AllocatorData> m_Data;
    void*                          m_StartPtr{nullptr};
    UIntPtr                        m_StartAddress;
    UIntPtr                        m_EndAddress;
    Offset                         m_CurrentOffset;
};

} // namespace Memarena