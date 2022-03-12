#pragma once

#include <memory>
#include <string>

#include "Source/TypeAliases.hpp"

namespace Memarena
{

class MemoryManager;
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

    [[nodiscard]] Size        GetUsedSize() const;
    [[nodiscard]] Size        GetTotalSize() const;
    [[nodiscard]] std::string GetDebugName() const;

  protected:
    Allocator(Size totalSize, const std::shared_ptr<MemoryManager>& memoryManager, const std::string& debugName);

    ~Allocator();

    [[nodiscard]] const void* GetStartPtr() const { return m_StartPtr; }
    void                      SetUsedSize(Size size);
    [[nodiscard]] bool        OwnsAddress(UIntPtr address) const;

  private:
    std::shared_ptr<MemoryManager> m_MemoryManager; // the memory manager that this allocator will report the memory usage to
    void*                          m_StartPtr = nullptr;
    std::shared_ptr<AllocatorData> m_Data;
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