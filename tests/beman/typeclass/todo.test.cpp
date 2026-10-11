// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/typeclass/config.hpp>
#include <catch2/catch_all.hpp>
#include <beman/typeclass/todo.hpp>

TEST_CASE("todo", "[typeclass::todo]") {
    const bool todo = true;
    CHECK(todo);
}
