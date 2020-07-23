
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
#include <stdexcept>

#include "AbstractComponentHandle.hpp"
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
			using Super = AbstractComponentHandle;

		public:
			constexpr ComponentHandle(UID uid, ComponentHolder& holder, TComponent& component) :
				m_Holder(&holder),
				Super(uid, component)
			{
				assert(m_Holder);
			}

			ComponentHandle(const ComponentHandle&) = delete;
			ComponentHandle& operator =(const ComponentHandle&) = delete;

			// ToDo: in cpp20
			/*constexpr*/ ComponentHandle(ComponentHandle&& other) noexcept
			{
				*this = std::move(other);
			}
			// ToDo: in cpp20
			/*constexpr*/ ComponentHandle& operator =(ComponentHandle&& other) noexcept
			{
				if (this != &other)
				{
					this->Super::operator =(std::move(other));
					m_Holder = other.m_Holder;
					other.reset();
				}
				return *this;
			}

			~ComponentHandle() noexcept override
			{
				release();
			}

			constexpr const ComponentHolder* getComponentHolder() const noexcept
			{
				return m_Holder;
			}

			constexpr std::type_index getTypeInfo() const noexcept override
			{
				return typeid(TComponent);
			}

			// ToDo: in cpp20
			/*constexpr*/ void release() noexcept override
			{
				if (getUID() != 0)
				{
					assert(m_Holder);
					m_Holder->deleteComponent(getUID());
					reset();
				}
			}

		private:
			ComponentHolder* m_Holder;

			constexpr void reset() noexcept
			{
				Super::reset();
				m_Holder = nullptr;
			}
		};

		ComponentHolder(const ComponentHolder&) = delete;
		ComponentHolder& operator =(const ComponentHolder&) = delete;

		template <class TComponentCreator = utils::EmptyCallable<TComponent>>
		ComponentHandle createComponent(UID entityUID, TComponentCreator&& creator = TComponentCreator{})
		{
			if (auto itr = std::find(std::begin(m_Components), std::end(m_Components), std::nullopt);
				itr != std::end(m_Components))
			{
				itr->emplace(ComponentInfo{ entityUID, creator() });
				return { static_cast<UID>(std::distance(std::begin(m_Components), itr) + 1u), *this, (*itr)->component };
			}
			m_Components.emplace_back(ComponentInfo{ entityUID, creator() });
			return { std::size(m_Components), *this, m_Components.back()->component };
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
		struct ComponentInfo
		{
			UID entityUID;
			TComponent component;
		};

		std::deque<std::optional<ComponentInfo>> m_Components;

		constexpr void deleteComponent(UID uid) noexcept
		{
			if (uid <= std::size(m_Components))
				m_Components[uid - 1].reset();
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