#pragma once

#include <type_traits>

namespace Memarena
{
template <class T>
concept Allocatable = std::negation<std::is_same<T, void>>::value;

template <typename T>
concept Integral = std::is_integral<T>::value;
} // namespace Memarena