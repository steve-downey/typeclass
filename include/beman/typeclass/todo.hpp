// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef BEMAN_TYPECLASS_TODO_HPP
#define BEMAN_TYPECLASS_TODO_HPP

#include <beman/typeclass/config.hpp>

#if BEMAN_TYPECLASS_USE_MODULES() && !defined(BEMAN_TYPECLASS_INCLUDED_FROM_INTERFACE_UNIT)

import beman.typeclass;

#else

namespace beman::typeclass {

// TODO

} // namespace beman::typeclass

#endif // BEMAN_TYPECLASS_USE_MODULES() &&
       // !defined(BEMAN_TYPECLASS_INCLUDED_FROM_INTERFACE_UNIT)

#endif // BEMAN_TYPECLASS_TODO_HPP
