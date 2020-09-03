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
#include <ranges>
#include <string>
#include <typeinfo>
#include <type_traits>
#include <vector>

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
				*itr = { typeid(TSystem), typeid(typename TSystem::ComponentType), std::forward<TSystem>(system) };
				return static_cast<TSystem&>(*itr->system);
			}
			auto& ref = m_Systems.emplace_back(typeid(TSystem), typeid(typename TSystem::ComponentType), std::forward<TSystem>(system));
			return static_cast<TSystem&>(*ref.system);
		}

		template <System TSystem>
		[[nodiscard]] const TSystem* findSystem() const noexcept
		{
			auto itr = findSystemStorage<TSystem>(*this);
			return itr != std::end(m_Systems) ? static_cast<TSystem*>(itr->system.get()) : nullptr;
		}

		template <System TSystem>
		[[nodiscard]] TSystem* findSystem() noexcept
		{
			return const_cast<TSystem*>(std::as_const(*this).findSystem<TSystem>());
		}

		template <System TSystem>
		[[nodiscard]] const TSystem& system() const
		{
			if (auto* ptr = findSystem<TSystem>())
				return *ptr;
			using namespace std::string_literals;
			throw SystemError("System not found: "s + typeid(TSystem).name());
		}

		template <System TSystem>
		[[nodiscard]] TSystem& system()
		{
			return const_cast<TSystem&>(std::as_const(*this).system<TSystem>());
		}

		template <Component TComponent>
		[[nodiscard]] const SystemBase<TComponent>* findSystemByComponentType() const noexcept
		{
			auto itr = findSystemStorageForComponent<TComponent>(*this);
			return itr != std::end(m_Systems) ? static_cast<SystemBase<TComponent>*>(itr->system.get()) : nullptr;
		}

		template <Component TComponent>
		[[nodiscard]] SystemBase<TComponent>* findSystemByComponentType() noexcept
		{
			return const_cast<SystemBase<TComponent>*>(std::as_const(*this).findSystemByComponentType<TComponent>());
		}

		template <Component TComponent>
		[[nodiscard]] const SystemBase<TComponent>& systemByComponentType() const
		{
			if (auto* ptr = findSystemByComponentType<TComponent>())
				return *ptr;
			using namespace std::string_literals;
			throw SystemError("System for component not found: "s + typeid(TComponent).name());
		}

		template <Component TComponent>
		[[nodiscard]] SystemBase<TComponent>& systemByComponentType()
		{
			return const_cast<SystemBase<TComponent>&>(std::as_const(*this).systemByComponentType<TComponent>());
		}

		template <Component... TComponent>
		Entity& createEntity()
		{
			std::scoped_lock entityLock{ m_NewEntityMx };

			auto entityUID = m_NextUID++;
			m_NewEntities.emplace_back(
										std::make_unique<Entity>(
																entityUID,
																std::vector<detail::ComponentStorageInfo>{ makeComponentStorageInfo(systemByComponentType<TComponent>())... }
																)
									);
			return *m_NewEntities.back();
		}

		void destroyEntityLater(Uid uid)
		{
			std::scoped_lock lock{ m_DestructibleEntityMx };
			m_DestructibleEntityUIDs.emplace_back(uid);
		}

		[[nodiscard]] const Entity* findEntity(Uid uid) const noexcept
		{
			std::scoped_lock entityLock{ m_EntityMx, m_NewEntityMx };
			if (const auto itr = findEntityItr(m_Entities, uid); itr != std::end(m_Entities))
			{
				return &**itr;
			}

			if (const auto itr = findEntityItr(m_InitializingEntities, uid); itr != std::end(m_InitializingEntities))
			{
				return &**itr;
			}

			if (const auto itr = findEntityItr(m_NewEntities, uid); itr != std::end(m_NewEntities))
			{
				return &**itr;
			}
		
			if (const auto itr = findEntityItr(m_TeardownEntities, uid); itr != std::end(m_TeardownEntities))
			{
				return &**itr;
			}
			return nullptr;
		}

		[[nodiscard]] Entity* findEntity(Uid uid) noexcept
		{
			return const_cast<Entity*>(std::as_const(*this).findEntity(uid));
		}

		[[nodiscard]] const Entity& entity(Uid uid) const
		{
			if (const auto* ptr = findEntity(uid))
				return *ptr;
			using namespace std::string_literals;
			throw EntityError("Entity uid: "s + std::to_string(uid) + " not found: ");
		}

		[[nodiscard]] Entity& entity(Uid uid)
		{
			return const_cast<Entity&>(std::as_const(*this).entity(uid));
		}

		void preUpdate() noexcept
		{
			for (auto& storage : m_Systems)
				storage.system->preUpdate();
		}

		void update(float delta) noexcept
		{
			for (auto& storage : m_Systems)
				storage.system->update(delta);
		}

		void postUpdate()
		{
			postUpdateSystems();

			processInitializingEntities();
			processNewEntities();
			processEntityDestruction();
		}

	private:
		struct SystemStorage
		{
			std::type_index type;
			std::type_index componentType;
			std::unique_ptr<ISystem> system;

			template <class TSystem>
			constexpr SystemStorage(std::type_index type_, std::type_index componentType_, TSystem&& system_) :
				type{ type_ },
				componentType{ componentType_ },
				system{ std::make_unique<std::remove_cvref_t<TSystem>>(std::forward<TSystem>(system_)) }
			{
			}
		};

		std::vector<SystemStorage> m_Systems;

		template <class TSystem>
		detail::ComponentStorageInfo makeComponentStorageInfo(TSystem& system)
		{
			auto uid = system.createComponent();
			using ComponentType = typename TSystem::ComponentType;
			return { &system, uid, typeid(ComponentType), &detail::componentRtti<ComponentType> };
		}

		template <System TSystem, class TWorld>
		constexpr static decltype(auto) findSystemStorage(TWorld&& world) noexcept
		{
			return std::ranges::find(
									world.m_Systems,
									typeid(std::decay_t<TSystem>),
									[](const auto& storage) { return storage.type; }
									);
		}

		template <Component TComponent, class TWorld>
		constexpr static decltype(auto) findSystemStorageForComponent(TWorld&& world) noexcept
		{
			return std::ranges::find(
									world.m_Systems,
									typeid(std::decay_t<TComponent>),
									[](const auto& storage) { return storage.componentType; }
									);
		}

		void postUpdateSystems() noexcept
		{
			for (auto& storage : m_Systems)
				storage.system->postUpdate();
		}

		template <class TContainer>
		constexpr static auto findEntityItr(TContainer& container, Uid uid) noexcept -> decltype(std::begin(container))
		{
			if (auto itr = std::lower_bound(std::begin(container), std::end(container), uid, EntityLessByUid{});
				itr != std::end(container) && (*itr)->uid() == uid)
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

		void processNewEntities() noexcept
		{
			m_InitializingEntities = takeNewEntities();
			for (auto& entity : m_InitializingEntities)
			{
				entity->changeState(EntityState::initializing);
			}
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
								EntityLessByUid{}
								);

			m_TeardownEntities.erase(std::remove(std::begin(m_TeardownEntities), std::end(m_TeardownEntities), nullptr), std::end(m_TeardownEntities));
			for (auto& entity : m_TeardownEntities)
			{
				assert(entity);
				entity->changeState(EntityState::teardown);
			}
		}

		Uid m_NextUID = 1;
		mutable std::mutex m_NewEntityMx;
		std::vector<std::unique_ptr<Entity>> m_NewEntities;

		std::vector<std::unique_ptr<Entity>> m_InitializingEntities;

		mutable std::mutex m_EntityMx;
		std::vector<std::unique_ptr<Entity>> m_Entities;

		mutable std::mutex m_DestructibleEntityMx;
		std::vector<Uid> m_DestructibleEntityUIDs;

		std::vector<std::unique_ptr<Entity>> m_TeardownEntities;
	};
}

#endif
