
# Windows

## Building boost

First, open your MINGW64 shell.  You can simply use the MINGW64 shell that
comes shipped with the Windows version of Git.  Once you are in it, change
directory to the root of the boost library directory, and run the following
command:

```bash
./bootstrap.bat
mkdir -p stage/x64
./b2 --stagedir=./stage/x64 address-model=64 --build-type=complete -j 8
```

where you may change the part `-j 8` which controls how many concurrent
processes to use for the build.

## Using CMake to build ixion

While at the root of the source directory, run the following commands:

```bash
cmake -G "Visual Studio 15 2017 Win64" -H. -Bbuild \
    -DBOOST_INCLUDE_DIR="/path/to/boost" \
    -DBOOST_LIB_DIR="/path/to/boost/lib" \
    -DMDDS_INCLUDE_DIR="/path/to/mdds/include"
cmake --build build
```

This will create a `build` directory along with a whole bunch of build-related
files.  The final executables are found in `build/Debug`.

You may choose a different generator name than what is shown in the above
example.

# Linux

## Using autotools to build the test binaries

You need to use GNU Autotools to build ixion on Linux.  The process follows a
standard autotools workflow, which basically includes the following steps:

```bash
./autogen.sh
make check
make install
```

