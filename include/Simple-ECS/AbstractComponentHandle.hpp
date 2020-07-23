
//          Copyright Dominic Koepke 2020 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#ifndef SECS_ABSTRACT_COMPONENT_HANDLE_HPP
#define SECS_ABSTRACT_COMPONENT_HANDLE_HPP

#pragma once

#include <cassert>
#include <typeindex>
#include "Typedefs.hpp"

namespace secs
{
	class AbstractComponentHandle
	{
	public:
		constexpr AbstractComponentHandle(const AbstractComponentHandle&) noexcept = delete;
		constexpr AbstractComponentHandle& operator =(const AbstractComponentHandle&) noexcept = delete;

		virtual ~AbstractComponentHandle() noexcept
		{
			m_UID = 0;
			m_RawPtr = nullptr;
		}

		constexpr UID getUID() const noexcept
		{
			return m_UID;
		}

		constexpr void* getRawPtr() const noexcept
		{
			return m_RawPtr;
		}

		virtual std::type_index getTypeInfo() const noexcept = 0;
		// ToDo: in cpp20
		virtual /*constexpr*/ void release() noexcept = 0;

	protected:
		constexpr AbstractComponentHandle() noexcept = default;

		template <class TComponent>
		constexpr AbstractComponentHandle(UID uid, TComponent& component) noexcept :
			m_UID{ uid },
			m_RawPtr{ static_cast<void*>(&component) }
		{
			assert(m_UID != 0 && m_RawPtr);
		}

		// ToDo: in cpp20
		/*constexpr*/ AbstractComponentHandle(AbstractComponentHandle&& other) noexcept
		{
			*this = std::move(other);
		}
		// ToDo: in cpp20
		/*constexpr*/ AbstractComponentHandle& operator =(AbstractComponentHandle&& other) noexcept
		{
			m_UID = other.m_UID;
			m_RawPtr = other.m_RawPtr;
			return *this;
		}

		// non-virtual is intended; just shadow in sub-class and call explicitly
		constexpr void reset() noexcept
		{
			m_UID = 0;
			m_RawPtr = 0;
		}

	private:
		UID m_UID = 0;
		void* m_RawPtr = nullptr;
	};
}

#endif
