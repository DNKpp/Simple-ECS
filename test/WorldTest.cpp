
//          Copyright Dominic Koepke 2017 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#include "catch.hpp"
#include "../include/Simple-ECS/World.hpp"
#include <optional>

TEST_CASE("World system managing tests", "[System]")
{
	using namespace secs;

	struct TestComponent
	{
		float data = 0;
	};

	class TestSystem :
		public secs::SystemBase<TestComponent>
	{
	public:
	};

	World world;
	world.registerSystem(TestSystem{});

	auto& entity = world.createEntity<TestComponent>();
	world.postUpdate();
}
