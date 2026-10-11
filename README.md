# beman.typeclass: Typeclass Based Libraries for C++

<!--
SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
-->

<!-- markdownlint-disable line-length -->
[![Library Status](https://raw.githubusercontent.com/bemanproject/beman/refs/heads/main/images/badges/beman_badge-beman_library_under_development.svg)](https://github.com/bemanproject/beman/blob/main/docs/beman_library_maturity_model.md#the-beman-library-maturity-model)
[![Continuous Integration Tests](https://github.com/bemanproject/typeclass/actions/workflows/ci_tests.yml/badge.svg)](https://github.com/bemanproject/typeclass/actions/workflows/ci_tests.yml)
[![Lint Check (pre-commit)](https://github.com/bemanproject/typeclass/actions/workflows/pre-commit-check.yml/badge.svg)](https://github.com/bemanproject/typeclass/actions/workflows/pre-commit-check.yml)
[![Coverage](https://coveralls.io/repos/github/bemanproject/typeclass/badge.svg?branch=main)](https://coveralls.io/github/bemanproject/typeclass?branch=main)
![Standard Target](https://github.com/bemanproject/beman/blob/main/images/badges/cpp29.svg)
<!-- markdownlint-restore -->

`beman.typeclass` is (... TODO: description).

**Implements**: `std::todo` proposed in [TODO (P3200R0)](https://wg21.link/P3200R0).

**Status**: [Under development and not yet ready for production use.](https://github.com/bemanproject/beman/blob/main/docs/beman_library_maturity_model.md#under-development-and-not-yet-ready-for-production-use)

## License

`beman.typeclass` is licensed under the Apache License v2.0 with LLVM Exceptions.

## Usage

TODO

Full runnable examples can be found in [`examples/`](examples/).

## Dependencies

### Build Environment

This project requires at least the following to build:

* A C++ compiler that conforms to the C++26 standard or greater
* CMake 3.30 or later
* (Test Only) GoogleTest

You can disable building tests by setting CMake option `BEMAN_TYPECLASS_BUILD_TESTS` to
`OFF` when configuring the project.

You can disable building examples by setting CMake option `BEMAN_TYPECLASS_BUILD_EXAMPLES` to
`OFF` when configuring the project.

### Supported Platforms

| Compiler   | Version | C++ Standards | Standard Library  |
|------------|---------|---------------|-------------------|
| GCC        | 16-13   | C++26-C++17   | libstdc++         |
| GCC        | 12-11   | C++23-C++17   | libstdc++         |
| Clang      | 22-19   | C++26-C++17   | libstdc++, libc++ |
| Clang      | 18      | C++26-C++17   | libc++            |
| Clang      | 18      | C++23-C++17   | libstdc++         |
| Clang      | 17      | C++26-C++17   | libc++            |
| Clang      | 17      | C++20, C++17  | libstdc++         |
| AppleClang | latest  | C++26-C++17   | libc++            |
| MSVC       | latest  | C++23         | MSVC STL          |

## Development

See the [Contributing Guidelines](CONTRIBUTING.md).

## Integrate beman.typeclass into your project

### Build

You can build typeclass using a CMake workflow preset:

```bash
cmake --workflow --preset gcc-release
```

To list available workflow presets, you can invoke:

```bash
cmake --list-presets=workflow
```

For details on building beman.typeclass without using a CMake preset, refer to the
[Contributing Guidelines](CONTRIBUTING.md).

### Installation

#### Vcpkg

The preferred way to install typeclass is via vcpkg. To do so, after installing vcpkg
itself, you need to add support for the Beman project's [vcpkg
registry](https://github.com/bemanproject/vcpkg-registry) by configuring a
`vcpkg-configuration.json` file (which typeclass [provides](vcpkg-configuration.json)).

Then, simply run `vcpkg install beman-typeclass`.

#### Manual

To install beman.typeclass globally after building with the `gcc-release` preset, you can
run:

```bash
sudo cmake --install build/gcc-release
```

Alternatively, to install to a prefix, for example `/opt/beman`, you can run:

```bash
sudo cmake --install build/gcc-release --prefix /opt/beman
```

This will generate the following directory structure:

```txt
/opt/beman
├── include
│   └── beman
│       └── typeclass
│           ├── typeclass.hpp
│           └── ...
└── lib
    └── cmake
        └── beman.typeclass
            ├── beman.typeclass-config-version.cmake
            ├── beman.typeclass-config.cmake
            └── beman.typeclass-targets.cmake
```

### CMake Configuration

If you installed beman.typeclass to a prefix, you can specify that prefix to your CMake
project using `CMAKE_PREFIX_PATH`; for example, `-DCMAKE_PREFIX_PATH=/opt/beman`.

You need to bring in the `beman.typeclass` package to define the `beman::typeclass` CMake
target:

```cmake
find_package(beman.typeclass REQUIRED)
```

You will then need to add `beman::typeclass` to the link libraries of any libraries or
executables that include `beman.typeclass` headers.

```cmake
target_link_libraries(yourlib PUBLIC beman::typeclass)
```

### Using beman.typeclass

To use `beman.typeclass` in your C++ project,
include an appropriate `beman.typeclass` header from your source code.

```c++
#include <beman/typeclass/typeclass.hpp>
```
