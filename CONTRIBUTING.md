
# Windows

## Building boost

First, open your MINGW64 shell.  You can simply use the MINGW64 shell that
comes shipped with the Windows version of Git.  Once you are in it, change
directory to the root of the Boost library directory, and run the following
command:

```bash
./bootstrap.bat
./b2 --stagedir=./stage/x64 address-model=64 link=static --build-type=complete -j 8
```

in order to build Boost as static libraries.  You may want to change the part
`-j 8` which controls the number of concurrent processes to use for your build.
Note that if you build Boost as dynamic libraries, make sure that all the
relevant dll files are in your PATH.


## Clone spdlog and mdds

As ixion uses [spdlog](https://github.com/gabime/spdlog) as its logging facility,
you need to clone its repository before building ixion.

Likewise, you also need to clone [mdds](https://gitlab.com/mdds/mdds).  As both
mdds and spdlog are header-only libraries, you don't need to go through any
build process for these libraries.  Just make note of their respective header
directory locations.


## Using CMake to build ixion

Run the following series of commands to configure your build.

```bash
mkdir build
cd build
cmake .. \
    -DCMAKE_INSTALL_PREFIX="/path/to/install" \
    -DBOOST_INCLUDEDIR="/path/to/boost" \
    -DBOOST_LIBRARYDIR="/path/to/boost/stage/x64/lib" \
    -DMDDS_INCLUDEDIR="/path/to/mdds/include" \
    -DSPDLOG_INCLUDEDIR="/path/to/spdlog/include" \
    -DBoost_USE_STATIC_LIBS=1
```

You may skip the last option `-DBoost_USE_STATIC_LIBS=1` if you have built
Boost as dynamic libraries.

Once the configuration is finished, start the build by running:

```bash
cmake --build . --config Release
```

You may choose a different generator name than what is shown in the above
example to suit your need.

The ixion build process also requires Python 3 interpreter.  In case you have
trouble getting python3 detected, try specifying the path to your Python 3
installation via `Python3_ROOT_DIR` option.

To install ixion to the location specified via `CMAKE_INSTALL_PREFIX`, simply run:

```bash
cmake --build . --config Release --target install
```

# Linux

## Using autotools to build ixion

You need to use GNU Autotools to build ixion on Linux.  The process follows a
standard autotools workflow, which basically involves the following steps:

```bash
./autogen.sh
make check
make install
```

