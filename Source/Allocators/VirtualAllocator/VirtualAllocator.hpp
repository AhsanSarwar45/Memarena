// #pragma once

// #include <bit>
// #include <vector>

// #include "Source/Aliases.hpp"
// #include "Source/Allocator.hpp"
// #include "Source/AllocatorData.hpp"
// #include "Source/AllocatorUtils.hpp"
// #include "Source/Assert.hpp"
// #include "Source/Macros.hpp"
// #include "Source/Policies/MultithreadedPolicy.hpp"
// #include "Source/Policies/Policies.hpp"
// #include "Source/Traits.hpp"
// #include "Source/Utility/Alignment/Alignment.hpp"
// #include "Source/Utility/Math.hpp"
// #include "Source/Utility/VirtualMemory.hpp"

// namespace Memarena
// {

// // template <typename T>
// // concept Allocatable = requires(T a)
// // {
// //     {
// //         std::hash<T>{}(a)
// //         } -> std::convertible_to<std::size_t>;
// // };

// // template <typename T>
// // using MallocPtr = Internal::BaseAllocatorPtr<T>;
// // template <typename T>
// // using MallocArrayPtr = Internal::BaseAllocatorArrayPtr<T>;

// template <VirtualAllocatorPolicy policy = VirtualAllocatorPolicy::Default>
// class VirtualAllocator : public Allocator
// {
//   private:
//     // static constexpr bool IsDoubleFreePreventionEnabled = PolicyContains(policy, VirtualAllocatorPolicy::DoubleFreePrevention);
//     // static constexpr bool NullDeallocCheckIsEnabled =
//     //     PolicyContains(policy, VirtualAllocatorPolicy::NullDeallocCheck) || IsDoubleFreePreventionEnabled;
//     // static constexpr bool IsNullAllocCheckEnabled     = PolicyContains(policy, VirtualAllocatorPolicy::NullAllocCheck);
//     // static constexpr bool AllocationTrackingIsEnabled = PolicyContains(policy, VirtualAllocatorPolicy::AllocationTracking);
//     // static constexpr bool IsSizeTrackingEnabled       = PolicyContains(policy, VirtualAllocatorPolicy::SizeTracking);
//     // static constexpr bool NeedsMultithreading         = AllocationTrackingIsEnabled || IsSizeTrackingEnabled;
//     // static constexpr bool IsMultithreaded = PolicyContains(policy, VirtualAllocatorPolicy::Multithreaded) && NeedsMultithreading;

//     // using ThreadPolicy = MultithreadedPolicy<IsMultithreaded>;

//     // template <typename SyncPrimitive>
//     // using LockGuard = typename ThreadPolicy::template LockGuard<SyncPrimitive>;
//     // using Mutex     = typename ThreadPolicy::Mutex;

//   public:
//     // Prohibit default construction, moving and assignment
//     VirtualAllocator()                        = delete;
//     VirtualAllocator(const VirtualAllocator&) = delete;
//     VirtualAllocator(VirtualAllocator&)       = delete;
//     VirtualAllocator(VirtualAllocator&&)      = delete;
//     VirtualAllocator& operator=(const VirtualAllocator&) = delete;
//     VirtualAllocator& operator=(VirtualAllocator&&) = delete;

//     explicit VirtualAllocator(const Size maxSize, const Size growStepSize, const std::string& debugName = "VirtualAllocator")
//         : Allocator(0, debugName, true), m_MaxSize(maxSize), m_GrowStepSize(growStepSize),
//           m_VirtualStartAddress(std::bit_cast<UIntPtr>(ReserveVirtualMemory(maxSize))),
//           m_VirtualEndAddress(m_VirtualStartAddress + growStepSize), m_PhysicalCurrentAddress(m_VirtualStartAddress),
//           m_PhysicalEndAddress(m_VirtualStartAddress)
//     {
//     }

//     ~VirtualAllocator() = default;

//     // template <Allocatable Object, typename... Args>
//     // NO_DISCARD Object* NewRaw(Args&&... argList)
//     // {
//     //     void* voidPtr = Allocate<Object>();
//     //     return new (voidPtr) Object(std::forward<Args>(argList)...);
//     // }

