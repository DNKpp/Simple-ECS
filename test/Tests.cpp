//          Copyright Dominic Koepke 2020 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#include <optional>

#include "Simple-ECS/World.hpp"

#include "catch.hpp"
#include "TestUtility.hpp"

using namespace secs::test;

secs::World world;
auto& testSystem = world.registerSystem(TestSystem{});
secs::Uid uid = 0;

TEST_CASE("World system managing tests", "[World]")
{
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

	SECTION("system - component get functions with empty")
	{
		REQUIRE(testSystem.empty());
		REQUIRE(testSystem.size() == 0);

		REQUIRE(testSystem.findComponent(0) == nullptr);
		REQUIRE(std::as_const(testSystem).findComponent(0) == nullptr);
		REQUIRE_THROWS(testSystem.component(0));
		REQUIRE_THROWS(std::as_const(testSystem).component(0));

		REQUIRE(testSystem.findComponent(std::numeric_limits<secs::Uid>::max()) == nullptr);
		REQUIRE(std::as_const(testSystem).findComponent(std::numeric_limits<secs::Uid>::max()) == nullptr);
		REQUIRE_THROWS(testSystem.component(std::numeric_limits<secs::Uid>::max()));
		REQUIRE_THROWS(std::as_const(testSystem).component(std::numeric_limits<secs::Uid>::max()));
	}

	SECTION("entity construction")
	{
		auto& entity = world.createEntity<TestComponent>();
		uid = entity.uid();

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
	}

	SECTION("system - component get functions with one element")
	{
		REQUIRE(!testSystem.empty());
		REQUIRE(testSystem.size() == 1);

		REQUIRE(testSystem.findComponent(0) == nullptr);
		REQUIRE(std::as_const(testSystem).findComponent(0) == nullptr);
		REQUIRE_THROWS(testSystem.component(0));
		REQUIRE_THROWS(std::as_const(testSystem).component(0));

		REQUIRE(testSystem.findComponent(1) != nullptr);
		REQUIRE(std::as_const(testSystem).findComponent(1) != nullptr);
		REQUIRE_NOTHROW(testSystem.component(1));
		REQUIRE_NOTHROW(std::as_const(testSystem).component(1));

		REQUIRE(testSystem.findComponent(std::numeric_limits<secs::Uid>::max()) == nullptr);
		REQUIRE(std::as_const(testSystem).findComponent(std::numeric_limits<secs::Uid>::max()) == nullptr);
		REQUIRE_THROWS(testSystem.component(std::numeric_limits<secs::Uid>::max()));
		REQUIRE_THROWS(std::as_const(testSystem).component(std::numeric_limits<secs::Uid>::max()));
	}

	SECTION("entity destruction")
	{
		world.destroyEntityLater(uid);
		auto* entity = world.findEntity(uid);
		REQUIRE(entity != nullptr);
		REQUIRE(entity->uid() == uid);

		world.postUpdate(); // entity will set to teardown state
		REQUIRE(world.findEntity(uid) == entity);
		REQUIRE(std::as_const(world).findEntity(uid) == entity);
		REQUIRE_NOTHROW(world.entity(uid));
		REQUIRE_NOTHROW(std::as_const(world).entity(uid));

		world.postUpdate(); // entity will be deleted here
		REQUIRE(world.findEntity(uid) == nullptr);
		REQUIRE(std::as_const(world).findEntity(uid) == nullptr);
		REQUIRE_THROWS(world.entity(uid));
		REQUIRE_THROWS(std::as_const(world).entity(uid));
	}

	SECTION("system - component get functions empty again")
	{
		REQUIRE(testSystem.empty());
		REQUIRE(testSystem.size() == 0);

		REQUIRE(testSystem.findComponent(0) == nullptr);
		REQUIRE(std::as_const(testSystem).findComponent(0) == nullptr);
		REQUIRE_THROWS(testSystem.component(0));
		REQUIRE_THROWS(std::as_const(testSystem).component(0));

		REQUIRE(testSystem.findComponent(1) == nullptr);
		REQUIRE(std::as_const(testSystem).findComponent(1) == nullptr);
		REQUIRE_THROWS(testSystem.component(1));
		REQUIRE_THROWS(std::as_const(testSystem).component(1));

		REQUIRE(testSystem.findComponent(std::numeric_limits<secs::Uid>::max()) == nullptr);
		REQUIRE(std::as_const(testSystem).findComponent(std::numeric_limits<secs::Uid>::max()) == nullptr);
		REQUIRE_THROWS(testSystem.component(std::numeric_limits<secs::Uid>::max()));
		REQUIRE_THROWS(std::as_const(testSystem).component(std::numeric_limits<secs::Uid>::max()));
	}

	SECTION("system - component get functions recycle test")
	{
		auto& entity = world.createEntity<TestComponent>();

		REQUIRE(!testSystem.empty());
		REQUIRE(testSystem.size() == 1);

		REQUIRE(testSystem.findComponent(0) == nullptr);
		REQUIRE(std::as_const(testSystem).findComponent(0) == nullptr);
		REQUIRE_THROWS(testSystem.component(0));
		REQUIRE_THROWS(std::as_const(testSystem).component(0));

		REQUIRE(testSystem.findComponent(1) != nullptr);
		REQUIRE(std::as_const(testSystem).findComponent(1) != nullptr);
		REQUIRE_NOTHROW(testSystem.component(1));
		REQUIRE_NOTHROW(std::as_const(testSystem).component(1));

		REQUIRE(testSystem.findComponent(std::numeric_limits<secs::Uid>::max()) == nullptr);
		REQUIRE(std::as_const(testSystem).findComponent(std::numeric_limits<secs::Uid>::max()) == nullptr);
		REQUIRE_THROWS(testSystem.component(std::numeric_limits<secs::Uid>::max()));
		REQUIRE_THROWS(std::as_const(testSystem).component(std::numeric_limits<secs::Uid>::max()));
	}
}
