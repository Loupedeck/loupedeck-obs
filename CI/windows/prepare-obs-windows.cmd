@echo off
SETLOCAL EnableDelayedExpansion

REM If obs-studio directory does not exist, clone the git repo
if not exist %OBS_PATH% (
    echo obs-studio directory does not exist
    git clone https://github.com/obsproject/obs-studio %OBS_PATH%
    cd /D %OBS_PATH%\
    git describe --tags --abbrev=0 > "%OBS_PATH%\obs-studio-latest-tag.txt"
    set /p OBS_LATEST_TAG=<"%OBS_PATH%\obs-studio-latest-tag.txt"

)
REM Prepare OBS Studio builds

set OBS_LATEST_TAG="27.1.3"
echo Running CMake...,  obs path is %OBS_PATH%, using tag %OBS_LATEST_TAG%
cd /D %OBS_PATH%
echo   git checkout %OBS_LATEST_TAG%
git checkout %OBS_LATEST_TAG%
echo:

if not exist build32 mkdir build32
if not exist build64 mkdir build64

rem echo   Running cmake for obs-studio %OBS_LATEST_TAG% 32-bit...
cd build32
rem cmake -G "Visual Studio 16 2019" -A Win32 -DCMAKE_SYSTEM_VERSION=10.0 -DQTDIR="%QTDIR32%" -DDepsPath="%DEPS_PATH_32%" -DDISABLE_PLUGINS=true -DCOPIED_DEPENDENCIES=false -DCOPY_DEPENDENCIES=true ..
rem echo:
rem echo:

echo   Running cmake for obs-studio %OBS_LATEST_TAG% 64-bit...
cd ..\build64
cmake -G "Visual Studio 16 2019" -A x64 -DCMAKE_SYSTEM_VERSION=10.0 -DQTDIR="%QTDIR64%" -DDepsPath="%DEPS_PATH_64%" -DDISABLE_PLUGINS=true -DCOPIED_DEPENDENCIES=false -DCOPY_DEPENDENCIES=true ..
echo:
echo:

cd ..
echo Building OBS
rem cmake --build build32 --config %build_config%
cmake --build build64 --config %build_config%

