#pragma once

#include <type_traits>

#include "Source/TypeAliases.hpp"

namespace Memarena
{
template <typename T>
class Ptr
{
    friend class Allocator;

  public:
    inline T*                     GetPtr() { return m_Ptr; }
    [[nodiscard]] inline const T* GetPtr() const { return m_Ptr; }
    [[nodiscard]] inline bool     IsNullPtr() const { return m_Ptr == nullptr; }
    inline void                   Reset() { m_Ptr = nullptr; }

    inline const T* operator->() const { return m_Ptr; }
    inline auto     operator*() const -> std::add_lvalue_reference_t<T>
    requires(!std::is_void_v<T>) { return *m_Ptr; }
    inline explicit operator bool() const noexcept { return (m_Ptr != nullptr); }
    inline bool     operator==(std::nullptr_t) const noexcept { return m_Ptr == nullptr; }

    void operator=(const Ptr&) = delete;

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

namespace Internal
{

} // namespace Internal
} // namespace Memarena