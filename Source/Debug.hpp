#pragma once

#include <PCH.hpp>

#define STRINGIZE(arg)  STRINGIZE1(arg)
#define STRINGIZE1(arg) STRINGIZE2(arg)
#define STRINGIZE2(arg) #arg

#define CONCATENATE(arg1, arg2)  CONCATENATE1(arg1, arg2)
#define CONCATENATE1(arg1, arg2) CONCATENATE2(arg1, arg2)
#define CONCATENATE2(arg1, arg2) arg1##arg2

#define MEMBER_SIZE(type, member) sizeof(((type*)0)->member)

#define PRINT_INFO(structure, field)                                                                              \
    printf(STRINGIZE(field) ": Size: %d bytes, Offset: %d bytes, End: %d bytes\n", MEMBER_SIZE(structure, field), \
                     offsetof(structure, field), MEMBER_SIZE(structure, field) + offsetof(structure, field))

/* PRN_STRUCT_OFFSETS will print offset of each of the fields
 within structure passed as the first argument.
 */
#define PRN_STRUCT_OFFSETS_1(structure, field, ...) PRINT_INFO(structure, field);

#define PRN_STRUCT_OFFSETS_2(structure, field, ...) \
    PRINT_INFO(structure, field);                   \
    PRN_STRUCT_OFFSETS_1(structure, __VA_ARGS__)
#define PRN_STRUCT_OFFSETS_3(structure, field, ...) \
    PRINT_INFO(structure, field);                   \
    PRN_STRUCT_OFFSETS_2(structure, __VA_ARGS__)
#define PRN_STRUCT_OFFSETS_4(structure, field, ...) \
    PRINT_INFO(structure, field);                   \
    PRN_STRUCT_OFFSETS_3(structure, __VA_ARGS__)
#define PRN_STRUCT_OFFSETS_5(structure, field, ...) \
    PRINT_INFO(structure, field);                   \
    PRN_STRUCT_OFFSETS_4(structure, __VA_ARGS__)
#define PRN_STRUCT_OFFSETS_6(structure, field, ...) \
    PRINT_INFO(structure, field);                   \
    PRN_STRUCT_OFFSETS_5(structure, __VA_ARGS__)
#define PRN_STRUCT_OFFSETS_7(structure, field, ...) \
    PRINT_INFO(structure, field);                   \
    PRN_STRUCT_OFFSETS_6(structure, __VA_ARGS__)
#define PRN_STRUCT_OFFSETS_8(structure, field, ...) \
    PRINT_INFO(structure, field);                   \
    PRN_STRUCT_OFFSETS_7(structure, __VA_ARGS__)

#define PRN_STRUCT_OFFSETS_NARG(...)                                     PRN_STRUCT_OFFSETS_NARG_(__VA_ARGS__, PRN_STRUCT_OFFSETS_RSEQ_N())
#define PRN_STRUCT_OFFSETS_NARG_(...)                                    PRN_STRUCT_OFFSETS_ARG_N(__VA_ARGS__)
#define PRN_STRUCT_OFFSETS_ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, N, ...) N
#define PRN_STRUCT_OFFSETS_RSEQ_N()                                      8, 7, 6, 5, 4, 3, 2, 1, 0

#define PRN_STRUCT_OFFSETS_(N, structure, field, ...) CONCATENATE(PRN_STRUCT_OFFSETS_, N)(structure, field, __VA_ARGS__)

#define PRN_STRUCT_OFFSETS(structure, field, ...)                                                    \
    printf("---------------------------------------------\n");                                       \
    printf("%s: %d\n", #structure, sizeof(structure));                                               \
    PRN_STRUCT_OFFSETS_(PRN_STRUCT_OFFSETS_NARG(field, __VA_ARGS__), structure, field, __VA_ARGS__); \
    printf("---------------------------------------------\n")
