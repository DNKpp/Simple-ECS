//          Copyright Dominic Koepke 2020 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#ifndef SECS_WORLD_HPP
#define SECS_WORLD_HPP

#pragma once

#include <iterator>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <typeinfo>
#include <type_traits>
#include <vector>

#include "ComponentStorage.hpp"
#include "Concepts.hpp"
#include "Entity.hpp"
#include "System.hpp"

namespace secs
{
	class World
	{
	public:
		template <System TSystem>
		constexpr TSystem& registerSystem(TSystem&& system)
		{
			if (auto itr = findSystemStorage<TSystem>(*this);
				itr != std::end(m_Systems))
			{
				itr->system = std::make_unique<std::remove_cvref_t<TSystem>>(std::forward<TSystem>(system));
				return **itr;
			}
			return *m_Systems.emplace_back(typeid(TSystem), std::forward<TSystem>(system));
		}

		template <System TSystem>
		[[nodiscard]] const TSystem* getSystemPtr() const noexcept
		{
			auto itr = findSystemStorage<TSystem>(*this);
			return itr != std::end(m_Systems) ? static_cast<TSystem*>(itr->system.get()) : nullptr;
		}

		template <System TSystem>
		[[nodiscard]] TSystem* getSystemPtr() noexcept
		{
			return const_cast<TSystem*>(std::as_const(*this).getSystem<TSystem>());
		}

		template <System TSystem>
		[[nodiscard]] const TSystem& getSystem() const
		{
			if (auto ptr = getSystemPtr<TSystem>())
				return *ptr;
			using namespace std::string_literals;
			throw SystemError("System not found: "s + typeid(TSystem).name());
		}

		template <System TSystem>
		[[nodiscard]] TSystem& getSystem()
		{
			return const_cast<TSystem&>(std::as_const(*this).getSystem<TSystem>());
		}

		template <Component... TComponent>
		Entity& createEntity()
		{
			std::scoped_lock entityLock{ m_NewEntityMx };

			auto entityUID = m_NextUID++;
			using ComponentStorage = ComponentStorage<TComponent...>;
			auto componentStorage = std::make_unique<ComponentStorage>(getSystem<TComponent>().createComponent()...);

			m_NewEntities.emplace_back(std::make_unique<Entity>(entityUID, std::move(componentStorage)));
			return *m_NewEntities.back();
		}

		void destroyEntityLater(UID uid)
		{
			std::scoped_lock lock{ m_DestructibleEntityMx };
			m_DestructibleEntityUIDs.emplace_back(uid);
		}

		[[nodiscard]] const Entity* findEntityPtr(UID uid) const noexcept
		{
			std::shared_lock entityLock{ m_EntityMx };
			const auto itr = findEntityItr(m_Entities, uid);
			return itr != std::end(m_Entities) ? &**itr : nullptr;
		}

		[[nodiscard]] Entity* findEntityPtr(UID uid) noexcept
		{
			return const_cast<Entity*>(std::as_const(*this).findEntityPtr(uid));
		}

		[[nodiscard]] const Entity& findEntity(UID uid) const
		{
			if (const auto ptr = findEntityPtr(uid))
				return *ptr;
			using namespace std::string_literals;
			throw EntityError("Entity uid: "s + std::to_string(uid) + " not found: ");
		}

		[[nodiscard]] Entity& findEntity(UID uid)
		{
			return const_cast<Entity&>(std::as_const(*this).findEntity(uid));
		}

		void preUpdate()
		{
			for (auto& storage : m_Systems)
				storage.system->preUpdate();
		}

		void update(float delta)
		{
			for (auto& storage : m_Systems)
				storage.system->update(delta);
		}

		void postUpdate()
		{
			for (auto& storage : m_Systems)
				storage.system->postUpdate();

			processInitializingEntities();
			processNewEntities();
			processEntityDestruction();
		}

	private:
		struct SystemStorage
		{
			std::type_index type;
			std::unique_ptr<ISystem> system;

			template <class TSystem>
			constexpr SystemStorage(std::type_index _type, TSystem&& system) :
				type{ _type },
				system{ std::make_unique<std::remove_cvref_t<TSystem>>(std::forward<TSystem>(system)) }
			{
			}
		};

		std::vector<SystemStorage> m_Systems;

		template <class TSystem, class TWorld>
		constexpr static decltype(auto) findSystemStorage(TWorld&& world) noexcept
		{
			std::type_index typeIndex = typeid(std::decay_t<TSystem>);
			return std::find_if(
								std::begin(world.m_Systems),
								std::end(world.m_Systems),
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

		auto takeDestructibleEntityUIDs() noexcept
		{
			std::scoped_lock lock{ m_DestructibleEntityMx };
			auto tmp = std::move(m_DestructibleEntityUIDs);
			return tmp;
		}

		auto takeNewEntities() noexcept
		{
			std::scoped_lock lock{ m_NewEntityMx };
			auto tmp = std::move(m_NewEntities);
			return tmp;
		}

		void processNewEntities()
		{
			m_InitializingEntities = takeNewEntities();
			if (std::empty(m_InitializingEntities))
				return;

			for (auto& entity : m_InitializingEntities)
				entity->changeState(EntityState::initializing);
		}

		void processInitializingEntities()
		{
			if (std::empty(m_InitializingEntities))
				return;

			for (auto& entity : m_InitializingEntities)
				entity->changeState(EntityState::running);

			std::scoped_lock lock{ m_EntityMx };
			m_Entities.insert(std::end(m_Entities), std::make_move_iterator(std::begin(m_InitializingEntities)), std::make_move_iterator(std::end(m_InitializingEntities)));
			m_InitializingEntities.clear();
		}

		void processEntityDestruction()
		{
			m_TeardownEntities.clear();

			auto destructibleEntityUIDs = takeDestructibleEntityUIDs();
			if (std::empty(destructibleEntityUIDs))
				return;

			std::sort(std::begin(destructibleEntityUIDs), std::end(destructibleEntityUIDs));
			destructibleEntityUIDs.erase(std::unique(std::begin(destructibleEntityUIDs), std::end(destructibleEntityUIDs)), std::end(destructibleEntityUIDs));

			std::scoped_lock entityLock{ m_EntityMx };
			m_TeardownEntities = std::move(m_Entities);

			std::set_difference(
								std::make_move_iterator(std::begin(m_TeardownEntities)),
								std::make_move_iterator(std::end(m_TeardownEntities)),
								std::begin(destructibleEntityUIDs),
								std::end(destructibleEntityUIDs),
								std::back_inserter(m_Entities),
								LessEntityByUID{}
								);

			m_TeardownEntities.erase(std::remove(std::begin(m_TeardownEntities), std::end(m_TeardownEntities), nullptr), std::end(m_TeardownEntities));
			for (auto& entity : m_TeardownEntities)
			{
				assert(entity);
				entity->changeState(EntityState::teardown);
			}
		}

		UID m_NextUID = 1;
		mutable std::mutex m_NewEntityMx;
		std::vector<std::unique_ptr<Entity>> m_NewEntities;

		std::vector<std::unique_ptr<Entity>> m_InitializingEntities;

		mutable std::shared_mutex m_EntityMx;
		std::vector<std::unique_ptr<Entity>> m_Entities;

		mutable std::mutex m_DestructibleEntityMx;
		std::vector<UID> m_DestructibleEntityUIDs;

		std::vector<std::unique_ptr<Entity>> m_TeardownEntities;
	};
}

#endif
