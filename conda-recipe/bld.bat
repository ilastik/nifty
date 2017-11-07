mkdir build
cd build

REM ----------------------------------------------------------------------
IF NOT DEFINED WITH_CPLEX (SET WITH_CPLEX=0)
IF NOT DEFINED WITH_GUROBI (SET WITH_GUROBI=0)
IF %WITH_CPLEX% == "" (SET WITH_CPLEX=0)
IF %WITH_GUROBI% == "" (SET WITH_GUROBI=0)

SET OPTIMIZER_ARGS=
IF "%WITH_GUROBI%" == "1" (
    IF NOT DEFINED GUROBI_ROOT_DIR (
        ECHO "GUROBI_ROOT_DIR must be set for building!"
        exit 1
    )
    IF "%GUROBI_ROOT_DIR%" == "" (
        ECHO "GUROBI_ROOT_DIR cannot be empty for building!"
        exit 1
    ) ELSE (
        ECHO "Using GUROBI_ROOT_DIR=%GUROBI_ROOT_DIR%"
    )
    REM if we build with Gurobi, we need to configure the paths.
    REM MHT chooses gurobi first if that is configured.
    REM The GUROBI_ROOT_DIR should point to gurobiXYZ\win64
    dir "%GUROBI_ROOT_DIR%\lib\gurobi*.lib" /s/b | findstr gurobi[0-9][0-9].lib > gurobilib.tmp
    set /p GUROBI_LIB=<gurobilib.tmp
    ECHO found gurobi lib %GUROBI_LIB%
    SET OPTIMIZER_ARGS=-DWITH_GUROBI=ON -DGUROBI_ROOT_DIR=%GUROBI_ROOT_DIR% ^
      -DGUROBI_LIBRARY=%GUROBI_LIB% -DGUROBI_INCLUDE_DIR=%GUROBI_ROOT_DIR%\include ^
      -DGUROBI_CXX_LIBRARY=%GUROBI_ROOT_DIR%\lib\gurobi_c++md2015.lib
) 

IF "%WITH_CPLEX" == "1" (
    REM CPLEX is found automatically if installed. 
    REM No idea what happens with two CPLEXinstallations, but for now we don't care.
    SET OPTIMIZER_ARGS="-DWITH_CPLEX=ON"
)

REM ----------------------------------------------------------------------

set CONFIGURATION=Release

cmake .. -G "%CMAKE_GENERATOR%" -DCMAKE_PREFIX_PATH="%LIBRARY_PREFIX%" ^
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
    -DWITH_HDF5=yes ^
    -DWITH_LP_MP=no ^
    -DWITH_QPBO=no

cmake --build . --target ALL_BUILD --config %CONFIGURATION%
if errorlevel 1 exit 1
cmake --build . --target INSTALL --config %CONFIGURATION%
if errorlevel 1 exit 1