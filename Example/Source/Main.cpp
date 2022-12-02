#include "Source/Allocators/Mallocator/Mallocator.hpp"
#include "Source/Policies/Policies.hpp"
// #define MEMARENA_ENABLE_ASSERTS_BREAK 0
// #define MEMARENA_ENABLE_ASSERTS_ERROR_MSG 0

#include "Memarena/Memarena.hpp"

using namespace Memarena::SizeLiterals;

struct TestStruct
{
    int a;
    int b;
    int c;
    int d;
};

int main()
{
    // constexpr Memarena::PoolAllocatorPolicy policy = Memarena::PoolAllocatorPolicy::Default | Memarena::PoolAllocatorPolicy::Growable;
    // Memarena::PoolAllocatorTemplated<TestStruct, policy> allocator{1};

    // Memarena::PoolPtr<TestStruct> ptr  = allocator.New();
    // Memarena::PoolPtr<TestStruct> ptr2 = allocator.New();

    Memarena::Mallocator allocator;
    auto                 ptr = allocator.New<TestStruct>();

    if (!ptr.IsNullPtr())
    {
        std::cout << "Hekllo" << ptr->a << "\n";
    }
    else
    {
        std::cout << "Failed to allocate memory!\n";
    }
}
