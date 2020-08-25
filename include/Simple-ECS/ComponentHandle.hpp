//          Copyright Dominic Koepke 2020 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#ifndef SECS_COMPONENT_HANDLE_HPP
#define SECS_COMPONENT_HANDLE_HPP

#pragma once

#include "Defines.hpp"

#include <cassert>
#include <concepts>

namespace secs
{
	class Entity;

	template <class TSystem>
	class ComponentHandle
	{
	public:
		using SystemType = TSystem;
		using ComponentType = typename SystemType::ComponentType;

		ComponentHandle(const ComponentHandle&) = delete;
		ComponentHandle& operator =(const ComponentHandle&) = delete;

		constexpr ComponentHandle(Uid uid, SystemType& system) noexcept :
			m_UID{ uid },
			m_System{ &system }
		{
		}

		// ToDo: c++20
		/*constexpr*/
		~ComponentHandle() noexcept
		{
			if (!isEmpty())
			{
				getSystem().deleteComponent(m_UID);
			}
		}

		template <std::derived_from<TSystem> T2System>
		constexpr ComponentHandle(ComponentHandle<T2System>&& other) noexcept
		{
			*this = std::move(other);
		}

		template <std::derived_from<TSystem> T2System>
		constexpr ComponentHandle& operator =(ComponentHandle<T2System>&& other) noexcept
		{
			ComponentHandle tmp{ std::move(*this) };
			m_UID = std::exchange(other.m_UID, 0);
			m_System = std::exchange(other.m_System, nullptr);
			return *this;
		}

		constexpr ComponentHandle& operator =(ComponentHandle&& other) noexcept
		{
			using std::swap;
			swap(m_UID, other.m_UID);
			swap(m_System, other.m_System);
			return *this;
		}

		void setupEntity(Entity& entity) const noexcept
		{
			getSystem().setComponentEntity(getUID(), entity);
		}

		[[nodiscard]] constexpr Uid getUID() const noexcept
		{
			return m_UID;
		}

		[[nodiscard]] constexpr SystemType& getSystem() const noexcept
		{
			assert(m_System);
			return *m_System;
		}

		[[nodiscard]] constexpr SystemType* getSystemPtr() const noexcept
		{
			return m_System;
		}

		[[nodiscard]] constexpr ComponentType& getComponent() const noexcept
		{
			return getSystem().getComponent(m_UID);
		}

		[[nodiscard]] constexpr bool isEmpty() const noexcept
		{
			return m_UID == 0;
		}

		constexpr explicit operator bool() const noexcept
		{
			return !isEmpty();
		}

		constexpr void release() noexcept
		{
			*this = ComponentHandle{};
		}

	private:
		Uid m_UID = 0;
		SystemType* m_System = nullptr;

		constexpr ComponentHandle() noexcept = default;
	};
}

#endif
