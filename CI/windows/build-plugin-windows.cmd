@echo off
echo **************************************************
echo Building Loupedeck connector for Windows
echo please run me from the root directory of this repo
echo Assumption is you have run download-obs-deps.cmd, install-qt-win.cmd and prepare-obs-windows.cmd  scripts
echo If you did not, please exit at this stage, otherwise please continue
pause

set LD_OBS_CONNECTOR_ROOT=..
set SOURCE_PATH=%LD_OBS_CONNECTOR_ROOT%\loupedeck-obs
set BUILD_PATH=%SOURCE_PATH%\build64
set OBS_PATH=%LD_OBS_CONNECTOR_ROOT%\obs-studio
rem NOTE: obs-build-dependencies are there brought by OBS Studio build script
set QT_TOOLCHAIN_FILE=%LD_OBS_CONNECTOR_ROOT%\obs-build-dependencies\windows-deps-2022-08-02-x64\lib\cmake\Qt6\qt.toolchain.cmake
set CMAKE_PREFIX_PATH=%OBS_PATH%\build64\libobs\;%OBS_PATH%\build64\deps\w32-pthreads\;%OBS_PATH%\build64\UI\obs-frontend-api\
set CMAKE_CXX_FLAGS="-D_WIN32_WINNT=_WIN32_WINNT_WIN10"

echo EXPECTING OBS STUDIO BUILT in %OBS_PATH%
echo Note: I Need Visual Studio 17 2022 to run
echo connectir be built in %BUILD_PATH%
set build_config=RelWithDebInfo

cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_CXX_FLAGS=%CMAKE_CXX_FLAGS% -DCMAKE_SYSTEM_VERSION=10.0 -DCMAKE_TOOLCHAIN_FILE=%QT_TOOLCHAIN_FILE% -DCMAKE_PREFIX_PATH=%CMAKE_PREFIX_PATH%  -S %SOURCE_PATH% -B %BUILD_PATH%
echo *** Now building ***
cmake --build %BUILD_PATH% --config %build_config% 

