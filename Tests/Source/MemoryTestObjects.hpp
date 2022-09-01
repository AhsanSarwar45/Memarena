#pragma once

#include <vector>

struct TestObject
{
    int   a;
    float b;
    char  c;
    bool  d;
    float e;

    auto operator<=>(const TestObject&) const = default;

    TestObject(int _a, float _b, char _c, bool _d, float _e) : a(_a), b(_b), c(_c), d(_d), e(_e) {}
    TestObject() = default;
    // TestObject(const TestObject& other)
    // {
    //     a = other.a;
    //     b = other.b;
    //     c = other.c;
    //     d = other.d;
    //     e = other.e;
    // }
    // TestObject(TestObject& other)
    // {
    //     a = other.a;
    //     b = other.b;
    //     c = other.c;
    //     d = other.d;
    //     e = other.e;
    // }
    // TestObject(TestObject&& other)
    // {
    //     a = other.a;
    //     b = other.b;
    //     c = other.c;
    //     d = other.d;
    //     e = other.e;
    // }
};

struct Pair
{
    int   first;
    float second;

    auto operator<=>(const Pair&) const = default;
};

struct TestObject2
{
    int    a;
    double b;
    double c;
    bool   d;
    Pair   e;

    auto operator<=>(const TestObject2&) const = default;

    TestObject2(int _a, double _b, double _c, bool _d, const Pair& _e) : a(_a), b(_b), c(_c), d(_d), e(_e) {}
};

struct TestObject3
{
    TestObject2 a;
    TestObject  b;
    char        c;
    bool        d;
    float       e;

    auto operator<=>(const TestObject3&) const = default;

    TestObject3(const TestObject2& _a, const TestObject& _b, char _c, bool _d, float _e) : a(_a), b(_b), c(_c), d(_d), e(_e) {}
};
