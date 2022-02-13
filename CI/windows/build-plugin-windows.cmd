echo **************************************************
echo Building Loupedeck connector
echo please run me from the root directory of this repo
echo Assumption is you have run download-obs-deps.cmd, install-qt-win.cmd and prepare-obs-windows.cmd  scripts
echo If you did not, please exit at this stage, otherwise please continue
echo OBS binaries also need to be build prior to that for the same buildconfig

call CI\windows\settings.cmd
pause

call CI\windows\prepare-plugin-windows.cmd 

rem cmake --build build32 --config %build_config%
cmake --build build64 --config %build_config%