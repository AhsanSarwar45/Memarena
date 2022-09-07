#pragma once

#include "LinearAllocator.hpp"

namespace Memarena
{
template <Allocatable Object, LinearAllocatorPolicy policy = LinearAllocatorPolicy::Default>
class LinearAllocatorTemplated
{
  public:
    LinearAllocatorTemplated()                                = delete;
    LinearAllocatorTemplated(LinearAllocatorTemplated&)       = delete;
    LinearAllocatorTemplated(const LinearAllocatorTemplated&) = delete;
    LinearAllocatorTemplated(LinearAllocatorTemplated&&)      = delete;
    LinearAllocatorTemplated& operator=(const LinearAllocatorTemplated&) = delete;
    LinearAllocatorTemplated& operator=(LinearAllocatorTemplated&&) = delete;

    explicit LinearAllocatorTemplated(const Size totalSize, const std::string& debugName = "LinearAllocatorTemplated",
                                      std::shared_ptr<Allocator> baseAllocator = Allocator::GetDefaultAllocator())
        : m_LinearAllocator(totalSize, debugName, baseAllocator)
    {
    }

    ~LinearAllocatorTemplated() = default;

    template <typename... Args>
    NO_DISCARD Object* NewRaw(Args&&... argList)
    {
        return m_LinearAllocator.template NewRaw<Object>(std::forward<Args>(argList)...);
    }

    template <typename... Args>
    NO_DISCARD Object* NewArrayRaw(const Size objectCount, Args&&... argList)
    {
        return m_LinearAllocator.template NewArrayRaw<Object>(objectCount, std::forward<Args>(argList)...);
    }

    NO_DISCARD void* Allocate(const Size size, const Alignment& alignment, const std::string& category = "",
                              const SourceLocation& sourceLocation = SourceLocation::current())
    {
        return m_LinearAllocator.Allocate(size, alignment, category, sourceLocation);
    }

    NO_DISCARD void* Allocate(const std::string& category = "", const SourceLocation& sourceLocation = SourceLocation::current())
    {
        return m_LinearAllocator.Allocate(sizeof(Object), alignof(Object), category, sourceLocation);
    }

    NO_DISCARD void* AllocateArray(const Size objectCount, const Size objectSize, const Alignment& alignment,
                                   const std::string& category = "", const SourceLocation& sourceLocation = SourceLocation::current())
    {
        return m_LinearAllocator.AllocateArray(objectCount, objectSize, alignment, category, sourceLocation);
    }

    NO_DISCARD void* AllocateArray(const Size objectCount, const std::string& category = "",
                                   const SourceLocation& sourceLocation = SourceLocation::current())
    {
        return AllocateArray(objectCount, sizeof(Object), alignof(Object), category, sourceLocation);
    }
    /**
     * @brief Releases the allocator to its initial state. Any further allocations
     * will possibly overwrite all object allocated prior to calling this method.
     * So make sure to only call this when you don't need any objects previously
     * allocated by this allocator.
     *
     */
    inline void Release() { m_LinearAllocator.Release(); }

    [[nodiscard]] Size        GetUsedSize() const { return m_LinearAllocator.GetUsedSize(); }
    [[nodiscard]] Size        GetTotalSize() const { return m_LinearAllocator.GetTotalSize(); }
    [[nodiscard]] std::string GetDebugName() const { return m_LinearAllocator.GetDebugName(); }

  private:
    LinearAllocator<policy> m_LinearAllocator;
};
} // namespace Memarena