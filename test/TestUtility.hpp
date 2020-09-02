//          Copyright Dominic Koepke 2020 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#ifndef SECS_TEST_UTILITY_HPP
#define SECS_TEST_UTILITY_HPP

#pragma once

#include "Simple-ECS/System.hpp"

namespace secs::test
{
	struct TestComponent
	{
		int data = 0;
	};

	class TestSystem final :
		public SystemBase<TestComponent>
	{
	public:
		void preUpdate() noexcept override
		{
			forEachComponent(
							[](Entity& entity, auto& component)
							{
								component.data += 1;
							}
							);
		}

		void update(float delta) noexcept override
		{
			forEachComponent(
							[](Entity& entity, auto& component)
							{
								component.data += 2;
							}
							);
		}

		void postUpdate() noexcept override
		{
			forEachComponent(
							[](Entity& entity, auto& component)
							{
								component.data += 4;
							}
							);
		}
	};

	struct Test2Component
	{
	};

	class Test2System final :
		public SystemBase<Test2Component>
	{
	public:
	};
}

#endif
