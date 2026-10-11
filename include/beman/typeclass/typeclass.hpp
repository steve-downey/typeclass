// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef BEMAN_TYPECLASS_TYPECLASS_HPP
#define BEMAN_TYPECLASS_TYPECLASS_HPP

#include <beman/typeclass/config.hpp>

#if BEMAN_TYPECLASS_USE_MODULES() && !defined(BEMAN_TYPECLASS_INCLUDED_FROM_INTERFACE_UNIT)

import beman.typeclass;

#else

    #include <beman/typeclass/todo.hpp>

#endif // BEMAN_TYPECLASS_USE_MODULES() &&
       // !defined(BEMAN_TYPECLASS_INCLUDED_FROM_INTERFACE_UNIT)

#endif // BEMAN_TYPECLASS_TYPECLASS_HPP
