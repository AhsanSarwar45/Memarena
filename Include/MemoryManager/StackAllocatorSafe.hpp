#pragma once

#include "TypeAliases.hpp"

#include "StackAllocatorBase.hpp"

namespace Memory
{

template <typename T>
struct StackPtr
{
    T*     ptr;
    UInt32 startOffset;
    UInt32 endOffset;

    T*       operator->() const { return ptr; }
    explicit operator bool() const noexcept { return (ptr != nullptr); }
             operator StackPtr<void>() const noexcept { return {.ptr = ptr, .startOffset = startOffset, .endOffset = endOffset}; }
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
class StackAllocatorSafe : public StackAllocatorBase
{
  public:
    // Prohibit default construction, moving and assignment
    StackAllocatorSafe()                          = delete;
    StackAllocatorSafe(const StackAllocatorSafe&) = delete;
    StackAllocatorSafe(StackAllocatorSafe&&)      = delete;
    StackAllocatorSafe& operator=(const StackAllocatorSafe&) = delete;
    StackAllocatorSafe& operator=(StackAllocatorSafe&&) = delete;

    StackAllocatorSafe(const Size totalSize, const std::shared_ptr<MemoryManager> memoryManager = nullptr,
                       const char* debugName = "StackAllocatorSafe");

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
    StackPtr<Object> New(Args... argList);

    /**
     * @brief Deallocates a pointer and calls the destructor
     * @details Speed complexity is O(1)
     *
     * @tparam Object The type of the passed pointer
     * @param ptr The pointer to the memory to be deallocated
     */
    template <typename Object>
    void Delete(StackPtr<Object> ptr);

    /**
     * @brief Allocates raw memory without calling any constructor
     * @details Speed complexity is O(1)
     * @param size The size of the memory to be allocated in bytes
     * @param alignment The alignment of the memory to be allocated in bytes
     * @return void* The pointer to the newly allocated memory
     */
    StackPtr<void> Allocate(const Size size, const Alignment alignment = 8);

    /**
     * @brief Deallocates raw memory without calling any destructor. It also deallocates
     * all allocations that were done after this pointer was allocated.
     * @details Speed complexity is O(1)
     * @param ptr The pointer to the memory to be deallocated
     */
    void Deallocate(StackPtr<void> ptr);

  private:
    StackAllocatorSafe(StackAllocatorSafe& stackAllocator); // Restrict copying
};

template <typename Object, typename... Args>
StackPtr<Object> StackAllocatorSafe::New(Args... argList)
{
    StackPtr<void> rawPtr = Allocate(sizeof(Object), alignof(Object)); // Allocate the raw memory and get a pointer to it
    return {.ptr = new (rawPtr.ptr) Object(argList...), .startOffset = rawPtr.startOffset, .endOffset = rawPtr.endOffset};
}

template <typename Object>
void StackAllocatorSafe::Delete(StackPtr<Object> ptr)
{

    ptr.ptr->~Object(); // Call the destructor on the object
    Deallocate(ptr);    // Deallocate the pointer
}

} // namespace Memory