//     // template <Allocatable Object>
//     // void Delete(MallocPtr<Object>& ptr)
//     // {
//     //     DeallocateInternal(ptr, ptr.GetSize());
//     //     ptr->~Object();
//     // }

//     // template <Allocatable Object, typename... Args>
//     // NO_DISCARD Object* NewArrayRaw(const Size objectCount, Args&&... argList)
//     // {
//     //     void* voidPtr = AllocateArray<Object>(objectCount);
//     //     return Internal::ConstructArray<Object>(voidPtr, objectCount, std::forward<Args>(argList)...);
//     // }

//     // template <Allocatable Object>
//     // void DeleteArray(Object* ptr)
//     // {
//     //     DeallocateInternal(ptr, ptr.GetSize());
//     //     std::destroy_n(ptr.GetPtr(), ptr.GetCount());
//     // }

//     template <typename Object>
//     NO_DISCARD void* Allocate(const std::string& category = "", const SourceLocation& sourceLocation = SourceLocation::current())
//     {
//         return Allocate(sizeof(Object), category, sourceLocation);
//     }

//     NO_DISCARD void* AllocateArray(const Size objectCount, const Size objectSize, const std::string& category = "",
//                                    const SourceLocation& sourceLocation = SourceLocation::current())
//     {
//         const Size allocationSize = objectCount * objectSize;
//         return Allocate(allocationSize, category, sourceLocation);
//     }

//     template <typename Object>
//     NO_DISCARD void* AllocateArray(const Size objectCount, const std::string& category = "",
//                                    const SourceLocation& sourceLocation = SourceLocation::current())
//     {
//         return AllocateArray(objectCount, sizeof(Object), category, sourceLocation);
//     }

//     NO_DISCARD void* Allocate(const Size size, const Alignment& alignment, const std::string& category = "",
//                               const SourceLocation& sourceLocation = SourceLocation::current())
//     {

//         return AllocateAt(CalculateAlignedAddress(m_PhysicalCurrentAddress + size, alignment), category, sourceLocation);
//     }

//     void DeallocateAt(UIntPtr address) { m_PhysicalCurrentAddress = address; }

//     void Purge()
//     {
//         const UIntPtr addressToFree = CalculateAlignedAddress(m_PhysicalCurrentAddress, m_GrowStepSize);
//         const Size    sizeToFree    = m_PhysicalEndAddress - addressToFree;
//         FreeVirtualMemory(addressToFree, sizeToFree);
//         m_PhysicalEndAddress = addressToFree;
//     }

//     NO_DISCARD void* AllocateAt(const UIntPtr address, const std::string& category = "",
//                                 const SourceLocation& sourceLocation = SourceLocation::current())
//     {
//         m_PhysicalCurrentAddress = address;

//         if (m_PhysicalCurrentAddress > m_PhysicalEndAddress)
//         {
//             const Size requiredPhysicalSize = RoundUpToMultiple(m_PhysicalCurrentAddress - m_PhysicalEndAddress, m_GrowStepSize);

//             MEMARENA_ASSERT(m_PhysicalEndAddress + neededPhysicalSize < m_VirtualEndAddress,
//                             "The allocator '%s' has reached it's max reserved size!", GetDebugName().c_str());

//             CommitVirtualMemory(m_PhysicalEndAddress, requiredPhysicalSize);
//             m_PhysicalEndAddress += requiredPhysicalSize;
//         }

//         // return allocation to user
//         return std::bit_cast<void*>(m_PhysicalCurrentAddress);
//     }

//     // NO_DISCARD Internal::BaseAllocatorPtr<void> AllocateBase(const Size size) final { return Allocate(size); }
//     // void                                        DeallocateBase(Internal::BaseAllocatorPtr<void> ptr) final { Deallocate(ptr); }

//   private:
//     // ThreadPolicy m_MultithreadedPolicy;

//     // ----- Don't change order ------
//     UIntPtr m_VirtualStartAddress;
//     // -------------------------------

//     UIntPtr m_VirtualEndAddress;
//     UIntPtr m_PhysicalCurrentAddress;
//     UIntPtr m_PhysicalEndAddress;
//     Size    m_MaxSize;
//     Size    m_GrowStepSize;
// };
// } // namespace Memarena