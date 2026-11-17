# Testing the installed packages

A downstream project that finds each library with `find_package` and uses it.
Nothing here reaches into the source tree: the test for `beman.tree_algorithms`
also checks that its package config finds `beman.transpose` on its own, and
the `beman.fingertree` test runs only where that package was installed (it
needs `std::inplace_vector` at C++26). The optional
`beman.typeclass_tree_adapters` test runs on the same condition; it links that
one target and checks that the package finds both libraries it joins.

From the repository root, with the toolchain the install was built with:

```sh
make TOOLCHAIN=gcc-16 testinstall
```

That installs into `.install/`, configures this directory against it, builds,
and runs a test for each installed package. To test another prefix:

```sh
cmake -S installtest -B /tmp/installtest -DCMAKE_PREFIX_PATH=/opt/beman.package
cmake --build /tmp/installtest
ctest --test-dir /tmp/installtest --output-on-failure
```
