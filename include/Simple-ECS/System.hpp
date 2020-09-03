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
	template <class TComponent>
	class SystemBase :
		public ISystem
	{
	private:
		friend struct detail::ComponentRtti;

		struct ComponentInfo
		{
			Entity* entity;
			TComponent component;
		};

	public:
		using ComponentType = TComponent;

		SystemBase(const SystemBase&) = delete;
		SystemBase& operator =(const SystemBase&) = delete;

		SystemBase(SystemBase&&) /* NOT noexcept*/ = default;
		SystemBase& operator =(SystemBase&&) /* NOT noexcept*/ = default;

		~SystemBase() noexcept override = default;

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

		[[nodiscard]] constexpr bool hasComponent(Uid uid) const noexcept
		{
			return 0u < uid && uid <= std::size(m_Components) && m_Components[uid - 1u];
		}

		[[nodiscard]] constexpr const TComponent* findComponent(Uid uid) const noexcept
		{
			if (0u < uid && uid <= std::size(m_Components))
			{
				if (auto& info = m_Components[uid - 1u])
					return &info->component;
			}
			return nullptr;
		}

		[[nodiscard]] constexpr TComponent* findComponent(Uid uid) noexcept
		{
			return const_cast<TComponent*>(std::as_const(*this).findComponent(uid));
		}

		[[nodiscard]] constexpr const TComponent& component(Uid uid) const
		{
			if (hasComponent(uid))
			{
				return m_Components[uid - 1u]->component;
			}
			using namespace std::string_literals;
			throw SystemError("System: \""s + typeid(*this).name() + "\" Component uid: " + std::to_string(uid) + " not found.");
		}

		[[nodiscard]] constexpr TComponent& component(Uid uid)
		{
			return const_cast<TComponent&>(std::as_const(*this).component(uid));
		}

		[[nodiscard]] constexpr std::size_t size() const noexcept
		{
			return m_ComponentCount;
		}

		[[nodiscard]] constexpr bool empty() const noexcept
		{
			return m_ComponentCount == 0;
		}

		template <class TComponentAction = utils::EmptyCallable<>>
		void forEachComponent(TComponentAction&& action = TComponentAction())
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

		void preUpdate() override
		{
		}

		void update(float delta) override
		{
		}

		void postUpdate() override
		{
		}

	protected:
		SystemBase() = default;

		virtual void derivedEntityStateChanged(TComponent& component, Entity& entity)
		{
		}

	private:
		std::size_t m_ComponentCount = 0;
		std::deque<std::optional<ComponentInfo>> m_Components;

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
