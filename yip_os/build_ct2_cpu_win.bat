@echo off
REM ===================================================================
REM  build_ct2_cpu_win.bat -- Build CTranslate2 (CPU-only) for Windows
REM  Run from a VS x64 Native Tools Command Prompt.
REM
REM  This builds a CPU-only variant of CTranslate2 and installs it to
REM  C:\ct2_install_cpu.  The standard (non-CUDA) installer uses this.
REM  The CUDA build at C:\ct2_install is separate and unchanged.
REM
REM  The CT2 source is expected at C:\ct2_build\CTranslate2 (same repo
REM  used for the CUDA build).
REM ===================================================================

setlocal enabledelayedexpansion

REM --- VS environment ---
if defined VSCMD_VER goto :skip_vcvars
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
:skip_vcvars

REM --- Configuration ---
set "CT2_SRC=C:\ct2_build\CTranslate2"
set "CT2_CPU_PREFIX=C:\ct2_install_cpu"
set "CT2_CPU_BUILD=%CT2_SRC%\build_cpu"

echo.
echo ============================================================
echo  CTranslate2 CPU-only build script
echo  Source:  %CT2_SRC%
echo  Build:   %CT2_CPU_BUILD%
echo  Install: %CT2_CPU_PREFIX%
echo ============================================================
echo.

REM --- Check source exists ---
if not exist "%CT2_SRC%\CMakeLists.txt" (
    echo ERROR: CTranslate2 source not found at %CT2_SRC%
    echo        Clone it first: git clone https://github.com/OpenNMT/CTranslate2.git %CT2_SRC%
    exit /b 1
)

REM --- vcpkg (optional, matches main build) ---
set "VCPKG_CMAKE="
if not defined VCPKG_ROOT goto :skip_vcpkg
if not exist "%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" goto :skip_vcpkg
set VCPKG_CMAKE=-DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake"
echo vcpkg found at %VCPKG_ROOT%
:skip_vcpkg

REM --- Configure (CPU-only) ---
echo.
echo --- Configuring CMake (CPU-only, Release) ---
cmake -B "%CT2_CPU_BUILD%" -S "%CT2_SRC%" -G "Ninja" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_INSTALL_PREFIX="%CT2_CPU_PREFIX%" ^
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 ^
    -DWITH_CUDA=OFF ^
    -DWITH_MKL=OFF ^
    -DWITH_OPENBLAS=OFF ^
    -DOPENMP_RUNTIME=NONE ^
    %VCPKG_CMAKE%
if errorlevel 1 (
    echo ERROR: CMake configure failed.
    exit /b 1
)

REM --- Build ---
echo.
echo --- Building ---
cmake --build "%CT2_CPU_BUILD%"
if errorlevel 1 (
    echo ERROR: Build failed.
    exit /b 1
)

REM --- Install ---
echo.
echo --- Installing to %CT2_CPU_PREFIX% ---
cmake --install "%CT2_CPU_BUILD%"
if errorlevel 1 (
    echo ERROR: Install failed.
    exit /b 1
)

REM --- Verify ---
echo.
echo ============================================================
if exist "%CT2_CPU_PREFIX%\bin\ctranslate2.dll" (
    if exist "%CT2_CPU_PREFIX%\lib\cmake\ctranslate2" (
        echo  SUCCESS: CTranslate2 (CPU-only) installed to %CT2_CPU_PREFIX%
        echo.
        echo  dll:    %CT2_CPU_PREFIX%\bin\ctranslate2.dll
        echo  cmake:  %CT2_CPU_PREFIX%\lib\cmake\ctranslate2
        echo.
        echo  build_installer.bat should now find it for the standard installer.
    ) else (
        echo  WARNING: cmake config not found in lib\cmake\ctranslate2
    )
) else (
    echo  WARNING: ctranslate2.dll not found in bin\
)
echo ============================================================

endlocal
