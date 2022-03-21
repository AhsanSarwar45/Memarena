#pragma once

#include "StackAllocator.hpp"

namespace Memarena
{
template <typename Object, StackAllocatorPolicy policy = StackAllocatorPolicy::Default>
class StackAllocatorTemplated
{
  public:
    StackAllocatorTemplated()                               = delete;
    StackAllocatorTemplated(StackAllocatorTemplated&)       = delete;
    StackAllocatorTemplated(const StackAllocatorTemplated&) = delete;
    StackAllocatorTemplated(StackAllocatorTemplated&&)      = delete;
    StackAllocatorTemplated& operator=(const StackAllocatorTemplated&) = delete;
    StackAllocatorTemplated& operator=(StackAllocatorTemplated&&) = delete;

    explicit StackAllocatorTemplated(const Size totalSize, const std::string& debugName = "StackAllocator")
        : m_StackAllocator(totalSize, debugName)
    {
    }

    ~StackAllocatorTemplated() = default;

    template <typename... Args>
    [[nodiscard(NO_DISCARD_ALLOC_INFO)]] StackPtr<Object> New(Args&&... argList)
    {
        return m_StackAllocator.template New<Object>(std::forward<Args>(argList)...);
    }

    template <typename... Args>
    [[nodiscard(NO_DISCARD_ALLOC_INFO)]] Object* NewRaw(Args&&... argList)
    {
        return m_StackAllocator.template NewRaw<Object>(std::forward<Args>(argList)...);
    }

    void Delete(StackPtr<Object> ptr) { m_StackAllocator.Delete(ptr); }

    void Delete(Object* ptr) { m_StackAllocator.Delete(ptr); }

    template <typename... Args>
    [[nodiscard(NO_DISCARD_ALLOC_INFO)]] StackArrayPtr<Object> NewArray(const Size objectCount, Args&&... argList)
    {
        return m_StackAllocator.template NewArray<Object>(objectCount, std::forward<Args>(argList)...);
    }

    template <typename... Args>
    [[nodiscard(NO_DISCARD_ALLOC_INFO)]] Object* NewArrayRaw(const Size objectCount, Args&&... argList)
    {
        return m_StackAllocator.template NewArrayRaw<Object>(objectCount, std::forward<Args>(argList)...);
    }

    void DeleteArray(Object* ptr) { m_StackAllocator.DeleteArray(ptr); }

    void DeleteArray(StackArrayPtr<Object> ptr) { m_StackAllocator.DeleteArray(ptr); }

    [[nodiscard(NO_DISCARD_ALLOC_INFO)]] void* Allocate(const Size size, const Alignment& alignment, const std::string& category = "",
                                                        const SourceLocation& sourceLocation = SourceLocation::current())
    {
        return m_StackAllocator.Allocate(size, alignment, category, sourceLocation);
    }

    [[nodiscard(NO_DISCARD_ALLOC_INFO)]] void* Allocate(const std::string&    category       = "",
                                                        const SourceLocation& sourceLocation = SourceLocation::current())
    {
        return m_StackAllocator.Allocate(sizeof(Object), AlignOf(alignof(Object)), category, sourceLocation);
    }

    void Deallocate(void* ptr) { m_StackAllocator.Deallocate(ptr); }

    void Deallocate(const StackPtr<void>& ptr) { m_StackAllocator.Deallocate(ptr); }

    [[nodiscard(NO_DISCARD_ALLOC_INFO)]] void* AllocateArray(const Size objectCount, const Size objectSize, const Alignment& alignment,
                                                             const std::string&    category       = "",
                                                             const SourceLocation& sourceLocation = SourceLocation::current())
    {
        return m_StackAllocator.AllocateArray(objectCount, objectSize, alignment, category, sourceLocation);
    }

    [[nodiscard(NO_DISCARD_ALLOC_INFO)]] void* AllocateArray(const Size objectCount, const std::string& category = "",
                                                             const SourceLocation& sourceLocation = SourceLocation::current())
    {
        return AllocateArray(objectCount, sizeof(Object), AlignOf(alignof(Object)), category, sourceLocation);
    }

    Size DeallocateArray(void* ptr, const Size objectSize) { return m_StackAllocator.DeallocateArray(ptr, objectSize); }

    Size DeallocateArray(const StackArrayPtr<void>& ptr, const Size objectSize)
    {
        return m_StackAllocator.DeallocateArray(ptr, objectSize);
    }

    /**
     * @brief Releases the allocator to its initial state. Any further allocations
     * will possibly overwrite all object allocated prior to calling this method.
     * So make sure to only call this when you don't need any objects previously
     * allocated by this allocator.
     *
     */
    inline void Release() { m_StackAllocator.Release(); }

    [[nodiscard]] Size        GetUsedSize() const { return m_StackAllocator.GetUsedSize(); }
    [[nodiscard]] Size        GetTotalSize() const { return m_StackAllocator.GetTotalSize(); }
    [[nodiscard]] std::string GetDebugName() const { return m_StackAllocator.GetDebugName(); }

  private:
    StackAllocator<policy> m_StackAllocator;
};
} // namespace Memarena