//          Copyright Dominic Koepke 2020 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#ifndef SECS_ENTITY_HPP
#define SECS_ENTITY_HPP

#pragma once

#include <cassert>
#include <memory>
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

	/**
	 * \brief RAII wrapper for a collection of Components
	 *
	 * This class actually acts as a RAII wrapper for its associated Components. During Construction it receives handle like infos
	 * for its "owning" Components and will destruct them during its own destruction. Each Entity has its own unique identifier (uid) which
	 * will never reused during a programs session. It is safe to store references or pointers to a specific Entity object, because an Entity
	 * will never be moved around in memory during its lifetime.
	 */
	class Entity
	{
		friend class World;

	public:
		Entity(const Entity&) = delete;
		Entity& operator =(const Entity&) = delete;
		Entity(Entity&&) = delete;
		Entity& operator =(Entity&&) = delete;

		/**
		 * \brief Constructor
		 * \param uid Unique identifier for this Entity
		 * \param componentInfos Infos for the Components this Entity will be responsible for.
		 */
		explicit Entity(Uid uid, std::vector<detail::ComponentStorageInfo> componentInfos) :
			m_Uid{ uid },
			m_ComponentInfos{ std::move(componentInfos) }
		{
			assert(uid != 0);
			setComponentEntity();
		}

		/**
		 * \brief Destructor
		 */
		~Entity() noexcept
		{
			for (auto& info : m_ComponentInfos)
			{
				assert(info.rtti && info.systemPtr && info.componentUid != 0);
				info.rtti->destroy(info.systemPtr, info.componentUid);
			}
		}

		/**
		 * \brief Unique identifier
		 * \return Returns the uid of this Entity.
		 */
		[[nodiscard]] constexpr Uid uid() const noexcept
		{
			return m_Uid;
		}

		/**
		 * \brief Current state
		 * \return Returns the current \ref EntityState of this Entity.
		 */
		[[nodiscard]] constexpr EntityState state() const noexcept
		{
			return m_State;
		}

		/**
		 * \brief Checks if Component is present
		 *
		 * \remark This function does not perform any inheritance checks, thus you can always query for concrete Component types.
		 * \tparam TComponent Expected Component type.
		 * \return True if Entity has Component of type TComponent.
		 */
		template <Component TComponent>
		[[nodiscard]] bool hasComponent() const noexcept
		{
			return findComponentInfo<TComponent>(m_ComponentInfos) != end(m_ComponentInfos);
		}

		/**
		 * \brief Queries for a specific Component type
		 *
		 * This function searches for a specific Component type and returns a const pointer to the caller.
		  * \remark This function does not perform any inheritance checks, thus you can always query for concrete Component types.
		 * \tparam TComponent Expected Component type.
		 * \return Const pointer to the stored Component object or nullptr if not found.
		 */
		template <Component TComponent>
		[[nodiscard]] const TComponent* findComponent() const noexcept
		{
			if (auto itr = findComponentInfo<TComponent>(m_ComponentInfos); itr != std::end(m_ComponentInfos))
			{
				assert(isValid(*itr));
				return static_cast<const TComponent*>(itr->rtti->findComponent(itr->systemPtr, itr->componentUid));
			}
			return nullptr;
		}

		/**
		 * \brief Queries for a specific Component type
		 *
		 * This function searches for a specific Component type and returns a pointer to the caller.
		  * \remark This function does not perform any inheritance checks, thus you can always query for concrete Component types.
		 * \tparam TComponent Expected Component type.
		 * \return Pointer to the stored Component object or nullptr if not found.
		 */
		template <Component TComponent>
		[[nodiscard]] TComponent* findComponent() noexcept
		{
			return const_cast<TComponent*>(std::as_const(*this).findComponent<TComponent>());
		}

		/**
		 * \brief Queries for a specific Component type
		 *
		 * This function searches for a specific Component type and returns a const reference to the caller.
		  * \remark This function does not perform any inheritance checks, thus you can always query for concrete Component types.
		 * \tparam TComponent Expected Component type.
		 * \throws EntityError if a Component object could not be found.
		 * \return Const reference to the stored Component object.
		 */
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

		/**
		 * \brief Queries for a specific Component type
		 *
		 * This function searches for a specific Component type and returns a reference to the caller.
		 * \remark This function does not perform any inheritance checks, thus you can always query for concrete Component types.
		 * \tparam TComponent Expected Component type.
		 * \throws EntityError if a Component object could not be found.
		 * \return Reference to the stored Component object.
		 */
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
		EntityState m_State = EntityState::none;
		std::vector<detail::ComponentStorageInfo> m_ComponentInfos;

		template <class TComponent, class TContainer>
		static auto findComponentInfo(TContainer& container)
		{
			std::type_index expectedTypeIndex = typeid(std::remove_cvref_t<TComponent>);
			return std::ranges::find(container, expectedTypeIndex, [](const detail::ComponentStorageInfo& info) { return info.componentTypeIndex; });
		}

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
