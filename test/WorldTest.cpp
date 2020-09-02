//          Copyright Dominic Koepke 2020 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#include <optional>

#include "Simple-ECS/World.hpp"

#include "catch.hpp"
#include "TestUtility.hpp"

TEST_CASE("World system managing tests", "[World]")
{
	using namespace secs::test;

	secs::World world;
	auto& testSystem = world.registerSystem(TestSystem{});

	SECTION("system get functions")
	{
		REQUIRE(&testSystem == world.findSystem<TestSystem>());
		REQUIRE(&testSystem == std::as_const(world).findSystem<TestSystem>());
		REQUIRE(&testSystem == &world.system<TestSystem>());
		REQUIRE(&testSystem == &std::as_const(world).system<TestSystem>());

		REQUIRE(&testSystem == world.findSystemByComponentType<TestComponent>());
		REQUIRE(&testSystem == std::as_const(world).findSystemByComponentType<TestComponent>());
		REQUIRE(&testSystem == &world.systemByComponentType<TestComponent>());
		REQUIRE(&testSystem == &std::as_const(world).systemByComponentType<TestComponent>());

		REQUIRE(world.findSystem<Test2System>() == nullptr);
		REQUIRE(std::as_const(world).findSystem<Test2System>() == nullptr);
		REQUIRE_THROWS(world.system<Test2System>());
		REQUIRE_THROWS(std::as_const(world).system<Test2System>());

		REQUIRE(world.findSystemByComponentType<Test2Component>() == nullptr);
		REQUIRE(std::as_const(world).findSystemByComponentType<Test2Component>() == nullptr);
		REQUIRE_THROWS(world.systemByComponentType<Test2Component>());
		REQUIRE_THROWS(std::as_const(world).systemByComponentType<Test2Component>());
	}

	auto& entity = world.createEntity<TestComponent>();
	auto uid = entity.uid();

	REQUIRE(&entity == world.findEntity(uid));
	REQUIRE(&entity == std::as_const(world).findEntity(uid));
	REQUIRE(&entity == &world.entity(uid));
	REQUIRE(&entity == &std::as_const(world).entity(uid));

	auto& testComponent = entity.component<TestComponent>();
	REQUIRE(testComponent.data == 0);
	world.preUpdate();
	REQUIRE(testComponent.data == 1);
	world.update(0);
	REQUIRE(testComponent.data == 3);
	world.postUpdate();
	REQUIRE(testComponent.data == 7);

	world.destroyEntityLater(uid);
	REQUIRE(&entity == world.findEntity(uid));
	world.postUpdate(); // force entity to be destroyed
	REQUIRE(world.findEntity(uid) == nullptr);
	REQUIRE(std::as_const(world).findEntity(uid) == nullptr);
	REQUIRE_THROWS(world.entity(uid));
	REQUIRE_THROWS(std::as_const(world).entity(uid));
}
