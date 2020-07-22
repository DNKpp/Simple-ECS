
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
#include <memory>

#include "AbstractComponentHandle.hpp"
#include "EmptyCallable.hpp"

namespace secs
{
namespace details
{
	template <class TComponent>
	class ComponentHolder
	{
	public:
		using ComponentType = TComponent;

		class ComponentHandle :
			public AbstractComponentHandle
		{
		public:
			constexpr ComponentHandle(UID uid, ComponentHolder& holder, TComponent& component) :
				m_Holder(&holder),
				AbstractComponentHandle(uid, component)
			{
				assert(m_Holder);
			}

			ComponentHandle(const ComponentHandle&) = delete;
			ComponentHandle& operator =(const ComponentHandle&) = delete;

			constexpr ComponentHandle(ComponentHandle&&) noexcept = default;
			constexpr ComponentHandle& operator =(ComponentHandle&&) noexcept = default;

			~ComponentHandle() noexcept override
			{
				assert(m_Holder);
				m_Holder->deleteComponent(getUID());
				m_Holder = nullptr;
			}

			constexpr std::type_index getTypeInfo() const noexcept override
			{
				return typeid(TComponent);
			}

		private:
			ComponentHolder* m_Holder;
		};

		ComponentHolder(const ComponentHolder&) = delete;
		ComponentHolder& operator =(const ComponentHolder&) = delete;

		template <class TComponentCreator = utils::EmptyCallable<TComponent>>
		ComponentHandle createComponent(TComponentCreator&& creator = TComponentCreator{})
		{
			if (auto itr = std::find(std::begin(m_Components), std::end(m_Components), std::nullopt);
				itr != std::end(m_Components))
			{
				itr->emplace(creator());
				return { static_cast<UID>(std::distance(std::begin(m_Components), itr) + 1u), *this, *itr };
			}
			m_Components.emplace_back(creator());
			return { std::size(m_Components), *this, m_Components.back() };
		}

		TComponent* getComponent(UID uid)
		{
			if (uid < std::size(m_Components))
			{
				auto& component = m_Components[uid];
				if (component)
					return *component;
			}
			return nullptr;
		}

	protected:
		constexpr ComponentHolder() = default;
		~ComponentHolder() noexcept = default;

		constexpr ComponentHolder(ComponentHolder&& other) = default;
		constexpr ComponentHolder& operator =(ComponentHolder&&) = default;

		template <class TComponentUpdater = utils::EmptyCallable<>>
		void foreachComponent(TComponentUpdater&& updater = TComponentUpdater())
		{
			for (auto& component : m_Components)
			{
				if (component)
					updater(*component);
			}
		}

	private:
		std::deque<std::optional<TComponent>> m_Components;

		constexpr void deleteComponent(UID uid) noexcept
		{
			if (uid < std::size(m_Components))
				m_Components[uid].reset();
		}
	};
}

	class ISystem
	{
	public:
		virtual ~ISystem() noexcept = default;

		virtual void preUpdateComponents() = 0;
		virtual void updateComponents() = 0;
		virtual void postUpdateComponents() = 0;

	protected:
		ISystem() noexcept = default;
	};

	template <class TComponent>
	class SystemBase :
		public ISystem,
		public details::ComponentHolder<TComponent>
	{
	public:
		void preUpdateComponents() noexcept override {}
		void updateComponents() noexcept override {}
		void postUpdateComponents() noexcept override {}

	protected:
		constexpr SystemBase() = default;
	};
}

#endif