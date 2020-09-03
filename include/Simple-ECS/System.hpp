//          Copyright Dominic Koepke 2020 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#ifndef SECS_SYSTEM_HPP
#define SECS_SYSTEM_HPP

#pragma once

#include <cassert>
#include <cstddef>
#include <optional>
#include <queue>
#include <stdexcept>
#include <string>
#include <typeindex>

#include "Defines.hpp"
#include "EmptyCallable.hpp"

namespace secs
{
	class Entity;

	class SystemError final :
		public std::runtime_error
	{
	public:
		SystemError(const std::string& msg) :
			std::runtime_error(msg)
		{
		}

		SystemError(const char* msg) :
			std::runtime_error(msg)
		{
		}
	};

	/**
	 * \brief Interface for System classes
	 */
	class ISystem
	{
	public:
		constexpr ISystem(const ISystem&) noexcept = delete;
		constexpr ISystem& operator =(const ISystem&) noexcept = delete;

		constexpr ISystem(ISystem&&) noexcept = default;
		constexpr ISystem& operator =(ISystem&&) noexcept = default;

		/*ToDo: c++20
		constexpr*/
		virtual ~ISystem() noexcept = default;

		virtual void preUpdate() = 0;
		virtual void update(float delta) = 0;
		virtual void postUpdate() = 0;

	protected:
		constexpr ISystem() noexcept = default;
	};

	template <class TComponent>
	class SystemBase;
}

namespace secs::detail
{
	struct ComponentRtti
	{
		using DestroyFn_t = void(void*, Uid) noexcept;
		using SetEntityFn_t = void(void*, Uid, Entity&) noexcept;
		using EntityStateChangeFn_t = void(void*, Uid);
		using FindComponentFn_t = const void*(const void*, Uid) noexcept;

		template <class TComponent>
		static void destroyImpl(void* targetSystem, Uid componentUid) noexcept
		{
			assert(targetSystem);
			auto& system = *static_cast<SystemBase<TComponent>*>(targetSystem);
			system.destroyComponent(componentUid);
		}

		template <class TComponent>
		static void setEntityImpl(void* targetSystem, Uid componentUid, Entity& entity) noexcept
		{
			assert(targetSystem);
			auto& system = *static_cast<SystemBase<TComponent>*>(targetSystem);
			system.setComponentEntity(componentUid, entity);
		}

		template <class TComponent>
		static void entityStateChangedImpl(void* targetSystem, Uid componentUid)
		{
			assert(targetSystem);
			auto& system = *static_cast<SystemBase<TComponent>*>(targetSystem);
			system.entityStateChanged(componentUid);
		}

		template <class TComponent>
		static const void* findComponentImpl(const void* targetSystem, Uid componentUid) noexcept
		{
			assert(targetSystem);
			auto& system = *static_cast<const SystemBase<TComponent>*>(targetSystem);
			return static_cast<const void*>(system.findComponent(componentUid));
		}

		DestroyFn_t* destroy;
		SetEntityFn_t* setEntity;
		EntityStateChangeFn_t* entityStateChanged;
		FindComponentFn_t* findComponent;
	};

	template <class TComponent>
	inline constexpr ComponentRtti componentRtti
	{
		&ComponentRtti::destroyImpl<TComponent>,
		&ComponentRtti::setEntityImpl<TComponent>,
		&ComponentRtti::entityStateChangedImpl<TComponent>,
		&ComponentRtti::findComponentImpl<TComponent>
	};

	struct ComponentStorageInfo
	{
		void* systemPtr = nullptr;
		Uid componentUid = 0;
		std::type_index componentTypeIndex{ typeid(void) };
		const ComponentRtti* rtti = nullptr;
	};

	[[nodiscard]] inline bool isValid(const ComponentStorageInfo& info) noexcept
	{
		return info.systemPtr != nullptr && info.componentUid != 0 && info.componentTypeIndex != typeid(void) && info.rtti != nullptr;
	}
}

