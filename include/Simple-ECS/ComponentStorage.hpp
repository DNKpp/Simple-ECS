
//          Copyright Dominic Koepke 2020 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#ifndef SECS_COMPONENT_STORAGE_HPP
#define SECS_COMPONENT_STORAGE_HPP

#pragma once

#include <tuple>
#include <typeindex>

#include "ComponentHandle.hpp"
#include "ComponentTraits.hpp"
#include "Concepts.hpp"

namespace secs
{
	class Entity;

	class BaseComponentStorage
	{
	public:
		constexpr BaseComponentStorage(const BaseComponentStorage&) noexcept = delete;
		constexpr BaseComponentStorage& operator =(const BaseComponentStorage&) noexcept = delete;
		constexpr BaseComponentStorage(BaseComponentStorage&&) noexcept = delete;
		constexpr BaseComponentStorage& operator =(BaseComponentStorage&&) noexcept = delete;
		
		/*ToDo: c++20
		constexpr */virtual ~BaseComponentStorage() noexcept = default;

		template <class TComponent>
		// ToDo: c++20
		[[nodiscard]] /*constexpr */bool hasComponent() const noexcept
		{
			return hasComponentImpl(typeid(TComponent));
		}

		template <class TComponent>
		// ToDo: c++20
		[[nodiscard]] /*constexpr */const TComponent* getComponent() const noexcept
		{
			return static_cast<const TComponent*>(getComponentImpl(typeid(TComponent)));
		}

		template <class TComponent>
		// ToDo: c++20
		[[nodiscard]] /*constexpr */TComponent* getComponent() noexcept
		{
			return static_cast<TComponent*>(getComponentImpl(typeid(TComponent)));
		}

		// ToDo: c++20
		 /*constexpr */void onEntityStateChanged(Entity& entity) noexcept
		{
			onEntityStateChangedImpl(entity);
		}

		virtual void setupEntity(Entity& entity) noexcept = 0;

	protected:
		constexpr BaseComponentStorage() noexcept = default;

	private:
		// ToDo: c++20
		[[nodiscard]] /*constexpr*/ virtual bool hasComponentImpl(std::type_index typeIndex) const noexcept = 0;
		[[nodiscard]] /*constexpr*/ virtual const void* getComponentImpl(std::type_index typeIndex) const noexcept = 0;
		[[nodiscard]] /*constexpr*/ virtual void* getComponentImpl(std::type_index typeIndex) noexcept = 0;
		/*constexpr*/ virtual void onEntityStateChangedImpl(Entity& entity) noexcept = 0;
	};

	template <Component... TComponent>
	class ComponentStorage final :
		public BaseComponentStorage
	{
	private:
		template <Component T2Component>
		using HandleType = typename ComponentTraits<T2Component>::ComponentHandleType;

	public:
		constexpr ComponentStorage() noexcept = delete;
		constexpr ComponentStorage(const ComponentStorage&) noexcept = delete;
		constexpr ComponentStorage& operator =(const ComponentStorage&) noexcept = delete;
		constexpr ComponentStorage(ComponentStorage&&) noexcept = delete;
		constexpr ComponentStorage& operator =(ComponentStorage&&) noexcept = delete;
		
		constexpr ComponentStorage(HandleType<TComponent>&&... handle) noexcept :
			BaseComponentStorage{},
			m_ComponentHandles{ std::forward<HandleType<TComponent>>(handle)... }
		{}

		/*ToDo: c++20
		constexpr */~ComponentStorage() noexcept override = default;

	private:
		std::tuple<HandleType<TComponent>...> m_ComponentHandles;

		void setupEntity(Entity& entity) noexcept override
		{
			auto exec = [&entity](auto& handle)
			{
				assert(!handle.isEmpty());
				handle.setupEntity(entity);
			};
			(exec(std::get<HandleType<TComponent>>(m_ComponentHandles)), ...);
		}

		// ToDo: c++20
		[[nodiscard]] /*constexpr */bool hasComponentImpl(std::type_index typeIndex) const noexcept override
		{
			return ((std::type_index(typeid(TComponent)) == typeIndex) || ...);
		}

		// ToDo: c++20
		[[nodiscard]] /*constexpr */const void* getComponentImpl(std::type_index typeIndex) const noexcept override
		{
			const void* component = nullptr;
			auto find = [&component, typeIndex](const auto& param)
			{
				using ComponentType = typename std::decay_t<decltype(param)>::ComponentType;
				if (!component && std::type_index(typeid(ComponentType)) == typeIndex)
				{
					component = &param.getComponent();
				}
			};
			(find(std::get<HandleType<TComponent>>(m_ComponentHandles)), ...);
			return component;
		}

		// ToDo: c++20
		[[nodiscard]] /*constexpr */void* getComponentImpl(std::type_index typeIndex) noexcept override
		{
			return const_cast<void*>(std::as_const(*this).getComponentImpl(typeIndex));
		}

		// ToDo: c++20
		/*constexpr */void onEntityStateChangedImpl(Entity& entity) noexcept override
		{
			auto exec = [&entity](auto& handle)
			{
				assert(!handle.isEmpty());
				handle.getSystem().onEntityStateChanged(handle.getUID(), entity);
			};
			(exec(std::get<HandleType<TComponent>>(m_ComponentHandles)), ...);
		}

		//template <class TComponents>
		//consteval static auto findComponent(TComponents&& components, std::type_index typeIndex) noexcept
		//{
		//	//const void* component = nullptr;
		//	//auto find = [&component, typeIndex](const auto& param)
		//	//{
		//	//	if (!component && std::type_index(typeid(param)) == typeIndex)
		//	//	{
		//	//		component = &param;
		//	//	}
		//	//};
		//	//(find(*std::get<TComponentHandle>(m_ComponentHandles).system), ...);
		//	//return component;
		//	for (std::size_t i = 0; i < sizeof...(components); ++i)
		//	{
		//		if (std::type_index(typeid(typename decltype(std::get<i>(components))::ComponentType) == typeIndex)
		//			return &std::get<i>(components).getComponent();
		//	}
		//}
	};

	template <class... TComponentHandles>
	ComponentStorage(TComponentHandles...) -> ComponentStorage<typename TComponentHandles::ComponentType...>;
}
#endif
