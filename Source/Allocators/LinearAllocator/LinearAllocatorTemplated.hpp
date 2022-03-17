#pragma once

#include "LinearAllocator.hpp"

namespace Memarena
{
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

    explicit LinearAllocatorTemplated(const Size totalSize, const std::string& debugName = "LinearAllocator")
        : m_LinearAllocator(totalSize, debugName)
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