
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
#include <type_traits>

#include "System.hpp"
#include "Entity.hpp"
#include "ComponentStorage.hpp"

namespace secs
{
	class World
	{
	public:
		template <class TSystem>
		constexpr void registerSystem(TSystem&& system)
		{
			using TComponentType = typename TSystem::ComponentType;
			if (auto itr = findSystemStorage<TComponentType>(*this);
				itr != std::end(m_Systems))
			{
				itr->system = std::make_unique<std::remove_cvref_t<TSystem>>(std::move(system));
			}
			else
				m_Systems.emplace_back(typeid(TComponentType), std::move(system));
		}

		template <class TComponent>
		const SystemBase<TComponent>* getSystemPtr() const noexcept
		{
			auto itr = findSystemStorage<TComponent>(*this);
			return itr != std::end(m_Systems) ? static_cast<SystemBase<TComponent>*>(itr->system.get()) : nullptr;
		}

		template <class TComponent>
		SystemBase<TComponent>* getSystemPtr() noexcept
		{
			return const_cast<SystemBase<TComponent>*>(std::as_const(*this).getSystem<TComponent>());
		}

		template <class TComponent>
		const SystemBase<TComponent>& getSystem() const
		{
			if (auto ptr = getSystemPtr<TComponent>())
				return *ptr;
			using namespace std::string_literals;
			throw SystemError("System not found: "s + typeid(SystemBase<TComponent>).name());
		}

		template <class TComponent>
		SystemBase<TComponent>& getSystem()
		{
			return const_cast<SystemBase<TComponent>&>(std::as_const(*this).getSystem<TComponent>());
		}

		template <class... TComponent>
		Entity& createEntity()
		{
			std::scoped_lock entityLock{ m_NewEntityMx };

			auto entityUID = m_NextUID++;
			auto componentStorage = std::make_unique<ComponentStorage<TComponent...>>((getSystem<TComponent>().createComponent(entityUID), ...));

			m_NewEntities.emplace_back(std::make_unique<Entity>(entityUID, std::move(componentStorage)));
			m_NewEntities.back()->changeState(EntityState::initializing);
			return *m_NewEntities.back();
		}

		void destroyEntityLater(UID uid) noexcept
		{
			std::scoped_lock lock{ m_DestructableEntityMx };
			m_DestructableEntityUIDs.emplace_back(uid);
		}

		const Entity* findEntityPtr(UID uid) const noexcept
		{
			std::shared_lock entityLock{ m_EntityMx };
			auto itr = findEntityItr(m_Entities, uid);
			return itr != std::end(m_Entities) ? &**itr : nullptr;
		}

		Entity* findEntityPtr(UID uid) noexcept
		{
			return const_cast<Entity*>(std::as_const(*this).findEntityPtr(uid));
		}

		const Entity& findEntity(UID uid) const
		{
			if (auto ptr = findEntityPtr(uid))
				return *ptr;
			using namespace std::string_literals;
			throw EntityError("Entity uid: "s + std::to_string(uid) + " not found: ");
		}

		Entity& findEntity(UID uid)
		{
			return const_cast<Entity&>(std::as_const(*this).findEntity(uid));
		}

		void preUpdate() noexcept
		{
			for (auto& storage : m_Systems)
				storage.system->preUpdate();
		}

		void update() noexcept
		{
			for (auto& storage : m_Systems)
				storage.system->update();
		}

		void postUpdate() noexcept
		{
			for (auto& storage : m_Systems)
				storage.system->postUpdate();

			mergeNewEntities();
			execEntityDestruction();
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

		template <class TComponent, class TWorld>
		constexpr static decltype(auto) findSystemStorage(TWorld&& world) noexcept
		{
			std::type_index typeIndex = typeid(std::decay_t<TComponent>);
			return std::find_if(std::begin(world.m_Systems), std::end(world.m_Systems),
				[typeIndex](const auto& info) { return info.type == typeIndex; }
			);
		}

		template <class TContainer>
		constexpr static auto findEntityItr(TContainer& container, UID uid) noexcept -> decltype(std::begin(container))
		{
			if (auto itr = std::lower_bound(std::begin(container), std::end(container), uid, LessEntityByUID{});
				itr != std::end(container) && (*itr)->getUID() == uid)
			{
				return itr;
			}
			return std::end(container);
		}

		auto takeDestructableEntityUIDs() noexcept
		{
			std::scoped_lock lock{ m_DestructableEntityMx };
			auto tmp = std::move(m_DestructableEntityUIDs);
			return tmp;
		}

		auto takeNewEntities() noexcept
		{
			std::scoped_lock lock{ m_NewEntityMx };
			auto tmp = std::move(m_NewEntities);
			return tmp;
		}

		void mergeNewEntities()
		{
			auto newEntities = takeNewEntities();
			if (std::empty(newEntities))
				return;

			for (auto& entity : newEntities)
				entity->changeState(EntityState::running);

			std::scoped_lock lock{ m_EntityMx };
			m_Entities.insert(std::end(m_Entities), std::make_move_iterator(std::begin(newEntities)), std::make_move_iterator(std::end(newEntities)));
		}

		void execEntityDestruction()
		{
			auto destructableEntityUIDs = takeDestructableEntityUIDs();
			if (std::empty(destructableEntityUIDs))
				return;

			std::sort(std::begin(destructableEntityUIDs), std::end(destructableEntityUIDs));
			destructableEntityUIDs.erase(std::unique(std::begin(destructableEntityUIDs), std::end(destructableEntityUIDs)), std::end(destructableEntityUIDs));

			std::scoped_lock entityLock{ m_EntityMx };
			auto oldEntities = std::move(m_Entities);

			std::set_difference(std::make_move_iterator(std::begin(oldEntities)), std::make_move_iterator(std::end(oldEntities)),
				std::begin(destructableEntityUIDs), std::end(destructableEntityUIDs),
				std::back_inserter(m_Entities), LessEntityByUID{});

			for (auto& entity : oldEntities)
			{
				if (entity)
					entity->changeState(EntityState::tearDown);
			}
		}

		UID m_NextUID = 1;
		mutable std::mutex m_NewEntityMx;
		std::vector<std::unique_ptr<Entity>> m_NewEntities;	// ptr is simply used to get a consistent memory space for each entity

		mutable std::shared_mutex m_EntityMx;
		std::vector<std::unique_ptr<Entity>> m_Entities;	// ptr is simply used to get a consistent memory space for each entity

		mutable std::mutex m_DestructableEntityMx;
		std::vector<UID> m_DestructableEntityUIDs;
	};
}

#endif
