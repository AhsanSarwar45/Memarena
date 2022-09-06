#pragma once

#include <memory>
#include <numeric>
#include <string>

#include "Source/AllocatorData.hpp"
#include "Source/Assert.hpp"
#include "Source/MemoryTracker.hpp"
#include "Source/Policies/MultithreadedPolicy.hpp"
#include "Source/TypeAliases.hpp"
#include "Utility/Alignment/Alignment.hpp"

namespace Memarena
{

struct AllocatorData;

template <typename T>
class Ptr
{
  public:
    inline T*                     GetPtr() { return m_Ptr; }
    [[nodiscard]] inline const T* GetPtr() const { return m_Ptr; }
    inline void                   Reset() { m_Ptr = nullptr; }

    inline const T* operator->() const { return m_Ptr; }
    inline auto     operator*() const -> std::add_lvalue_reference_t<T>
    requires(!std::is_void_v<T>) { return *m_Ptr; }
    inline explicit operator bool() const noexcept { return (m_Ptr != nullptr); }

  protected:
    explicit Ptr(T* ptr) : m_Ptr(ptr) {}

  private:
    T* m_Ptr;
};

template <typename T>
class ArrayPtr : public Ptr<T>
{
  public:
    T                         operator[](int index) const { return this->GetPtr()[index]; }
    [[nodiscard]] inline Size GetCount() const { return m_Count; }

  protected:
    explicit ArrayPtr(T* ptr, Size count) : Ptr<T>(ptr), m_Count(count) {}

  private:
    Size m_Count;
};

template <typename T>
class BaseAllocatorPtr : public Ptr<T>
{
  public:
    inline BaseAllocatorPtr(T* ptr, Size size) : Ptr<T>(ptr), m_Size(size) {}
    [[nodiscard]] Size GetSize() const { return m_Size; }

  private:
    Size m_Size;
};

template <typename T>
class BaseAllocatorArrayPtr : public ArrayPtr<T>
{
  public:
    inline BaseAllocatorArrayPtr(T* ptr, Size size, Size count) : ArrayPtr<T>(ptr, count), m_Size(size) {}
    [[nodiscard]] Size GetSize() const { return m_Size; }

  private:
    Size m_Size;
};

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

    NO_DISCARD virtual BaseAllocatorPtr<void> AllocateBase(Size /*size*/) { return BaseAllocatorPtr<void>(nullptr, 0); }
    virtual void                              DeallocateBase(BaseAllocatorPtr<void> ptr) {}

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