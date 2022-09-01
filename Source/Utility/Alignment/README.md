# Memory Alignment
Memory access is one of the primary bottlenecks in modern computer architectures. A single memory read takes significantly more time than a CPU operation. Hence, to retain performance, we should look to minimize the number of reads and writes to memory.

One way processors reduces memory access is to read and write memory in byte-sized chunks. The size of such chunks is called memory access **granularity**. This is done base on the assumption of spatial locality - memory that is stored together is used together. So instead of fetching single bytes at a time, they fetch multiple bytes in a single read. 

For example, if we want to read a struct of size `16 bytes`, a higher granularity will require less memory reads and hence be faster:

| Granularity | Number of memory reads needed|
|---|----|
| 1 | 16 |
| 2 | 8  |
| 4 | 4  |

Another thing to notice is that the processor can only start reading memory addresses that are a multiple of the granularity. A processor with a granularity of `8 bytes` cannot start reading at an address of `19`. It has to start at `16`

Aligning an object means storing it at a location that takes the fewest number of memory reads. Lets assume the granularity is `4 bytes` (can fetch `4 bytes` of data in a single read) and we have an object of size `4 bytes` which we want to fetch from memory. Consider the two scenarios: 
- The object is stored at address `0` (a multiple of the granularity). The processor can start reading from address `0` and can fetch the entire `4 byte` object in a single read.
- The object is stored at address `1` (not a multiple of the granularity) and ends at address `5`. The processor still has to start reading from address `0` and can only read up to address `3` in one read. It then has to carry out another read from address `4` to read the remaining `1 byte` of the object. It reads all the way up to address `7` even though it didn't contain our object. It takes a total of 2 reads to read the entire object.

![Memory Read](./Assets/1.jpg "")

In this case, we had to read 2 times the data. The same would have been true if the object was stored at address `2`, `3` or any other address that is not a multiple of `4` (the granularity). We can say the object had an **alignment requirement** of `4 bytes`.

## Alignment Requirement
How do we know what is the alignment requirement of an object? Luckily for us, the compiler can give us this information using the `alignof` operator.
    
```c++
alignof(char) // 1 byte
alignof(uint16_t) // 2 bytes
alignof(uint32_t) // 4 bytes
alignof(float) // 4 bytes
alignof(uint64_t) // 8 bytes
```
For a basic type like `int` or `float`, `alignof` returns the size of the type. For a struct, `alignof` returns the alignment requirement of the largest element in the structure.

An allocator should have the following properties:
- Always allocate an object at a address that is a multiple of its alignment requirement.
- If the current address is not a multiple of the alignment requirement, add padding before the allocation.
- Always return the aligned address.




