#pragma once

#include "Aliases.hpp"

namespace Memory
{

struct AllocatorData;
class MemoryManager;

struct AllocationHeader
{
    unsigned char padding;
};
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
class StackAllocator
{
  public:
    // Prohibit default construction, moving and assignment
    StackAllocator()                      = delete;
    StackAllocator(const StackAllocator&) = delete;
    StackAllocator(StackAllocator&&)      = delete;
    StackAllocator& operator=(const StackAllocator&) = delete;
    StackAllocator& operator=(StackAllocator&&) = delete;

    /**
     * @brief Construct a new Stack Allocator object.
     *
     * @param totalSize This will be allocated up-front. Even if the stack allocator is empty, it will
     * consume this amount of memory.
     * @param memoryManager The MemoryManager instance that will keep track of this allocator's memory.
     * If no value or a nullptr is passed, this allocator's memory won't be tracked
     * @param debugName The name that will appear in logs and any editor.
     */
    StackAllocator(const Size totalSize, std::shared_ptr<MemoryManager> memoryManager = nullptr, const char* debugName = "Allocator");

    ~StackAllocator();

    /**
     * @brief Allocates a new block of memory and calls the constructor
     * @details Speed complexity is O(1)
     *
     * @tparam Object The type to be created
     * @tparam Args Variadic arguments
     * @param argList The arguments to the constructor of the type "Object"
     * @return Object* The pointer to the newly allocated and created object
     */
    template <typename Object, typename... Args>
    Object* New(Args... argList);

    /**
     * @brief Deallocates a pointer and calls the destructor
     * @details Speed complexity is O(1)
     *
     * @tparam Object The type of the passed pointer
     * @param ptr The pointer to the memory to be deallocated
     */
    template <typename Object>
    void Delete(Object* ptr);

    /**
     * @brief Allocates raw memory without calling any constructor
     * @details Speed complexity is O(1)
     * Before:
     * ----------------------------------------------------------------------------
     * |... Used memory |Unused memory                                         ...|
     * ----------------------------------------------------------------------------
     * ^ m_HeadPtr      ^ m_Offset
     * After:
     * ----------------------------------------------------------------------------
     * |... Used memory |HEADER(in padding) |OBJECT        ...| Unused memory ...|
     * ----------------------------------------------------------------------------
     * ^ m_HeadPtr      ^ headerAddress     ^ currentAddress  ^ m_Offset
     *                                        (return value)
     *
     * @param size The size of the memory to be allocated in bytes
     * @param alignment The alignment of the memory to be allocated in bytes
     * @return void* The pointer to the newly allocated memory
     */
    void* Allocate(const Size size, const Size alignment = 8);

    /**
     * @brief Deallocates raw memory without calling any destructor
     * @details Speed complexity is O(1)
     * Before:
     * ----------------------------------------------------------------------------
     * |... Used memory |HEADER(in padding) |OBJECT        ...| Unused memory ...|
     * ----------------------------------------------------------------------------
     * ^ m_HeadPtr      ^ headerAddress     ^ currentAddress  ^ m_Offset
     *                                        (ptr parameter)
     * After:
     * ----------------------------------------------------------------------------
     * |... Used memory |Unused memory                                         ...|
     * ----------------------------------------------------------------------------
     * ^ m_HeadPtr      ^ m_Offset
     *
     * @param ptr The pointer to the memory to be deallocated
     */
    void Deallocate(void* ptr);

    /**
     * @brief Resets the allocator to its initial state. Any further allocations
     * will possibly overwrite all object allocated prior to calling this method.
     * So make sure to only call this when you don't need any objects previously
     * allocated by this allocator.
     *
     */
    void Clear();

    Size GetUsedSize() const;
    Size GetTotalSize() const;

  private:
    StackAllocator(StackAllocator& stackAllocator); // Restrict copying

  private:
    std::shared_ptr<MemoryManager> m_MemoryManager; // Pointer to the memory manager that this allocator will report the memory usage to
    std::shared_ptr<AllocatorData> m_Data;
    void*                          m_HeadPtr{nullptr}; // Points to the start of the stack allocator
    Size                           m_Offset{0};        // Points to the first available location

    struct AllocationHeader
    {
        UInt8 padding;

        AllocationHeader(UInt8 _padding) : padding(_padding) {}
    };
};

template <typename Object, typename... Args>
Object* StackAllocator::New(Args... argList)
{
    void* address = Allocate(sizeof(Object)); // Allocate the raw memory and get a pointer to it
    return new (address) Object(argList...);  // Call the placement new operator, which constructs the Object
}

template <typename Object>
void StackAllocator::Delete(Object* ptr)
{
    ptr->~Object();  // Call the destructor on the object
    Deallocate(ptr); // Deallocate the pointer
}
} // namespace Memory