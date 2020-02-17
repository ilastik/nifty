mkdir build
cd build

set CONFIGURATION=Release

cmake .. -G "NMake Makefiles" ^
    -DCMAKE_PREFIX_PATH="%LIBRARY_PREFIX%" ^
    -DCMAKE_BUILD_TYPE=%CONFIGURATION% ^
    -DCMAKE_INSTALL_PREFIX="%LIBRARY_PREFIX%"  ^
    -DBOOST_ROOT="%LIBRARY%" ^
    -DCMAKE_CXX_FLAGS="-DBOOST_ALL_NO_LIB /EHsc" ^
    ^
    %OPTIMIZER_ARGS% ^
    ^
    -DPYTHON_EXECUTABLE=%PYTHON% ^
    -DBUILD_NIFTY_PYTHON=yes ^
    -DBUILD_CPP_TEST=no ^
    -DBUILD_CPP_EXAMPLES=no ^
    -DWITH_GLPK=no ^
    -DWITH_HDF5=no ^
    -DWITH_Z5=yes ^
    -DWITH_LP_MP=no ^
    -DWITH_QPBO=no

nmake all
if errorlevel 1 exit 1
nmake install
if errorlevel 1 exit 1
