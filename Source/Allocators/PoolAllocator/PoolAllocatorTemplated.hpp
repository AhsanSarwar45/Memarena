#pragma once

#include "PoolAllocator.hpp"

namespace Memarena
{
template <Allocatable Object, PoolAllocatorSettings Settings = poolAllocatorDefaultSettings>
class PoolAllocatorTemplated
{
  public:
    PoolAllocatorTemplated()                              = delete;
    PoolAllocatorTemplated(PoolAllocatorTemplated&)       = delete;
    PoolAllocatorTemplated(const PoolAllocatorTemplated&) = delete;
    PoolAllocatorTemplated(PoolAllocatorTemplated&&)      = delete;
    PoolAllocatorTemplated& operator=(const PoolAllocatorTemplated&) = delete;
    PoolAllocatorTemplated& operator=(PoolAllocatorTemplated&&) = delete;

    explicit PoolAllocatorTemplated(const Size objectsPerBlock, const std::string& debugName = "PoolAllocatorTemplated")
        : m_PoolAllocator(sizeof(Object), objectsPerBlock, debugName)
    {
    }

    ~PoolAllocatorTemplated() = default;

    template <typename... Args>
    NO_DISCARD PoolPtr<Object> New(Args&&... argList)
    {
        return m_PoolAllocator.template New<Object>(std::forward<Args>(argList)...);
    }

    template <typename... Args>
    NO_DISCARD PoolArrayPtr<Object> NewArray(Size objectCount, Args&&... argList)
    {
        return m_PoolAllocator.template NewArray<Object>(objectCount, std::forward<Args>(argList)...);
    }

    void Delete(PoolPtr<Object> ptr) { m_PoolAllocator.Delete(ptr); }

    void DeleteArray(PoolArrayPtr<Object> ptr) { m_PoolAllocator.Delete(ptr); }

    NO_DISCARD PoolPtr<void> Allocate(const std::string& category = "", const SourceLocation& sourceLocation = SourceLocation::current())
    {
        return m_PoolAllocator.Allocate(category, sourceLocation);
    }

    NO_DISCARD PoolArrayPtr<void> AllocateArray(Size objectCount, const std::string& category = "",
                                                const SourceLocation& sourceLocation = SourceLocation::current())
    {
        return m_PoolAllocator.AllocateArray(objectCount, category, sourceLocation);
    }

    void Deallocate(PoolPtr<void> ptr) { m_PoolAllocator.Deallocate(ptr); }

    void DeallocateArray(PoolArrayPtr<void> ptr) { m_PoolAllocator.Deallocate(ptr); }

    /**
     * @brief Releases the allocator to its initial state. Any further allocations
     * will possibly overwrite all object allocated prior to calling this method.
     * So make sure to only call this when you don't need any objects previously
     * allocated by this allocator.
     *
     */
    // inline void Release() { m_PoolAllocator.Release(); }

    [[nodiscard]] Size        GetUsedSize() const { return m_PoolAllocator.GetUsedSize(); }
    [[nodiscard]] Size        GetTotalSize() const { return m_PoolAllocator.GetTotalSize(); }
    [[nodiscard]] std::string GetDebugName() const { return m_PoolAllocator.GetDebugName(); }

  private:
    PoolAllocator<Settings> m_PoolAllocator;
};
} // namespace Memarena