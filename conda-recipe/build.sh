
##
## START THE BUILD
##

mkdir -p build
cd build
##
## Configure
##
cmake .. \
        -DBOOST_ROOT=${PREFIX} \
        -DBUILD_NIFTY_PYTHON=ON \
        -DWITH_BLOSC=ON \
        -DWITH_ZLIB=ON \
        -DWITH_BZIP2=OFF \
        -DWITH_Z5=ON \
        -DWITH_HDF5=OFF \
\
        -DCMAKE_SHARED_LINKER_FLAGS="${LDFLAGS}" \
        -DCMAKE_EXE_LINKER_FLAGS="${LDFLAGS}" \
        -DCMAKE_CXX_FLAGS="${CXXFLAGS} -O3 -DNDEBUG -std=c++17" \
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
