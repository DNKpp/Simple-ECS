
//          Copyright Dominic Koepke 2020 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#ifndef SECS_ENTITY_HPP
#define SECS_ENTITY_HPP

#pragma once

#include <vector>
#include <typeindex>
#include <memory>
#include <cassert>

#include "AbstractComponentHandle.hpp"
#include "Typedefs.hpp"

namespace secs
{
	class Entity
	{
	public:
		template <class... TComponentHandles>
		constexpr Entity(UID uid, TComponentHandles&&... handles) :
			m_UID{ uid },
			m_Components{ makeComponentInfos(std::forward<TComponentHandles>(handles)...) }
		{
			assert(m_UID != 0);
		}

		template <class TComponent>
		constexpr TComponent* getComponent() noexcept
		{
			// ToDo: performe binary lookup
			std::type_index typeIndex = typeid(TComponent);
			for (auto& handle : m_Components)
			{
				if (handle->getTypeInfo() == typeIndex)
					return static_cast<TComponent*>(handle->getRawPtr());
			}
			return nullptr;
		}

	private:
		UID m_UID;

		//struct ComponentInfo
		//{
		//	std::type_index type;
		//	std::unique_ptr<AbstractComponentHandle> handle;

		//	template <class TComponentHandle>
		//	constexpr ComponentInfo(TComponentHandle&& _handle) :
		//		type{ _handle.getTypeInfo() },
		//		handle{ std::make_unique<TComponentHandle>(std::forward<TComponentHandle>(_handle)) }
		//	{
		//	}
		//};
		//std::vector<ComponentInfo> m_Components;

		// ToDo: use sorted container (by std::type_index) for faster lookups
		std::vector<std::unique_ptr<AbstractComponentHandle>> m_Components;

		template <class... TComponentHandles>
		static constexpr decltype(auto) makeComponentInfos(TComponentHandles&&... handles)
		{
			std::vector<std::unique_ptr<AbstractComponentHandle>> components;
			components.reserve(sizeof...(handles));
			(components.emplace_back(std::make_unique<TComponentHandles>(std::forward<TComponentHandles>(handles))), ...);
			return components;
		}
	};
}

#endif
