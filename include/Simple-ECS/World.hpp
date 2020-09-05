//          Copyright Dominic Koepke 2020 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#ifndef SECS_WORLD_HPP
#define SECS_WORLD_HPP

#pragma once

#include <atomic>
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
	/** \class World
	 * \brief Class which stores unique Systems and manages Entities
	 *
	 * This class is used as the central point of this library. At first the user has to register each System he or she
	 * wants to use in its program. You may register systems later on but that might result in unexpected behaviour and
	 * is generally no thread-safe action. Systems are stored internally and each System type will be unique, thus
	 * if you register a system type twice or more, you will override the previous stored object.
	 *
	 * <STRONG>Systems will be updated in order of their registration.</STRONG>
	 *
	 * Creating Entities is designed to be thread-safe and can therefore happen anywhere in the program. They may also be
	 * registered for destruction anytime. Please keep in mind, that Entities will not directly be destroyed and will
	 * be valid for at least one update cycle, so that each System can safely perform their cleanup processes.
	 */
	class World
	{
	public:
		/**
		 * \brief Registers System
		 *
		 * This function will register a new System object at this World. If there already exists a System of type TSystem,
		 * it will be overridden.
		 * \tparam TSystem Concrete System type
		 * \param system Concrete System object.
		 * \return Reference to the registered System object.
		 */
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

		/**
		 * \brief Queries for a specific System type
		 *
		 * This function searches for a specific System type and returns a const pointer to the caller.
		 * \remark Please note, that the function only searches for concrete types and does lookup inheritances.
		 * \tparam TSystem System type to be found
		 * \return Const pointer to the stored system object or nullptr if not found.
		 */
		template <System TSystem>
		[[nodiscard]] const TSystem* findSystem() const noexcept
		{
			auto itr = findSystemStorage<TSystem>(*this);
			return itr != std::end(m_Systems) ? static_cast<TSystem*>(itr->system.get()) : nullptr;
		}

		/**
		 * \brief Queries for a specific System type
		 *
		 * This function searches for a specific System type and returns a pointer to the caller.
		 * \remark Please note, that the function only searches for concrete types and does lookup inheritances.
		 * \tparam TSystem System type to be found
		 * \return Pointer to the stored system object or nullptr if not found.
		 */
		template <System TSystem>
		[[nodiscard]] TSystem* findSystem() noexcept
		{
			return const_cast<TSystem*>(std::as_const(*this).findSystem<TSystem>());
		}

		/**
		 * \brief Queries for a specific System type
		 *
		 * This function searches for a specific System type and returns a const reference to the caller.
		 * \remark Please note, that the function only searches for concrete types and does lookup inheritances.
		 * \throws SystemError if a System object of type TSystem could not be found.
		 * \tparam TSystem System type to be found
		 * \return Const reference to the stored system object.
		 */
		template <System TSystem>
		[[nodiscard]] const TSystem& system() const
		{
			if (auto* ptr = findSystem<TSystem>())
				return *ptr;
			using namespace std::string_literals;
			throw SystemError("System not found: "s + typeid(TSystem).name());
		}

		/**
		 * \brief Queries for a specific System type
		 *
		 * This function searches for a specific System type and returns a reference to the caller.
		 * \remark Please note, that the function only searches for concrete types and does lookup inheritances.
		 * \throws SystemError if a System object of type TSystem could not be found.
		 * \tparam TSystem System type to be found
		 * \return Reference to the stored system object.
		 */
		template <System TSystem>
		[[nodiscard]] TSystem& system()
		{
			return const_cast<TSystem&>(std::as_const(*this).system<TSystem>());
		}

		/**
		 * \brief Queries a SystemBase type which relates to the passed Component type
		 *
		 * This function searches for a specific SystemBase type which relates to the passed TComponent type.
		 * If you need the concrete type of that System object it is generally safe to cast it to that.
		 * \tparam TComponent Component type to be searched for
		 * \return Const pointer to the corresponding SystemBase object or nullptr if not found.
		 */
		template <Component TComponent>
		[[nodiscard]] const SystemBase<TComponent>* findSystemByComponentType() const noexcept
		{
			auto itr = findSystemStorageForComponent<TComponent>(*this);
			return itr != std::end(m_Systems) ? static_cast<SystemBase<TComponent>*>(itr->system.get()) : nullptr;
		}

		/**
		 * \brief Queries a SystemBase type which relates to the passed Component type
		 *
		 * This function searches for a specific SystemBase type which relates to the passed TComponent type.
		 * If you need the concrete type of that System object it is generally safe to cast it to that.
		 * \tparam TComponent Component type to be searched for
		 * \return Pointer to the corresponding SystemBase object or nullptr if not found.
		 */
		template <Component TComponent>
		[[nodiscard]] SystemBase<TComponent>* findSystemByComponentType() noexcept
		{
			return const_cast<SystemBase<TComponent>*>(std::as_const(*this).findSystemByComponentType<TComponent>());
		}

		/**
		 * \brief Queries a SystemBase type which relates to the passed Component type
		 *
		 * This function searches for a specific SystemBase type which relates to the passed TComponent type.
		 * If you need the concrete type of that System object it is generally safe to cast it to that.
		 * \throws SystemError if a related SystemBase object could not be found.
		 * \tparam TComponent Component type to be searched for
		 * \return Const reference to the stored SystemBase object related to the passed Component type.
		 */
		template <Component TComponent>
		[[nodiscard]] const SystemBase<TComponent>& systemByComponentType() const
		{
			if (auto* ptr = findSystemByComponentType<TComponent>())
				return *ptr;
			using namespace std::string_literals;
			throw SystemError("System for component not found: "s + typeid(TComponent).name());
		}

		/**
		 * \brief Queries a SystemBase type which relates to the passed Component type
		 *
		 * This function searches for a specific SystemBase type which relates to the passed TComponent type.
		 * If you need the concrete type of that System object it is generally safe to cast it to that.
		 * \throws SystemError if a related SystemBase object could not be found.
		 * \tparam TComponent Component type to be searched for
		 * \return Reference to the stored SystemBase object related to the passed Component type.
		 */
		template <Component TComponent>
		[[nodiscard]] SystemBase<TComponent>& systemByComponentType()
		{
			return const_cast<SystemBase<TComponent>&>(std::as_const(*this).systemByComponentType<TComponent>());
		}

		/**
		 * \brief Creates new Entity with specified Components
		 *
		 * A new Entity with one Component object for each of the passed Component types will be created. It is safe to use and store
		 * the reference to the newly constructed Entity.
		 * \tparam TComponent Indefinite amount of Component types
		 * \return Returns a reference to the newly constructed Entity.
		 */
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
			++m_EntityCount;
			return *m_NewEntities.back();
		}

		/**
		 * \brief Registers Entity for destruction
		 *
		 * This function registers a Entity for destruction. It does not perform any checks at this stage if a corresponding
		 * Entity exists or if the given uid is already registered, thus it is generally safe to pass any possible uid.
		 * \remark The Entity will not be directly destroyed, thus it is safe to use existing pointers and references to it during the next cycle.
		 * \param uid Uid of the corresponding Entity which should be destructed.
		 */
		void destroyEntityLater(Uid uid)
		{
			std::scoped_lock lock{ m_DestructibleEntityMx };
			m_DestructibleEntities.emplace_back(uid);
		}

		/**
		 * \brief Searches for the corresponding Entity
		 *
		 * This function searches for the Entity with the passed uid. Due to the huge amount of different containers which
		 * have to be searched, this is a quite expensive operation and should therefore be avoided if not necessary. Cache
		 * pointers or references instead.
		 * \param uid Entity Uid
		 * \return Const pointer to the corresponding Entity or nullptr if not found.
		 */
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

		/**
		 * \brief Searches for the corresponding Entity
		 *
		 * This function searches for the Entity with the passed uid. Due to the huge amount of different containers which
		 * have to be searched, this is a quite expensive operation and should therefore be avoided if not necessary. Cache
		 * pointers or references instead.
		 * \param uid Entity Uid
		 * \return Pointer to the corresponding Entity or nullptr if not found.
		 */
		[[nodiscard]] Entity* findEntity(Uid uid) noexcept
		{
			return const_cast<Entity*>(std::as_const(*this).findEntity(uid));
		}

		/**
		 * \brief Searches for the corresponding Entity
		 *
		 * This function searches for the Entity with the passed uid. Due to the huge amount of different containers which
		 * have to be searched, this is a quite expensive operation and should therefore be avoided if not necessary. Cache
		 * pointers or references instead.
		 * \throws EntityError if corresponding Entity could not be found.
		 * \param uid Entity Uid
		 * \return Const reference to the corresponding Entity.
		 */
		[[nodiscard]] const Entity& entity(Uid uid) const
		{
			if (const auto* ptr = findEntity(uid))
				return *ptr;
			using namespace std::string_literals;
			throw EntityError("Entity uid: "s + std::to_string(uid) + " not found: ");
		}

		/**
		 * \brief Searches for the corresponding Entity
		 *
		 * This function searches for the Entity with the passed uid. Due to the huge amount of different containers which
		 * have to be searched, this is a quite expensive operation and should therefore be avoided if not necessary. Cache
		 * pointers or references instead.
		 * \throws EntityError if corresponding Entity could not be found.
		 * \param uid Entity Uid
		 * \return Reference to the corresponding Entity.
		 */
		[[nodiscard]] Entity& entity(Uid uid)
		{
			return const_cast<Entity&>(std::as_const(*this).entity(uid));
		}

		/**
		 * \brief Entity count
		 * \return Returns amount of stored Entities.
		 */
		std::size_t entityCount() const noexcept
		{
			return m_EntityCount;
		}

		/**
		 * \brief Pre updates all Systems
		 *
		 * This function calls the preUpdate functions of every registered System, which may be used to perform necessary preparations
		 * for the actual updating process.
		 */
		void preUpdate() noexcept
		{
			for (auto& storage : m_Systems)
				storage.system->preUpdate();
		}

		/**
		 * \brief Updates all Systems
		 *
		 * This function calls the update functions of every registered System.
		 * \param delta Time delta between the previous and current update cycle
		 */
		void update(float delta) noexcept
		{
			for (auto& storage : m_Systems)
				storage.system->update(delta);
		}

		/**
		 * \brief Post updates all Systems
		 *
		 * This function calls the postUpdate functions of every registered System, which may be used to perform necessary finalization steps
		 * for the current update process.
		 * \remark In this function Entities with state teardown will be destructed and and other Entities may change their state.
		 */
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
			auto tmp = std::move(m_DestructibleEntities);
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
			assert(std::size(m_TeardownEntities) <= m_EntityCount);
			m_EntityCount -= std::size(m_TeardownEntities);
			m_TeardownEntities.clear();

			auto destructibleEntityUIDs = takeDestructibleEntityUIDs();
			if (std::empty(destructibleEntityUIDs))
				return;

			std::sort(std::begin(destructibleEntityUIDs), std::end(destructibleEntityUIDs));
			destructibleEntityUIDs.erase(std::unique(std::begin(destructibleEntityUIDs), std::end(destructibleEntityUIDs)), std::end(destructibleEntityUIDs));

			std::scoped_lock entityLock{ m_EntityMx, m_NewEntityMx };
			auto moveDestructibleEntities = [&teardownEntities = m_TeardownEntities](auto& entityRange, const auto& destructibleIds)
			{
				std::set_intersection(
								std::make_move_iterator(std::begin(entityRange)),
								std::make_move_iterator(std::end(entityRange)),
								std::begin(destructibleIds),
								std::end(destructibleIds),
								std::back_inserter(teardownEntities),
								EntityLessByUid{}
								);
				entityRange.erase(std::remove(std::begin(entityRange), std::end(entityRange), nullptr), std::end(entityRange));
			};

			moveDestructibleEntities(m_Entities, destructibleEntityUIDs);
			moveDestructibleEntities(m_InitializingEntities, destructibleEntityUIDs);
			moveDestructibleEntities(m_NewEntities, destructibleEntityUIDs);

			for (auto& entity : m_TeardownEntities)
			{
				assert(entity);
				entity->changeState(EntityState::teardown);
			}
		}

		std::atomic<std::size_t> m_EntityCount{ 0 };
		Uid m_NextUID = 1;
		mutable std::mutex m_NewEntityMx;
		std::vector<std::unique_ptr<Entity>> m_NewEntities;

		std::vector<std::unique_ptr<Entity>> m_InitializingEntities;

		mutable std::mutex m_EntityMx;
		std::vector<std::unique_ptr<Entity>> m_Entities;

		mutable std::mutex m_DestructibleEntityMx;
		std::vector<Uid> m_DestructibleEntities;

		std::vector<std::unique_ptr<Entity>> m_TeardownEntities;
	};
}

#endif
