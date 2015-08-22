
common_exec()
{
    ARG_ONE=$1
    if [ "$ARG_ONE" == "gdb" ]; then
        shift
        # execute inside gdb.
        gdb --args $EXECPATH "$@" || exit 1
        exit 0
    fi

    if [ "$ARG_ONE" == "memcheck" ]; then
        shift
        # execute inside valgrind for memcheck
        valgrind --tool=memcheck --leak-check=full $EXECPATH "$@" || exit 1
        exit 0
    fi

    if [ "$ARG_ONE" == "callgrind" ]; then
        shift
        # execute inside valgrind for memcheck
        valgrind --tool=callgrind --simulate-cache=yes $EXECPATH "$@" || exit 1
        exit 0
    fi

    # normal execution
    exec $EXECPATH "$@" || exit 1
    exit 0
}

ixion_exec()
{
    EXEC="$1"
    shift
    EXECDIR="$1"
    shift
    PROGDIR="$PWD/"`dirname $0`
    ROOTDIR=$PROGDIR/..
    EXECPATH=$ROOTDIR/src/libixion/.libs/$EXEC
    export LD_LIBRARY_PATH=$PROGDIR/../src/libixion/.libs
    # For OSX.
    export DYLD_LIBRARY_PATH=$LD_LIBRARY_PATH
    cd $EXECDIR

    common_exec "$@"
}
