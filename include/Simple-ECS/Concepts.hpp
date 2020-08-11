
//          Copyright Dominic Koepke 2020 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#ifndef SECS_CONCEPTS_HPP
#define SECS_CONCEPTS_HPP

#pragma once

#include <concepts>
#include <type_traits>

namespace secs
{
	template <class T>
	concept Component = std::movable<T>;

	template <class T>
	concept System = std::move_constructible<T> && requires (T system)
	{
		typename T::ComponentType;
		typename T::ComponentHandle;
	} && Component<typename T::ComponentType>;

}

#endif
