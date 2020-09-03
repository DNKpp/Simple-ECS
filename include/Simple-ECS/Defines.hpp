//          Copyright Dominic Koepke 2020 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#ifndef SECS_DEFINES_HPP
#define SECS_DEFINES_HPP

#pragma once

#include <cstddef>

namespace secs
{
	/** \typedef Uid
	 * \brief Alias which is used as scope based unique identifier.
	 */
	using Uid = std::size_t;

	/** \enum EntityState
	 * \brief States an Entity may have.
	 *
	 * An Entity may change its state during the World's postUpdate call.
	 */
	enum class EntityState
	{
		none,
		initializing,
		running,
		teardown
	};

	/** \var EntityState EntityState::none
	 * \brief Initial state
	 *
	 * 
	 * Nothing has happened yet except the Entity and its components have been created, thus it is safe to store references and pointers to them, but initial
	 * components still may change within next state.
	*/

	/** \var EntityState EntityState::initializing
	 * \brief Initializing state
	 *
	 * 
	 * The components of the corresponding Entity have been notified and should finally be set up. This state last for exactly one World update cycle.
	*/

	/** \var EntityState EntityState::running
	 * \brief Running state
	 *
	 * 
	 * This states indicates, that the Entity is running. This lasts until the World receives the sign to destroy the corresponding Entity.
	*/

	/** \var EntityState EntityState::teardown
	 * \brief Teardown state
	 * 
	 * This states indicates, that the Entity is about to be destroyed. It is safe to use pointers and references to the Entity and its components, but
	 * the components have already been informed that they are about to destruct. This state lasts exactly for one World update cycle and after the next
	 * postUpdate call, the Entity finally gets destroyed and the components may be reused, thus the user has to make sure that he cleans up everything
	 * related to the corresponding Entity.
	*/
}

#endif
