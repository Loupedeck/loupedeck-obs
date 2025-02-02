mkdir build32
mkdir build64

cd build32
rem cmake -G "Visual Studio 16 2019" -A Win32 -DCMAKE_SYSTEM_VERSION=10.0 -DQTDIR="%QTDIR32%" -DLibObs_DIR="%OBS_PATH%\build32\libobs" -DLIBOBS_INCLUDE_DIR="%OBS_PATH%\libobs" -DLIBOBS_LIB="%OBS_PATH%\build32\libobs\%build_config%\obs.lib" -DOBS_FRONTEND_LIB="%OBS_PATH%\build32\UI\obs-frontend-api\%build_config%\obs-frontend-api.lib" ..
cd ..\build64
cmake -G "Visual Studio 16 2019" -A x64 -DCMAKE_SYSTEM_VERSION=10.0 -DQTDIR="%QTDIR64%" -DLibObs_DIR="%OBS_PATH%\build64\libobs" -DLIBOBS_INCLUDE_DIR="%OBS_PATH%\libobs" -DLIBOBS_LIB="%OBS_PATH%\build64\libobs\%build_config%\obs.lib" -DOBS_FRONTEND_LIB="%OBS_PATH%\build64\UI\obs-frontend-api\%build_config%\obs-frontend-api.lib" ..
cd ..