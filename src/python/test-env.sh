
PYTHONPATH=.libs:$PYTHONPATH

# Ensure that the libixion shared library built as part of the current build
# is discovered first.
LD_LIBRARY_PATH=../libixion/.libs:$LD_LIBRARY_PATH

export PYTHONPATH LD_LIBRARY_PATH

