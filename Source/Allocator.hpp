#pragma once

#include <memory>
#include <numeric>
#include <string>

#include "Source/AllocatorData.hpp"
#include "Source/Assert.hpp"
#include "Source/MemoryTracker.hpp"
#include "Source/Policies/MultithreadedPolicy.hpp"
#include "Source/TypeAliases.hpp"

namespace Memarena
{

struct AllocatorData;

class Allocator
{
  public:
    Allocator()                 = delete;
    Allocator(const Allocator&) = delete;
    Allocator(Allocator&&)      = delete;
    Allocator& operator=(const Allocator&) = delete;
    Allocator& operator=(Allocator&&) = delete;

    [[nodiscard]] inline Size        GetUsedSize() const { return m_Data->usedSize; }
    [[nodiscard]] inline Size        GetTotalSize() const { return m_Data->totalSize; }
    [[nodiscard]] inline Size        GetPeakUsedSize() const { return m_Data->peakUsage; }
    [[nodiscard]] inline UInt32      GetAllocationCount() const { return m_Data->allocationCount; }
    [[nodiscard]] inline UInt32      GetDeallocationCount() const { return m_Data->deallocationCount; }
    [[nodiscard]] inline std::string GetDebugName() const { return m_Data->debugName; }

    [[nodiscard]] inline const std::vector<AllocationData>& GetAllocations() const { return m_Data->allocations; }

  protected:
    Allocator(Size totalSize, const std::string& debugName);

    ~Allocator();

    void        SetUsedSize(Size size);
    void        IncreaseTotalSize(Size size) { m_Data->totalSize += size; }
    void        AddAllocation(Size size, const std::string& category, const SourceLocation& sourceLocation = SourceLocation::current());
    inline void AddDeallocation() { m_Data->deallocationCount++; }

  private:
    std::shared_ptr<AllocatorData> m_Data;
};

template <typename T>
class Ptr
{
  public:
    inline T*       GetPtr() { return ptr; }
    inline const T* GetPtr() const { return ptr; }

    inline const T* operator->() const { return ptr; }
    inline auto     operator*() const -> std::add_lvalue_reference_t<T>
    requires(!std::is_void_v<T>) { return *ptr; }
    inline explicit operator bool() const noexcept { return (ptr != nullptr); }

  protected:
    explicit Ptr(T* _ptr) : ptr(_ptr) {}

  private:
    T* ptr;
};

} // namespace Memarena