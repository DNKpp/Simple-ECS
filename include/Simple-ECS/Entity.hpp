//          Copyright Dominic Koepke 2020 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#ifndef SECS_ENTITY_HPP
#define SECS_ENTITY_HPP

#pragma once

#include <cassert>
#include <ranges>
#include <stdexcept>
#include <typeindex>
#include <vector>

#include "Concepts.hpp"
#include "Defines.hpp"
#include "System.hpp"

namespace secs
{
	class Entity;
}

namespace secs
{
	class EntityError final :
		public std::runtime_error
	{
	public:
		explicit EntityError(const std::string& msg) :
			std::runtime_error(msg)
		{
		}

		explicit EntityError(const char* msg) :
			std::runtime_error(msg)
		{
		}
	};

	class Entity
	{
	public:
		Entity(const Entity&) = delete;
		Entity& operator =(const Entity&) = delete;
		Entity(Entity&&) = delete;
		Entity& operator =(Entity&&) = delete;

		explicit Entity(Uid uid, std::vector<detail::ComponentStorageInfo> componentInfos) :
			m_Uid{ uid },
			m_ComponentInfos{ std::move(componentInfos) }
		{
			assert(uid != 0);
			setComponentEntity();
		}

		~Entity() noexcept
		{
			for (auto& info : m_ComponentInfos)
			{
				assert(info.rtti && info.systemPtr && info.componentUid != 0);
				info.rtti->destroy(info.systemPtr, info.componentUid);
			}
		}

		[[nodiscard]] constexpr Uid uid() const noexcept
		{
			return m_Uid;
		}

		[[nodiscard]] constexpr EntityState state() const noexcept
		{
			return m_State;
		}

		using enum EntityState;
		void changeState(EntityState state)
		{
			assert(static_cast<int>(m_State) < static_cast<int>(state));
			m_State = state;

			for (auto& info : m_ComponentInfos)
			{
				assert(isValid(info));
				info.rtti->entityStateChanged(info.systemPtr, info.componentUid);
			}
		}

		template <Component TComponent>
		[[nodiscard]] bool hasComponent() const noexcept
		{
			return findComponentInfo<TComponent>(m_ComponentInfos) != end(m_ComponentInfos);
		}

		template <Component TComponent>
		[[nodiscard]] const TComponent* findComponent() const noexcept
		{
			if (auto itr = findComponentInfo<TComponent>(m_ComponentInfos); itr != std::end(m_ComponentInfos))
			{
				assert(isValid(*itr));
				return static_cast<const TComponent*>(itr->rtti->getComponentPtr(itr->systemPtr, itr->componentUid));
			}
			return nullptr;
		}

		template <Component TComponent>
		[[nodiscard]] TComponent* findComponent() noexcept
		{
			return const_cast<TComponent*>(std::as_const(*this).findComponent<TComponent>());
		}

		template <Component TComponent>
		[[nodiscard]] const TComponent& component() const
		{
			if (auto* componentPtr = findComponent<TComponent>())
			{
				return *componentPtr;
			}
			using namespace std::string_literals;
			throw EntityError("Component not found: "s + typeid(TComponent).name());
		}

		template <Component TComponent>
		[[nodiscard]] TComponent& component()
		{
			if (auto* componentPtr = findComponent<TComponent>())
			{
				return *componentPtr;
			}
			using namespace std::string_literals;
			throw EntityError("Component not found: "s + typeid(TComponent).name());
		}

	private:
		Uid m_Uid = 0;
		EntityState m_State = none;
		std::vector<detail::ComponentStorageInfo> m_ComponentInfos;

		template <class TComponent, class TContainer>
		static auto findComponentInfo(TContainer& container)
		{
			std::type_index expectedTypeIndex = typeid(std::remove_cvref_t<TComponent>);
			return std::ranges::find(container, expectedTypeIndex, [](const detail::ComponentStorageInfo& info) { return info.componentTypeIndex; });
		}

		void setComponentEntity() noexcept
		{
			for (auto& info : m_ComponentInfos)
			{
				assert(isValid(info));
				info.rtti->setEntity(info.systemPtr, info.componentUid, *this);
			}
		}
	};

	struct EntityLessByUid
	{
		template <class TLhs, class TRhs>
		[[nodiscard]] bool operator ()(const TLhs& lhs, const TRhs& rhs) const noexcept
		{
			return uid(lhs) < uid(rhs);
		}

	private:
		[[nodiscard]] constexpr static Uid uid(const Entity& entity) noexcept
		{
			return entity.uid();
		}

		[[nodiscard]] static Uid uid(const std::unique_ptr<Entity>& entityPtr) noexcept
		{
			assert(entityPtr);
			return entityPtr->uid();
		}

		[[nodiscard]] constexpr static Uid uid(Uid uid) noexcept
		{
			return uid;
		}
	};
}

#endif
