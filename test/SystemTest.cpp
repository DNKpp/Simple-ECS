
//          Copyright Dominic Koepke 2017 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#include "catch.hpp"
#include "../include/Simple-ECS/System.hpp"
#include <optional>

struct HpComponent
{
	float hp = 0;
};

class HpSystem :
	public secs::SystemBase<HpComponent>
{
};

TEST_CASE("ComponentHandle tests", "[System]")
{
	HpSystem hpSystem;

	REQUIRE(std::size(hpSystem) == 0);
	REQUIRE(std::empty(hpSystem));
	REQUIRE(hpSystem.componentCount() == 0);

	auto handle = hpSystem.createComponent(1);
	REQUIRE(std::size(hpSystem) == 1);
	REQUIRE(!std::empty(hpSystem));
	REQUIRE(hpSystem.componentCount() == 1);
	REQUIRE(handle.getComponentHolder() == &hpSystem);
	REQUIRE(handle.getRawPtr() != nullptr);
	REQUIRE(handle.getUID() != 0u);

	SECTION("ComponentHandle move checks")
	{
		std::optional<HpSystem::ComponentHandle> secHandle;
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

		REQUIRE(handle.getComponentHolder() == nullptr);
		REQUIRE(handle.getRawPtr() == nullptr);
		REQUIRE(handle.getUID() == 0u);
		REQUIRE(secHandle);
		REQUIRE(secHandle->getComponentHolder() == &hpSystem);
		REQUIRE(secHandle->getRawPtr() != nullptr);
		REQUIRE(secHandle->getUID() != 0u);

		handle.release();
		REQUIRE(std::size(hpSystem) == 1);
		REQUIRE(!std::empty(hpSystem));
		REQUIRE(hpSystem.componentCount() == 1);

		secHandle->release();
		REQUIRE(std::size(hpSystem) == 1);
		REQUIRE(!std::empty(hpSystem));
		REQUIRE(hpSystem.componentCount() == 0u);
		REQUIRE(secHandle->getComponentHolder() == nullptr);
		REQUIRE(secHandle->getRawPtr() == nullptr);
		REQUIRE(secHandle->getUID() == 0u);
	}
}

//TEST_CASE("Vector operator test", "[Vector]")
//{
//	using namespace georithm;
//
//	Vector<int, 2> vec;
//	auto vec2 = vec;
//	REQUIRE(vec == vec2);
//	REQUIRE(!(vec2 != vec));
//
//	vec2 = { 7, 2 };
//	REQUIRE(vec2 == Vector{ 7, 2 });
//	REQUIRE(vec2.x()== 7);
//	REQUIRE(vec2.y() == 2);
//	REQUIRE(vec + vec2 == Vector{ 7, 2 });
//
//	Vector real{ 1., 5. };
//	auto real2 = real;
//	real += real2;
//	REQUIRE(real == real2 * 2);
//	real -= real2;
//	REQUIRE(real == real2);
//
//	real += 1;
//	REQUIRE(real == real2 + 1);
//	real -= 2;
//	REQUIRE(real == real2 - 1);
//	real *= 3;
//	REQUIRE(real == (real2 - 1) * 3);
//	real /= 4;
//	REQUIRE(real == ((real2 - 1) * 3) / 4);
//	//real %= 5;
//	vec2 %= 5;
//	REQUIRE(vec2 == Vector{ 2, 2 });
//}
//
//TEST_CASE("Vector algorithm test", "[Vector]")
//{
//	using namespace georithm;
//
//	Vector<int, 2> vec{ 1, 2 };
//	REQUIRE(lengthSq(vec) == (vec.x() * vec.x() + vec.y() * vec.y()));
//	REQUIRE(scalarProduct(vec, vec) == lengthSq(vec));
//	REQUIRE(length(vec) == 2);	// sqrt converted to int
//	REQUIRE(length<double>(vec) == Approx(std::sqrt((vec.x() * vec.x() + vec.y() * vec.y()))));
//
//	REQUIRE(length(normalize(vec)) == Approx(1));
//}
//
