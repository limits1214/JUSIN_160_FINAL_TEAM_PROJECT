::@echo off

echo ***Vars
set "TargetFolder=Client"

echo Config=%~1
echo TargetFolder=%TargetFolder%
echo ***

call .\CopyCommon.bat "%~1" "%TargetFolder%"

if /I "%~1"=="Debug" (
	copy /Y ".\Client\lua_dkm_debug.json" ".\Client\Bin\lua_dkm_debug.json" >nul
	if exist ".\vcpkg_installed\x64-windows\x64-windows\debug\bin\lua.pdb" (
		copy /Y ".\vcpkg_installed\x64-windows\x64-windows\debug\bin\lua.pdb" ".\Client\Bin\lua.pdb" >nul
	)
)
