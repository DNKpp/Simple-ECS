
//          Copyright Dominic Koepke 2017 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#include "catch.hpp"
#include "../include/Simple-ECS/System.hpp"
#include "../include/Simple-ECS/ComponentStorage.hpp"
#include <optional>
#include <memory>

TEST_CASE("ComponentHandle tests", "[System]")
{
	struct TestComponent
	{
		float data = 0;
	};

	class TestSystem :
		public secs::SystemBase<TestComponent>
	{
	};

	TestSystem testSystem;

	REQUIRE(std::size(testSystem) == 0);
	REQUIRE(std::empty(testSystem));
	REQUIRE(testSystem.componentCount() == 0);

	auto handle = testSystem.createComponent(1);
	auto uid = handle.getUID();
	REQUIRE(std::size(testSystem) == 1);
	REQUIRE(!std::empty(testSystem));
	REQUIRE(testSystem.componentCount() == 1);
	REQUIRE(testSystem.getComponentPtr(uid) != nullptr);
	REQUIRE(std::addressof(testSystem.getComponent(uid)) == std::addressof(handle.getComponent()));

	REQUIRE(handle.getUID() != 0u);
	REQUIRE(handle.getSystemPtr() == &testSystem);
	REQUIRE(!handle.isEmpty());
	REQUIRE(handle);

	SECTION("ComponentHandle move checks")
	{
		std::optional<TestSystem::ComponentHandle> secHandle;
		SECTION("Move CTor")
		{
			secHandle.emplace(std::move(handle));
		}

		SECTION("Move Assign")
		{
			secHandle.emplace(std::move(handle));
			std::swap(*secHandle, handle);
			*secHandle = std::move(handle);
		}

		REQUIRE(handle.getUID() == 0u);
		REQUIRE(handle.getSystemPtr() == nullptr);
		REQUIRE(handle.isEmpty());
		REQUIRE(!handle);

		REQUIRE(secHandle->getUID() != 0u);
		REQUIRE(secHandle->getSystemPtr() == &testSystem);
		REQUIRE(!secHandle->isEmpty());
		REQUIRE(secHandle);

		// nothing should happen, because handle is already empty
		handle.release();
		REQUIRE(std::size(testSystem) == 1);
		REQUIRE(!std::empty(testSystem));
		REQUIRE(testSystem.componentCount() == 1);
		REQUIRE(testSystem.getComponentPtr(uid) != nullptr);
		REQUIRE(std::addressof(testSystem.getComponent(uid)) == std::addressof(secHandle->getComponent()));

		// this should have an impact on the system component count
		secHandle->release();
		REQUIRE(std::size(testSystem) == 1);
		REQUIRE(!std::empty(testSystem));
		REQUIRE(testSystem.componentCount() == 0u);
		REQUIRE(testSystem.getComponentPtr(uid) == nullptr);

		REQUIRE(secHandle->getUID() == 0u);
		REQUIRE(secHandle->getSystemPtr() == nullptr);
		REQUIRE(secHandle->isEmpty());
		REQUIRE(!(*secHandle));
	}
}

TEST_CASE("ComponentStorage tests", "[System]")
{
	using namespace secs;

	struct TestComponent
	{
		float data = 0;
	};

	class TestSystem :
		public secs::SystemBase<TestComponent>
	{
	};

	TestSystem testSystem;
	UID uid = 0;

	{
		std::unique_ptr<BaseComponentStorage> storage = std::make_unique<ComponentStorage<TestComponent>>(testSystem.createComponent(1));
		REQUIRE(storage->hasComponent<TestComponent>());
		REQUIRE(!storage->hasComponent<int>());

		REQUIRE(std::size(testSystem) == 1);
		REQUIRE(!std::empty(testSystem));
		REQUIRE(testSystem.componentCount() == 1);
	}

	REQUIRE(std::size(testSystem) == 1);
	REQUIRE(!std::empty(testSystem));
	REQUIRE(testSystem.componentCount() == 0);
}

TEST_CASE("System update tests", "[System]")
{
	using secs::UID;

	struct TestComponent
	{
		float data = 0;
	};

	class TestSystem :
		public secs::SystemBase<TestComponent>
	{
	public:
		void preUpdate() noexcept override
		{
			forEachComponent(
				[](UID entityUID, auto& component)
				{
					component.data += 1;
				}
			);
		}

		void update(float delta) noexcept override
		{
			forEachComponent(
				[](UID entityUID, auto& component)
				{
					component.data += 2;
				}
			);
		}

		void postUpdate() noexcept override
		{
			forEachComponent(
				[](UID entityUID, auto& component)
				{
					component.data += 4;
				}
			);
		}
	};

	TestSystem testSystem;

	auto handle = testSystem.createComponent(1);

	testSystem.preUpdate();
	REQUIRE(handle.getComponent().data == 1);

	testSystem.update(1.f);
	REQUIRE(handle.getComponent().data == 3);

	testSystem.postUpdate();
	REQUIRE(handle.getComponent().data == 7);
}