#pragma once

#include <memory>
#include <numeric>
#include <string>

#include "Pointer.hpp"
#include "Source/AllocatorData.hpp"
#include "Source/Assert.hpp"
#include "Source/MemoryTracker.hpp"
#include "Source/Policies/MultithreadedPolicy.hpp"
#include "Source/TypeAliases.hpp"
#include "Utility/Alignment/Alignment.hpp"

namespace Memarena
{

struct AllocatorData;

class Allocator
{
  public:
    Allocator()                 = delete;
    Allocator(Allocator&&)      = delete;
    Allocator(const Allocator&) = delete;
    Allocator& operator=(const Allocator&) = delete;
    Allocator& operator=(Allocator&&) = delete;

    ~Allocator();

    [[nodiscard]] inline Size        GetUsedSize() const { return m_Data->usedSize; }
    [[nodiscard]] inline Size        GetTotalSize() const { return m_Data->totalSize; }
    [[nodiscard]] inline Size        GetPeakUsedSize() const { return m_Data->peakUsage; }
    [[nodiscard]] inline UInt32      GetAllocationCount() const { return m_Data->allocationCount; }
    [[nodiscard]] inline UInt32      GetDeallocationCount() const { return m_Data->deallocationCount; }
    [[nodiscard]] inline std::string GetDebugName() const { return m_Data->debugName; }

    [[nodiscard]] inline const std::vector<AllocationData>& GetAllocations() const { return m_Data->allocations; }

    [[nodiscard]] static std::shared_ptr<Allocator> GetDefaultAllocator() { return m_DefaultAllocator; }

    NO_DISCARD virtual Internal::BaseAllocatorPtr<void> AllocateBase(Size /*size*/) { return {nullptr, 0}; }
    virtual void                                        DeallocateBase(Internal::BaseAllocatorPtr<void> ptr) {}

  protected:
    Allocator(Size totalSize, const std::string& debugName, bool isBaseAllocator = false);

    void        SetUsedSize(Size size);
    inline void IncreaseUsedSize(Size size) { SetUsedSize(m_Data->usedSize + size); }
    inline void DecreaseUsedSize(Size size) { m_Data->usedSize -= size; }
    void        SetTotalSize(Size size);
    inline void IncreaseTotalSize(Size size) { SetTotalSize(m_Data->totalSize + size); }
    inline void DecreaseTotalSize(Size size) { SetTotalSize(m_Data->totalSize - size); }
    void        AddAllocation(Size size, const std::string& category, const SourceLocation& sourceLocation = SourceLocation::current());
    inline void AddDeallocation() { m_Data->deallocationCount++; }

  private:
    std::shared_ptr<AllocatorData>          m_Data;
    static const std::shared_ptr<Allocator> m_DefaultAllocator;
};

} // namespace Memarena