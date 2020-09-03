//          Copyright Dominic Koepke 2020 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#ifndef SECS_UTILS_EMPTY_CALLABLE_HPP
#define SECS_UTILS_EMPTY_CALLABLE_HPP

#pragma once

namespace secs::utils
{
	template <class TReturn = void>
	struct EmptyCallable
	{
		EmptyCallable() = default;

		template <class... TArgs>
		constexpr TReturn operator ()(TArgs&&...) const noexcept
		{
			if constexpr (!std::is_same_v<void, TReturn>)
				return {};
		}
	};
}

#endif
