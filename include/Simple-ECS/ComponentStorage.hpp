
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
#include "Defines.hpp"
#include "Typedefs.hpp"

namespace secs
{
	class Entity;

	class BaseComponentStorage
	{
	public:
		template <class TComponent>
		/* ToDo: c++20
		constexpr */bool hasComponent() const noexcept
		{
			return hasComponentImpl(typeid(TComponent));
		}

		template <class TComponent>
		/* ToDo: c++20
		constexpr */const TComponent* getComponent() const noexcept
		{
			return static_cast<const TComponent*>(getComponentImpl(typeid(TComponent)));
		}

		template <class TComponent>
		/* ToDo: c++20
		constexpr */TComponent* getComponent() noexcept
		{
			return static_cast<TComponent*>(getComponentImpl(typeid(TComponent)));
		}

		/* ToDo: c++20
		constexpr */void onEntityStateChanged(Entity& entity) noexcept
		{
			onEntityStateChangedImpl(entity);
		}

	protected:
		constexpr BaseComponentStorage() noexcept = default;

	private:
		// ToDo: c++20
		/*constexpr*/ virtual bool hasComponentImpl(std::type_index typeIndex) const noexcept = 0;
		/*constexpr*/ virtual const void* getComponentImpl(std::type_index typeIndex) const noexcept = 0;
		/*constexpr*/ virtual void* getComponentImpl(std::type_index typeIndex) noexcept = 0;
		/*constexpr*/ virtual void onEntityStateChangedImpl(Entity& entity) noexcept = 0;
	};

	template <class... TComponent>
	class ComponentStorage :
		public BaseComponentStorage
	{
	private:
		template <class UComponent>
		using HandleType = typename ComponentTraits<UComponent>::ComponentHandleType;

	public:
		constexpr ComponentStorage(HandleType<TComponent>&&... handle) noexcept :
			BaseComponentStorage{},
			m_ComponentHandles{ std::forward<HandleType<TComponent>>(handle)... }
		{}

	private:
		std::tuple<HandleType<TComponent>...> m_ComponentHandles;

		/* ToDo: c++20
		constexpr */bool hasComponentImpl(std::type_index typeIndex) const noexcept override
		{
			return ((std::type_index(typeid(TComponent)) == typeIndex) || ...);
		}

		/* ToDo: c++20
		constexpr */const void* getComponentImpl(std::type_index typeIndex) const noexcept override
		{
			const void* component = nullptr;
			auto find = [&component, typeIndex](const auto& param)
			{
				if (!component && std::type_index(typeid(param)) == typeIndex)
				{
					component = &param;
				}
			};
			(find(std::get<HandleType<TComponent>>(m_ComponentHandles).getSystem()), ...);
			return component;
		}

		/* ToDo: c++20
		constexpr */void* getComponentImpl(std::type_index typeIndex) noexcept override
		{
			return const_cast<void*>(std::as_const(*this).getComponentImpl(typeIndex));
		}

		/* ToDo: c++20
		constexpr */void onEntityStateChangedImpl(Entity& entity) noexcept override
		{
			(emitEntityStateChange(std::get<HandleType<TComponent>>(m_ComponentHandles), entity), ...);
		}

		template <class THandle>
		constexpr void emitEntityStateChange(THandle& handle, Entity& entity) noexcept
		{
			assert(!handle.isEmpty());
			handle.getSystem().onEntityStateChanged(handle.getUID(), entity);
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
}
#endif
