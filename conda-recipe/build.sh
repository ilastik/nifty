
##
## START THE BUILD
##

mkdir -p build
cd build
##
## Configure
##

# Platform dependent flags. With z5py 2.0.5 there were problems building on mac and win
if [[ `uname` == 'Darwin' ]];
then
    EXTRA_CMAKE_ARGS="DWITH_BLOSC=OFF"
    EXTRA_CMAKE_ARGS="${EXTRA_CMAKE_ARGS} -DWITH_ZLIB=OFF"
    EXTRA_CMAKE_ARGS="${EXTRA_CMAKE_ARGS} -DWITH_Z5=OFF"
    EXTRA_CXX_ARGS="-std=c++14"
else
    EXTRA_CMAKE_ARGS="DWITH_BLOSC=ON"
    EXTRA_CMAKE_ARGS="${EXTRA_CMAKE_ARGS} -DWITH_ZLIB=ON"
    EXTRA_CMAKE_ARGS="${EXTRA_CMAKE_ARGS} -DWITH_Z5=ON"
    EXTRA_CXX_ARGS="-std=c++17"
fi


cmake .. \
        -DBOOST_ROOT=${PREFIX} \
        -DBUILD_NIFTY_PYTHON=ON \
        -DWITH_BZIP2=OFF \
        -DWITH_HDF5=OFF \
        -DWITH_Z5=OFF \
\
        -DCMAKE_SHARED_LINKER_FLAGS="${LDFLAGS}" \
        -DCMAKE_EXE_LINKER_FLAGS="${LDFLAGS}" \
        -DCMAKE_CXX_FLAGS="${CXXFLAGS} -O3 -DNDEBUG ${EXTRA_CXX_ARGS}" \
\
        -DCMAKE_PREFIX_PATH=${PREFIX} \
        -DCMAKE_INSTALL_PREFIX=${PREFIX} \
        -DBUILD_NIFTY_PYTHON=ON \
        -DPYTHON_EXECUTABLE=${PYTHON} \
        -DPYTHON_LIBRARY=${PREFIX}/lib/libpython${CONDA_PY}${SHLIB_EXT} \
        -DPYTHON_INCLUDE_DIR=${PREFIX}/include/python${CONDA_PY} \

##
## Compile and install
##
make -j${CPU_COUNT}
make install
