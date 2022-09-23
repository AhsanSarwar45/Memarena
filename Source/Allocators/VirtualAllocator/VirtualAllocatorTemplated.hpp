// #pragma once

// #include "Mallocator.hpp"

// namespace Memarena
// {
// template <Allocatable Object, MallocatorPolicy policy = MallocatorPolicy::Default>
// class MallocatorTemplated
// {
//   public:
//     MallocatorTemplated(MallocatorTemplated&)       = delete;
//     MallocatorTemplated(const MallocatorTemplated&) = delete;
//     MallocatorTemplated(MallocatorTemplated&&)      = delete;
//     MallocatorTemplated& operator=(const MallocatorTemplated&) = delete;
//     MallocatorTemplated& operator=(MallocatorTemplated&&) = delete;

//     explicit MallocatorTemplated(const std::string& debugName) : m_Mallocator(debugName) {}
//     MallocatorTemplated() : m_Mallocator("MallocatorTemplated") {}

//     ~MallocatorTemplated() = default;

//     template <typename... Args>
//     NO_DISCARD MallocPtr<Object> New(Args&&... argList)
//     {
//         return m_Mallocator.template New<Object>(std::forward<Args>(argList)...);
//     }

//     template <typename... Args>
//     NO_DISCARD MallocArrayPtr<Object> NewArray(const Size objectCount, Args&&... argList)
//     {
//         return m_Mallocator.template NewArray<Object>(objectCount, std::forward<Args>(argList)...);
//     }

//     NO_DISCARD MallocPtr<void> Allocate(const Size size, const std::string& category = "",
//                                         const SourceLocation& sourceLocation = SourceLocation::current())
//     {
//         return m_Mallocator.Allocate(size, category, sourceLocation);
//     }

//     NO_DISCARD MallocPtr<void> Allocate(const std::string& category = "", const SourceLocation& sourceLocation =
//     SourceLocation::current())
//     {
//         return m_Mallocator.Allocate(sizeof(Object), category, sourceLocation);
//     }

//     NO_DISCARD MallocPtr<void> AllocateArray(const Size objectCount, const Size objectSize, const std::string& category = "",
//                                              const SourceLocation& sourceLocation = SourceLocation::current())
//     {
//         return m_Mallocator.AllocateArray(objectCount, objectSize, category, sourceLocation);
//     }

//     NO_DISCARD MallocPtr<void> AllocateArray(const Size objectCount, const std::string& category = "",
//                                              const SourceLocation& sourceLocation = SourceLocation::current())
//     {
//         return AllocateArray(objectCount, sizeof(Object), alignof(Object), category, sourceLocation);
//     }

//     void Delete(MallocPtr<Object>& ptr) { m_Mallocator.Delete(ptr); }

//     void DeleteArray(MallocArrayPtr<Object>& ptr) { m_Mallocator.DeleteArray(ptr); }

//     void Deallocate(MallocPtr<void>& ptr) { m_Mallocator.Deallocate(ptr); }
//     void Deallocate(void* ptr, Size size) { m_Mallocator.Deallocate(ptr, size); }
//     void Deallocate(MallocArrayPtr<void>& ptr) { m_Mallocator.Deallocate(ptr); }

//     [[nodiscard]] Size        GetUsedSize() const { return m_Mallocator.GetUsedSize(); }
//     [[nodiscard]] Size        GetTotalSize() const { return m_Mallocator.GetTotalSize(); }
//     [[nodiscard]] std::string GetDebugName() const { return m_Mallocator.GetDebugName(); }

//   private:
//     Mallocator<policy> m_Mallocator;
// };
// } // namespace Memarena