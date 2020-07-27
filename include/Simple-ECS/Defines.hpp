
//          Copyright Dominic Koepke 2020 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#ifndef SECS_DEFINES_HPP
#define SECS_DEFINES_HPP

#pragma once

namespace secs
{
	enum class EntityState
	{
		none,
		initializing,
		running,
		tearDown
	};
}

#endif
