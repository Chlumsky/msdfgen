@echo off
setlocal
pushd "%~dp0"

if "%VCVARSALL%"=="" set "VCVARSALL=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
if "%VCTOOLSET%"=="" set "VCTOOLSET=VC143"

if not exist "%VCVARSALL%" (
    echo vcvarsall.bat not found, set VCVARSALL to its path
    exit /b 1
)

if "%1"=="" (
    echo No version specified, using current
    set "version=current"
) else (
    set "version=%1"
)
set "builddir=%~dp0\build\release-%version%"

if exist "%builddir%" (
    echo Deleting contents of %builddir%
    rd /s /q "%builddir%"
)

cmake --preset win32 -B "%builddir%\build-win32"
cmake --preset win32-omp -B "%builddir%\build-win32-omp"
cmake --preset win64 -B "%builddir%\build-win64"
cmake --preset win64-omp -B "%builddir%\build-win64-omp"

cmake --build "%builddir%\build-win32" --config Release
cmake --build "%builddir%\build-win32-omp" --config Release
cmake --build "%builddir%\build-win64" --config Release
cmake --build "%builddir%\build-win64-omp" --config Release

mkdir "%builddir%\rel-win32\msdfgen"
mkdir "%builddir%\rel-win32-omp\msdfgen"
mkdir "%builddir%\rel-win64\msdfgen"
mkdir "%builddir%\rel-win64-omp\msdfgen"

copy "%builddir%\build-win32\Release\msdfgen.exe" "%builddir%\rel-win32\msdfgen"
copy "%builddir%\build-win32-omp\Release\msdfgen.exe" "%builddir%\rel-win32-omp\msdfgen"
copy "%builddir%\build-win64\Release\msdfgen.exe" "%builddir%\rel-win64\msdfgen"
copy "%builddir%\build-win64-omp\Release\msdfgen.exe" "%builddir%\rel-win64-omp\msdfgen"

echo msdfgen.exe -defineshape "{ 1471,0; 1149,0; 1021,333; 435,333; 314,0; 0,0; 571,1466; 884,1466; # }{ 926,580; 724,1124; 526,580; # }" -size 16 16 -autoframe -testrender render.png 1024 1024 > "%builddir%\example.bat"

copy "%builddir%\example.bat" "%builddir%\rel-win32\msdfgen"
copy "%builddir%\example.bat" "%builddir%\rel-win32-omp\msdfgen"
copy "%builddir%\example.bat" "%builddir%\rel-win64\msdfgen"
copy "%builddir%\example.bat" "%builddir%\rel-win64-omp\msdfgen"

call "%VCVARSALL%" x64

set "omp32dll=%VCToolsRedistDir%\x86\Microsoft.%VCTOOLSET%.OPENMP\vcomp140.dll"
set "omp64dll=%VCToolsRedistDir%\x64\Microsoft.%VCTOOLSET%.OPENMP\vcomp140.dll"

if not exist "%omp32dll%" (
    echo vcomp140.dll [x86] not found, make sure to set VCTOOLSET or update this script
    exit /b 1
)
if not exist "%omp64dll%" (
    echo vcomp140.dll [x64] not found, make sure to set VCTOOLSET or update this script
    exit /b 1
)

copy "%omp32dll%" "%builddir%\rel-win32-omp\msdfgen"
copy "%omp64dll%" "%builddir%\rel-win64-omp\msdfgen"

if not exist "C:\Program Files\7-Zip\7z.exe" (
    echo 7-Zip not found, you have to package it manually
    exit /b 0
)

pushd "%builddir%\rel-win32"
"C:\Program Files\7-Zip\7z.exe" a "..\msdfgen-%version%-win32.zip" msdfgen
cd msdfgen
call example.bat
popd
pushd "%builddir%\rel-win32-omp"
"C:\Program Files\7-Zip\7z.exe" a "..\msdfgen-%version%-win32-openmp.zip" msdfgen
cd msdfgen
call example.bat
popd
pushd "%builddir%\rel-win64"
"C:\Program Files\7-Zip\7z.exe" a "..\msdfgen-%version%-win64.zip" msdfgen
cd msdfgen
call example.bat
popd
pushd "%builddir%\rel-win64-omp"
"C:\Program Files\7-Zip\7z.exe" a "..\msdfgen-%version%-win64-openmp.zip" msdfgen
cd msdfgen
call example.bat
popd

popd
