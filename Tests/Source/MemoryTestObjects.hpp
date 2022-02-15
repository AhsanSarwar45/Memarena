#pragma once

#include <vector>

struct TestObject
{
	int a;
	float b;
	char c;
	bool d;
	float e;

	TestObject(int _a, float _b, char _c, bool _d, float _e)
		: a(_a), b(_b), c(_c), d(_d), e(_e) {}
};

struct TestObject2
{
	int a;
	double b;
	double c;
	bool d;
	std::vector<int> e;

	TestObject2(int _a, double _b, double _c, bool _d, const std::vector<int>& _e)
		: a(_a), b(_b), c(_c), d(_d), e(_e) {}
};

struct TestObject3
{
	TestObject2 a;
	TestObject b;
	char c;
	bool d;
	float e;

	TestObject3(const TestObject2& _a, const TestObject& _b, char _c, bool _d, float _e)
		: a(_a), b(_b), c(_c), d(_d), e(_e) {}
};
