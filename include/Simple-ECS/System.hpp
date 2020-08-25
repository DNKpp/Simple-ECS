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

#include "ComponentHandle.hpp"
#include "Defines.hpp"
#include "EmptyCallable.hpp"

namespace secs
{
	class Entity;

	class SystemError :
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

		virtual void preUpdate() noexcept = 0;
		virtual void update(float delta) noexcept = 0;
		virtual void postUpdate() noexcept = 0;

	protected:
		constexpr ISystem() noexcept = default;
	};

	class Entity;
	class World;

	template <class TComponent>
	class SystemBase :
		public ISystem
	{
	private:
		template <class TSystem>
		friend class ComponentHandle;

		struct ComponentInfo
		{
			Entity* entity;
			TComponent component;
		};

	public:
		using ComponentType = TComponent;
		using ComponentHandle = ComponentHandle<SystemBase<TComponent>>;

		SystemBase(const SystemBase&) = delete;
		SystemBase& operator =(const SystemBase&) = delete;

		SystemBase(SystemBase&&) /* NOT noexcept*/ = default;
		SystemBase& operator =(SystemBase&&) /* NOT noexcept*/ = default;

		~SystemBase() noexcept override = default;

		template <class TComponentCreator = utils::EmptyCallable<TComponent>>
		[[nodiscard]] ComponentHandle createComponent(TComponentCreator&& creator = TComponentCreator{})
		{
			if (auto itr = std::find(std::begin(m_Components), std::end(m_Components), std::nullopt);
				itr != std::end(m_Components))
			{
				itr->emplace(ComponentInfo{ nullptr, creator() });
				return { static_cast<UID>(std::distance(std::begin(m_Components), itr) + 1u), *this };
			}
			m_Components.emplace_back(ComponentInfo{ nullptr, creator() });
			return { std::size(m_Components), *this };
		}

		[[nodiscard]] constexpr bool hasComponent(UID uid) const noexcept
		{
			return 0u < uid && uid <= std::size(m_Components) && m_Components[uid - 1u];
		}

		[[nodiscard]] constexpr const ComponentInfo& operator [](UID uid) const noexcept
		{
			assert(hasComponent(uid));
			return m_Components[uid - 1u];
		}

		[[nodiscard]] constexpr const TComponent* getComponentPtr(UID uid) const noexcept
		{
			if (0u < uid && uid <= std::size(m_Components))
			{
				if (auto& info = m_Components[uid - 1u])
					return &info->component;
			}
			return nullptr;
		}

		[[nodiscard]] constexpr TComponent* getComponentPtr(UID uid) noexcept
		{
			return const_cast<TComponent*>(std::as_const(*this).getComponentPtr(uid));
		}

		[[nodiscard]] constexpr const TComponent& getComponent(UID uid) const noexcept
		{
			assert(hasComponent(uid));
			return m_Components[uid - 1u]->component;
		}

		[[nodiscard]] constexpr TComponent& getComponent(UID uid) noexcept
		{
			return const_cast<TComponent&>(std::as_const(*this).getComponent(uid));
		}

		[[nodiscard]] constexpr std::size_t componentCount() const noexcept
		{
			return std::size(m_Components) - std::count(std::begin(m_Components), std::end(m_Components), std::nullopt);
		}

		[[nodiscard]] constexpr std::size_t size() const noexcept
		{
			return std::size(m_Components);
		}

		[[nodiscard]] constexpr bool empty() const noexcept
		{
			return std::empty(m_Components);
		}

		constexpr void onEntityStateChanged(UID componentUID, Entity& entity) noexcept
		{
			if (auto component = getComponentPtr(componentUID))
			{
				assert(component);
				onEntityStateChangedImpl(*component, entity);
			}
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

		void preUpdate() noexcept override
		{
		}

		void update(float delta) noexcept override
		{
		}

		void postUpdate() noexcept override
		{
		}

	protected:
		SystemBase() = default;

		virtual void onEntityStateChangedImpl(TComponent& component, Entity& entity) noexcept
		{
		}

	private:
		std::deque<std::optional<ComponentInfo>> m_Components;

		void setComponentEntity(UID componentUID, Entity& entity) noexcept
		{
			assert(hasComponent(componentUID));
			m_Components[componentUID - 1u]->entity = &entity;
		}

		constexpr void deleteComponent(UID uid) noexcept
		{
			if (uid <= std::size(m_Components))
				m_Components[uid - 1].reset();
		}
	};
}

#endif