namespace secs
{
	/**
	 * \brief Base class for custom Systems
	 *
	 * This is the class you should inherit from, when you are about to create a custom System for a corresponding Component type.
	 * There are some virtual member functions you could override to tweak the behaviour of your Systems.
	 * Each System type should only instantiated once during the runtime of your program.
	 * \tparam TComponent The associated Component type.
	 */
	template <class TComponent>
	// ReSharper disable once CppClassCanBeFinal
	class SystemBase :
		public ISystem
	{
	private:
		friend class World;
		friend struct detail::ComponentRtti;

		struct ComponentInfo
		{
			Entity* entity;
			TComponent component;
		};

	public:
		/**
		 * \brief Alias for the associated Component type.
		 */
		using ComponentType = TComponent;

		SystemBase(const SystemBase&) = delete;
		SystemBase& operator =(const SystemBase&) = delete;

		/**
		 * \brief Move constructor
		 */
		SystemBase(SystemBase&&) /* NOT noexcept*/ = default;

		/**
		 * \brief Move assignment
		 */
		SystemBase& operator =(SystemBase&&) /* NOT noexcept*/ = default;

		/**
		 * \brief Destructor
		 */
		~SystemBase() noexcept override = default;

		/**
		 * \brief Checks if Component is present and active
		 * \param uid Uid of the Component object.
		 * \return True if Component object is present and active.
		 */
		[[nodiscard]] constexpr bool hasComponent(Uid uid) const noexcept
		{
			return 0u < uid && uid <= std::size(m_Components) && m_Components[uid - 1u];
		}

		/**
		 * \brief Queries for a Component object
		 * \param uid Uid of the Component object.
		 * \return Const pointer to the stored Component object or nullptr if not valid.
		 */
		[[nodiscard]] constexpr const TComponent* findComponent(Uid uid) const noexcept
		{
			if (0u < uid && uid <= std::size(m_Components))
			{
				if (auto& info = m_Components[uid - 1u])
					return &info->component;
			}
			return nullptr;
		}

		/**
		 * \brief Queries for a Component object
		 * \param uid Uid of the Component object.
		 * \return pointer to the stored Component object or nullptr if not valid.
		 */
		[[nodiscard]] constexpr TComponent* findComponent(Uid uid) noexcept
		{
			return const_cast<TComponent*>(std::as_const(*this).findComponent(uid));
		}

		/**
		 * \brief Queries for a Component object
		 * \param uid Uid of the Component object.
		 * \throws SystemError if the Component object at uid is not valid.
		 * \return Const reference to the stored Component object.
		 */
		[[nodiscard]] constexpr const TComponent& component(Uid uid) const
		{
			if (hasComponent(uid))
			{
				return m_Components[uid - 1u]->component;
			}
			using namespace std::string_literals;
			throw SystemError("System: \""s + typeid(*this).name() + "\" Component uid: " + std::to_string(uid) + " not found.");
		}

		/**
		 * \brief Queries for a Component object
		 * \param uid Uid of the Component object.
		 * \throws SystemError if the Component object at uid is not valid.
		 * \return Reference to the stored Component object.
		 */
		[[nodiscard]] constexpr TComponent& component(Uid uid)
		{
			return const_cast<TComponent&>(std::as_const(*this).component(uid));
		}

		/**
		 * \brief Counts active Components
		 * \return Amount of active Component objects.
		 */
		[[nodiscard]] constexpr std::size_t size() const noexcept
		{
			return m_ComponentCount;
		}

		/**
		 * \brief Empty
		 * \return True if no active Component objects present.
		 */
		[[nodiscard]] constexpr bool empty() const noexcept
		{
			return m_ComponentCount == 0;
		}

		/**
		 * \brief preUpdate
		 *
		 * This function may be overriden to conduct necessary preparations for the next update call.
		 */
		void preUpdate() override
		{
		}

		/**
		 * \brief update
		 *
		 * This function may be overriden to perform frequent actions on components.
		 * \param delta Time elapsed until previous update call.
		 */
		void update(float delta) override
		{
		}

		/**
		 * \brief postUpdate
		 *
		 * This function may be overriden to conduct necessary finalization steps for the latest update call.
		 */
		void postUpdate() override
		{
		}

	protected:
		/**
		 * \brief Protected default Constructor
		 */
		SystemBase() = default;

		/**
		 * \brief Entity state changed
		 *
		 * This function will be called when an Component associated Entity changed its state. It may be overridden.
		 */
		virtual void derivedEntityStateChanged(TComponent& component, Entity& entity)
		{
		}

		/**
		 * \brief Executes action on each active Component
		 * \tparam TComponentAction Invokable object with specific signature.
		 * \param action Invokable object.
		 */
		template <std::invocable<Entity&, TComponent&> TComponentAction>
		void forEachComponent(TComponentAction action)
		{
			for (auto& info : m_Components)
			{
				if (info)
				{
					assert(info->entity);
					action(*info->entity, info->component);
				}
			}
		}

	private:
		std::size_t m_ComponentCount = 0;
		std::deque<std::optional<ComponentInfo>> m_Components;

		template <class TComponentCreator = utils::EmptyCallable<TComponent>>
		[[nodiscard]] Uid createComponent(TComponentCreator&& creator = TComponentCreator{})
		{
			if (auto itr = std::find(std::begin(m_Components), std::end(m_Components), std::nullopt);
				itr != std::end(m_Components))
			{
				itr->emplace(ComponentInfo{ nullptr, creator() });
				++m_ComponentCount;
				return { static_cast<Uid>(std::distance(std::begin(m_Components), itr)) + 1u };
			}
			m_Components.emplace_back(ComponentInfo{ nullptr, creator() });
			++m_ComponentCount;
			return static_cast<Uid>(std::size(m_Components));
		}

		void setComponentEntity(Uid uid, Entity& entity) noexcept
		{
			assert(hasComponent(uid));
			m_Components[uid - 1u]->entity = &entity;
		}

		constexpr void destroyComponent(Uid uid) noexcept
		{
			if (uid <= std::size(m_Components))
			{
				m_Components[uid - 1].reset();
				--m_ComponentCount;
			}
		}

		constexpr void entityStateChanged(Uid uid)
		{
			assert(0u < uid && uid <= std::size(m_Components));
			auto& info = m_Components[uid - 1u];
			assert(info && info->entity);
			derivedEntityStateChanged(info->component, *info->entity);
		}
	};
}

#endif
