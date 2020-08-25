
//          Copyright Dominic Koepke 2020 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#ifndef SECS_ENTITY_HPP
#define SECS_ENTITY_HPP

#pragma once

#include <cassert>
#include <memory>
#include <stdexcept>

#include "ComponentStorage.hpp"
#include "Concepts.hpp"
#include "Defines.hpp"
#include "Typedefs.hpp"

namespace secs
{
	class EntityError final :
		public std::runtime_error
	{
	public:
		EntityError(const std::string& msg) :
			std::runtime_error(msg)
		{}

		EntityError(const char* msg) :
			std::runtime_error(msg)
		{}
	};

	class Entity
	{
	public:
		Entity(Uid uid, std::unique_ptr<BaseComponentStorage> componentStorage) :
			m_UID{ uid },
			m_ComponentStorage{ std::move(componentStorage) }
		{
			assert(uid);
			assert(m_ComponentStorage);
			m_ComponentStorage->setupEntity(*this);
		}

		[[nodiscard]] constexpr Uid getUID() const noexcept
		{
			return m_UID;
		}

		[[nodiscard]] constexpr EntityState getState() const noexcept
		{
			return m_State;
		}

		void changeState(EntityState state) noexcept
		{
			assert(static_cast<int>(m_State) < static_cast<int>(state));
			m_State = state;

			assert(m_ComponentStorage);
			m_ComponentStorage->onEntityStateChanged(*this);

		}

		template <Component TComponent>
		[[nodiscard]] bool hasComponent() const noexcept
		{
			assert(m_ComponentStorage);
			return m_ComponentStorage->hasComponent<TComponent>();
		}

		template <Component TComponent>
		[[nodiscard]] const TComponent* getComponentPtr() const noexcept
		{
			assert(m_ComponentStorage);
			return m_ComponentStorage->getComponent<TComponent>();
		}

		template <Component TComponent>
		[[nodiscard]] TComponent* getComponentPtr() noexcept
		{
			assert(m_ComponentStorage);
			return m_ComponentStorage->getComponent< TComponent>();
		}

		template <Component TComponent>
		[[nodiscard]] const TComponent& getComponent() const noexcept
		{
			assert(m_ComponentStorage);
			auto component = m_ComponentStorage->getComponent<TComponent>();
			assert(component);
			return *component;
		}

		template <Component TComponent>
		[[nodiscard]] TComponent& getComponent() noexcept
		{
			return const_cast<TComponent&>(std::as_const(*this).getComponent<TComponent>());
		}

	private:
		Uid m_UID = 0;
		EntityState m_State = EntityState::none;
		std::unique_ptr<BaseComponentStorage> m_ComponentStorage;
	};

	struct LessEntityByUID
	{
		template <class TLhs, class TRhs>
		[[nodiscard]] bool operator ()(const TLhs& lhs, const TRhs& rhs) const noexcept
		{
			return getUID(lhs) < getUID(rhs);
		}

	private:
		[[nodiscard]] constexpr static Uid getUID(const Entity& entity) noexcept
		{
			return entity.getUID();
		}

		[[nodiscard]] static Uid getUID(const std::unique_ptr<Entity>& entityPtr) noexcept
		{
			assert(entityPtr);
			return entityPtr->getUID();
		}

		[[nodiscard]] constexpr static Uid getUID(Uid uid) noexcept
		{
			return uid;
		}
	};
}

#endif
