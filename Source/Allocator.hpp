#pragma once

#include <memory>
#include <numeric>
#include <string>

#include "Source/AllocatorData.hpp"
#include "Source/Assert.hpp"
#include "Source/MemoryManager.hpp"
#include "Source/Policies/MultithreadedPolicy.hpp"
#include "Source/TypeAliases.hpp"

namespace Memarena
{

struct AllocatorData;

namespace Internal
{

class Allocator
{
  public:
    Allocator()                 = delete;
    Allocator(const Allocator&) = delete;
    Allocator(Allocator&&)      = delete;
    Allocator& operator=(const Allocator&) = delete;
    Allocator& operator=(Allocator&&) = delete;

    [[nodiscard]] Size        GetUsedSize() const { return m_Data->usedSize; }
    [[nodiscard]] Size        GetTotalSize() const { return m_Data->totalSize; }
    [[nodiscard]] std::string GetDebugName() const { return m_Data->debugName; }

  protected:
    Allocator(Size totalSize, const std::string& debugName, bool isMemoryTrackingEnabled);

    ~Allocator();

    [[nodiscard]] inline const void* GetStartPtr() const { return m_StartPtr; }

    void SetUsedSize(Size size);

    inline static MemoryManager s_MemoryManager; // the memory manager that this allocator will report the memory usage to

  private:
    void*                          m_StartPtr = nullptr;
    std::shared_ptr<AllocatorData> m_Data;
    bool                           m_IsMemoryTrackingEnabled;
};

} // namespace Internal

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