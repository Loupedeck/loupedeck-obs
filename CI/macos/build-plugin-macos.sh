#!/bin/sh

echo "[loupedeck-obs] Building 'loupedeck-obs' for macOS."

LD_OBS_CONNECTOR_ROOT=`realpath ..`
SOURCE_PATH=$LD_OBS_CONNECTOR_ROOT/loupedeck-obs
BUILD_PATH=$SOURCE_PATH/build
INSTALL_PATH=$BUILD_PATH/install
OBS_PATH=$LD_OBS_CONNECTOR_ROOT/obs-studio
#NOTE: obs-build-dependencies are there brought by OBS Studio build script
QT_TOOLCHAIN_FILE=$LD_OBS_CONNECTOR_ROOT/obs-build-dependencies/obs-deps/lib/cmake/Qt6/qt.toolchain.cmake
#CMAKE_PREFIX_PATH="$OBS_PATH/build/libobs;$OBS_PATH/build/UI/obs-frontend-api"


mkdir -p $BUILD_PATH && cd $BUILD_PATH
cmake $SOURCE_PATH \
	-Dobs-frontend-api_DIR=$OBS_PATH/build/UI/obs-frontend-api \
	-Dlibobs_DIR=$OBS_PATH/build/libobs \
	-DCMAKE_TOOLCHAIN_FILE=$QT_TOOLCHAIN_FILE \
	-DCMAKE_PREFIX_PATH=$CMAKE_PREFIX_PATH  \
	-DCMAKE_BUILD_TYPE=RelWithDebInfo \
	-DCMAKE_INSTALL_PREFIX=$INSTALL_PATH \
&& make -j4

echo *** Now preparing binary  
make install

echo If all went well, the plugin is ready in  $INSTALL_PATH/loupedeck-obs-compat.plugin/Contents/MacOS/loupedeck-obs.so

ls  $INSTALL_PATH/loupedeck-obs-compat.plugin/Contents/MacOS/loupedeck-obs.so

echo bye!



