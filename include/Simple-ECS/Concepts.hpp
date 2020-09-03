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
	/** \defgroup concepts Concepts 
	 */

	/** \var Component
	 * \brief Concept for Component types.
	 *
	 * Requires Component objects to be movable and destructible.
	 * \ingroup concepts
	 */
	template <class T>
	concept Component = std::movable<T> && std::destructible<T>;

	/** \var System
	 * \brief Concept for System types.
	 *
	 * Requires System objects to be move constructable, destructible and expose their corresponding Component type
	 * as CompponentType.
	 * \ingroup concepts
	 */
	template <class T>
	concept System = std::move_constructible<T> &&
	std::destructible<T> &&
	requires(T system)
	{
		typename T::ComponentType;
	} &&
	Component<typename T::ComponentType>;
}

#endif
