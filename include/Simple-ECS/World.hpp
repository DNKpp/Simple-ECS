
//          Copyright Dominic Koepke 2020 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#ifndef SECS_WORLD_HPP
#define SECS_WORLD_HPP

#pragma once

#include <vector>
#include <memory>

#include "System.hpp"

namespace secs
{
	class World
	{
	public:
		template <class TSystem>
		constexpr void registerSystem(TSystem&& system)
		{
			if (auto itr = findSystemStorage<SystemComponentType<TSystem>>(*this);
				itr != std::end(m_Systems))
			{
				itr->system = std::make_unique<std::remove_cvref_t<TSystem>>(std::forward<TSystem>(system));
			}
			else
				m_Systems.emplace_back(typeid(SystemComponentType<TSystem>), std::forward<TSystem>(system));
		}

		template <class TComponent>
		constexpr const SystemBase<TComponent>* getSystem() const noexcept
		{
			auto itr = findSystemStorage<TComponent>(*this);
			return itr != std::end(m_Systems) ? static_cast<SystemBase<TComponent>*>(itr->system.get()) : nullptr;
		}

		template <class TComponent>
		constexpr SystemBase<TComponent>* getSystem() noexcept
		{
			return const_cast<SystemBase<TComponent>*>(std::as_const(*this).getSystem<TComponent>());
		}

	private:
		struct SystemStorage
		{
			std::type_index type;
			std::unique_ptr<ISystem> system;

			template <class TSystem>
			constexpr SystemStorage(std::type_index _type, TSystem&& system) :
				type{ std::move(_type) },
				system{ std::make_unique<std::remove_cvref_t<TSystem>>(std::forward<TSystem>(system)) }
			{
			}
		};
		std::vector<SystemStorage> m_Systems;

		template <class TSystem>
		using SystemComponentType = typename std::decay_t<TSystem>::ComponentType;

		template <class TComponent, class TWorld>
		static constexpr decltype(auto) findSystemStorage(TWorld&& world) noexcept
		{
			std::type_index typeIndex = typeid(std::decay_t<TComponent>);
			return std::find_if(std::begin(world.m_Systems), std::end(world.m_Systems), [typeIndex](const auto& info) { return info.type == typeIndex; });
		}
	};
}

#endif
