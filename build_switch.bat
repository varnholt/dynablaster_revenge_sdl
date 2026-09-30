@echo off
setlocal
pushd "%~dp0"
docker run --rm -v "%CD%:/workspace" -w /workspace devkitpro/devkita64:20240827 bash docker/build_switch.sh
set BUILD_RESULT=%ERRORLEVEL%
popd
exit /b %BUILD_RESULT%
