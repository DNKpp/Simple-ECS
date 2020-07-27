
//          Copyright Dominic Koepke 2020 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#ifndef SECS_WORLD_HPP
#define SECS_WORLD_HPP

#pragma once

#include <vector>
#include <memory>
#include <string>
#include <typeinfo>
#include <mutex>
#include <shared_mutex>

#include "System.hpp"
#include "Entity.hpp"

namespace secs
{
	class World
	{
	public:
		template <class TSystem>
		constexpr void registerSystem(TSystem&& system)
		{
			std::scoped_lock systemLock{ m_SystemMx };

			if (auto itr = findSystemStorage<SystemComponentType<TSystem>>(*this);
				itr != std::end(m_Systems))
			{
				itr->system = std::make_unique<std::remove_cvref_t<TSystem>>(std::forward<TSystem>(system));
			}
			else
				m_Systems.emplace_back(typeid(SystemComponentType<TSystem>), std::forward<TSystem>(system));
		}

		template <class TComponent>
		constexpr const SystemBase<TComponent>* getSystemPtr() const noexcept
		{
			std::shared_lock systemLock{ m_SystemMx };

			auto itr = findSystemStorage<TComponent>(*this);
			return itr != std::end(m_Systems) ? static_cast<SystemBase<TComponent>*>(itr->system.get()) : nullptr;
		}

		template <class TComponent>
		constexpr SystemBase<TComponent>* getSystemPtr() noexcept
		{
			return const_cast<SystemBase<TComponent>*>(std::as_const(*this).getSystem<TComponent>());
		}

		template <class TComponent>
		constexpr const SystemBase<TComponent>& getSystem() const
		{
			if (auto ptr = getSystemPtr<TComponent>())
				return *ptr;
			using namespace std::string_literals;
			throw SystemError("System not found: "s + typeid(SystemBase<TComponent>).name());
		}

		template <class TComponent>
		constexpr SystemBase<TComponent>& getSystem()
		{
			return const_cast<SystemBase<TComponent>&>(std::as_const(*this).getSystem<TComponent>());
		}

		template <class... TComponent>
		constexpr Entity& createEntity() noexcept
		{
			std::scoped_lock entityLock{ m_EntityMx };

			auto entityUID = m_NextUID++;
			m_Entities.emplace_back(std::make_unique<Entity>(entityUID, (getSystem<TComponent>().createComponent(entityUID), ...)));
			return *m_Entities.back();
		}

		constexpr void destroyEntityLater(UID uid) noexcept
		{
			std::scoped_lock lock{ m_DestructableEntityMx };
			m_DestructableEntityUIDs.emplace_back(uid);
		}

		constexpr const Entity* findEntityPtr(UID uid) const noexcept
		{
			std::shared_lock entityLock{ m_EntityMx };
			auto itr = findEntityItr(*this, uid);
			return itr != std::end(m_Entities) ? &**itr : nullptr;
		}

		constexpr Entity* findEntityPtr(UID uid) noexcept
		{
			return const_cast<Entity*>(std::as_const(*this).findEntityPtr(uid));
		}

		constexpr const Entity& findEntity(UID uid) const
		{
			if (auto ptr = findEntityPtr(uid))
				return *ptr;
			using namespace std::string_literals;
			throw EntityError("Entity uid: "s + std::to_string(uid) + " not found: ");
		}

		constexpr Entity& findEntity(UID uid)
		{
			return const_cast<Entity&>(std::as_const(*this).findEntity(uid));
		}

		constexpr void execEntityDestruction()
		{
			auto destructableEntityUIDs = takeDestructableEntityUIDs();
			if (std::empty(destructableEntityUIDs))
				return;

			std::sort(std::begin(destructableEntityUIDs), std::end(destructableEntityUIDs));
			destructableEntityUIDs.erase(std::unique(std::begin(destructableEntityUIDs), std::end(destructableEntityUIDs)), std::end(destructableEntityUIDs));

			std::scoped_lock entityLock{ m_EntityMx };
			auto oldEntites = std::move(m_Entities);

			std::set_difference(std::make_move_iterator(std::begin(oldEntites)), std::make_move_iterator(std::end(oldEntites)),
				std::begin(destructableEntityUIDs), std::end(destructableEntityUIDs),
				std::back_inserter(m_Entities), LessEntityByUID{});
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
		mutable std::shared_mutex m_SystemMx;
		std::vector<SystemStorage> m_Systems;

		template <class TSystem>
		using SystemComponentType = typename std::decay_t<TSystem>::ComponentType;

		template <class TComponent, class TWorld>
		constexpr static decltype(auto) findSystemStorage(TWorld&& world) noexcept
		{
			std::type_index typeIndex = typeid(std::decay_t<TComponent>);
			return std::find_if(std::begin(world.m_Systems), std::end(world.m_Systems),
				[typeIndex](const auto& info) { return info.type == typeIndex; }
			);
		}

		template <class TWorld>
		constexpr static auto findEntityItr(TWorld&& world, UID uid) noexcept
		{
			if (auto itr = std::lower_bound(std::begin(world.m_Entities), std::end(world.m_Entities), uid, LessEntityByUID{});
				itr != std::end(world.m_Entities) && (*itr)->getUID() == uid)
			{
				return itr;
			}
			return std::end(world.m_Entities);
		}

		constexpr std::vector<UID> takeDestructableEntityUIDs() noexcept
		{
			std::scoped_lock lock{ m_DestructableEntityMx };
			auto tmp = std::move(m_DestructableEntityUIDs);
			return tmp;
		}

		UID m_NextUID = 1;
		mutable std::shared_mutex m_EntityMx;
		std::vector<std::unique_ptr<Entity>> m_Entities;	// ptr is simply used to get a consistent memory space for each entity

		mutable std::mutex m_DestructableEntityMx;
		std::vector<UID> m_DestructableEntityUIDs;
	};
}

#endif
