
//          Copyright Dominic Koepke 2020 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#ifndef SECS_COMPONENT_TRAITS_HPP
#define SECS_COMPONENT_TRAITS_HPP

#pragma once

namespace secs
{
	template <class TComponent>
	class SystemBase;

	template <class TSystem>
	class ComponentHandle;

	template <class TComponent>
	struct ComponentTraits
	{
		using ComponentType = TComponent;
		using SystemBaseType = SystemBase<ComponentType>;
		using ComponentHandleType = ComponentHandle<SystemBaseType>;
	};
}

#endif
