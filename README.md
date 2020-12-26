# Telesto

TODO

## Building tests

To build tests, first clone all submodules with:
```
$ git submodule update --init
```

Then, create the build directory with tests enabled using meson:
```
$ meson builddir -Dbuild_tests=true
```

And finally, compile and run the tests using ninja:
```
$ ninja -C builddir test
```
