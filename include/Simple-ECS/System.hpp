
//          Copyright Dominic Koepke 2020 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#ifndef SECS_SYSTEM_HPP
#define SECS_SYSTEM_HPP

#pragma once

#include <queue>
#include <optional>
#include <cassert>
#include <stdexcept>

#include "ComponentHandle.hpp"
#include "Defines.hpp"
#include "EmptyCallable.hpp"

namespace secs
{
	class SystemError :
		public std::runtime_error
	{
	public:
		SystemError(const std::string& msg) :
			std::runtime_error(msg)
		{}

		SystemError(const char* msg) :
			std::runtime_error(msg)
		{}
	};

	class ISystem
	{
	public:
		virtual ~ISystem() noexcept = default;

		virtual void preUpdate() = 0;
		virtual void update() = 0;
		virtual void postUpdate() = 0;

	protected:
		ISystem() noexcept = default;
	};

	template <class TComponent>
	class SystemBase :
		public ISystem
	{
	private:
		template <class TSystem>
		friend class ComponentHandle;

		struct ComponentInfo
		{
			UID entityUID;
			TComponent component;
		};

	public:
		using ComponentType = TComponent;
		using ComponentHandle = ComponentHandle<SystemBase<TComponent>>;

		SystemBase(const SystemBase&) = delete;
		SystemBase& operator =(const SystemBase&) = delete;

		constexpr SystemBase(SystemBase&&) noexcept = default;
		constexpr SystemBase& operator =(SystemBase&&) noexcept = default;

		template <class TComponentCreator = utils::EmptyCallable<TComponent>>
		ComponentHandle createComponent(UID entityUID, TComponentCreator&& creator = TComponentCreator{})
		{
			if (auto itr = std::find(std::begin(m_Components), std::end(m_Components), std::nullopt);
				itr != std::end(m_Components))
			{
				itr->emplace(ComponentInfo{ entityUID, creator() });
				return { static_cast<UID>(std::distance(std::begin(m_Components), itr) + 1u), *this };
			}
			m_Components.emplace_back(ComponentInfo{ entityUID, creator() });
			return { std::size(m_Components), *this };
		}

		constexpr bool hasComponent(UID uid) const noexcept
		{
			return 0u < uid && uid <= std::size(m_Components) && m_Components[uid - 1u];
		}

		constexpr const ComponentInfo& operator [](UID uid) const noexcept
		{
			assert(hasComponent(uid));
			return m_Components[uid - 1u];
		}

		constexpr const TComponent* getComponentPtr(UID uid) const noexcept
		{
			if (0u < uid && uid <= std::size(m_Components))
			{
				if (auto& info = m_Components[uid - 1u])
					return &info->component;
			}
			return nullptr;
		}

		constexpr TComponent* getComponentPtr(UID uid) noexcept
		{
			return const_cast<TComponent*>(std::as_const(*this).getComponentPtr(uid));
		}

		constexpr const TComponent& getComponent(UID uid) const noexcept
		{
			assert(hasComponent(uid));
			return m_Components[uid - 1u]->component;
		}

		constexpr TComponent& getComponent(UID uid) noexcept
		{
			return const_cast<TComponent&>(std::as_const(*this).getComponent(uid));
		}

		constexpr std::size_t componentCount() const noexcept
		{
			return std::size(m_Components) - std::count(std::begin(m_Components), std::end(m_Components), std::nullopt);
		}

		constexpr std::size_t size() const noexcept
		{
			return std::size(m_Components);
		}

		constexpr bool empty() const noexcept
		{
			return std::empty(m_Components);
		}

		constexpr void onEntityStateChanged(UID componentUID, EntityState state)
		{
			if (auto info = getComponentPtr(componentUID))
				onEntityStateChangedImpl(info->component, state);
		}

		template <class TComponentAction = utils::EmptyCallable<>>
		void foreachComponent(TComponentAction&& action = TComponentAction())
		{
			for (auto& info : m_Components)
			{
				if (info)
					action(info->entityUID, info->component);
			}
		}

		void preUpdate() noexcept override {}
		void update() noexcept override {}
		void postUpdate() noexcept override {}

	protected:
		constexpr SystemBase() = default;

		virtual void onEntityStateChangedImpl(TComponent& component, EntityState state) noexcept
		{
		}

	private:
		std::deque<std::optional<ComponentInfo>> m_Components;

		constexpr void deleteComponent(UID uid) noexcept
		{
			if (uid <= std::size(m_Components))
				m_Components[uid - 1].reset();
		}
	};
}

#endif