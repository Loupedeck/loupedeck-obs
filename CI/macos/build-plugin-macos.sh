#!/bin/sh

echo "[loupedeck-obs] Building 'loupedeck-obs' for macOS."

LD_OBS_CONNECTOR_ROOT=..
SOURCE_PATH=$LD_OBS_CONNECTOR_ROOT/loupedeck-obs
BUILD_PATH=$SOURCE_PATH/build64
OBS_PATH=$LD_OBS_CONNECTOR_ROOT/obs-studio
#NOTE: obs-build-dependencies are there brought by OBS Studio build script
QT_TOOLCHAIN_FILE=$LD_OBS_CONNECTOR_ROOT/obs-build-dependencies/obs-deps/lib/cmake/Qt6/qt.toolchain.cmake
#CMAKE_PREFIX_PATH="$OBS_PATH/build/libobs;$OBS_PATH/build/UI/obs-frontend-api"


mkdir -p build && cd build
cmake .. \
	-Dobs-frontend-api_DIR=$OBS_PATH/build/UI/obs-frontend-api \
	-Dlibobs_DIR=$OBS_PATH/build/libobs \
	-DCMAKE_TOOLCHAIN_FILE=$QT_TOOLCHAIN_FILE \
	-DCMAKE_PREFIX_PATH=$CMAKE_PREFIX_PATH  \
	-DCMAKE_BUILD_TYPE=RelWithDebInfo \
	-DCMAKE_INSTALL_PREFIX=/Library/Application\ Support/obs-studio/plugins/ \
&& make -j4
