
#include <bit>
#include <memoryapi.h>
#include <minwindef.h>

#include "VirtualMemory.hpp"

namespace Memarena
{
// TODO: Write asserts

NO_DISCARD void* ReserveVirtualMemory(Size size) { return VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_NOACCESS); }
void CommitVirtualMemory(UIntPtr address, Size size) { VirtualAlloc(std::bit_cast<void*>(address), size, MEM_COMMIT, PAGE_READWRITE); }
void FreeVirtualMemory(UIntPtr address, Size size) { VirtualFree(std::bit_cast<void*>(address), size, MEM_RELEASE); }

} // namespace Memarena
