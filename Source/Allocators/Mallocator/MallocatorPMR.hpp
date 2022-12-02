#pragma once

#include <memory_resource>

#include "Mallocator.hpp"

namespace Memarena
{
template <MallocatorSettings Settings = mallocatorDefaultSettings>
class MallocatorPMR : public std::pmr::memory_resource
{
  public:
    explicit MallocatorPMR(const std::string& debugName = "MallocatorPMR") : m_Mallocator(debugName) {}

    void*              do_allocate(size_t bytes, size_t /*alignment*/) override { return m_Mallocator.Allocate(bytes).GetPtr(); }
    void               do_deallocate(void* ptr, size_t bytes, size_t /*alignment*/) override { m_Mallocator.Deallocate(ptr, bytes); }
    [[nodiscard]] bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override { return this == &other; }

    const Mallocator<Settings>& GetInternalAllocator() const { return m_Mallocator; }

  private:
    Mallocator<Settings> m_Mallocator;
};

} // namespace Memarena