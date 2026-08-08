::@echo off

:: Common Settings
set "TargetFolder=%~2"

:: Engine Copy
xcopy /E /I /Y /D .\Engine\public\*.* .\EngineSDK\Inc\
xcopy /E /I /Y /D .\Engine\Bin\Engine.dll .\%TargetFolder%\Bin\
xcopy /E /I /Y /D .\Engine\Bin\Engine.lib .\EngineSDK\Lib\

:: Shader Copy
xcopy /E /I /Y /D .\%TargetFolder%\ShaderFiles\*.* .\%TargetFolder%\Bin\ShaderFiles\
xcopy /E /I /Y /D .\Engine\ShaderFiles\*.* .\%TargetFolder%\Bin\ShaderFiles\

:: Lua Copy
xcopy /E /I /Y /D .\%TargetFolder%\LuaFiles\*.* .\%TargetFolder%\Bin\LuaFiles\
xcopy /E /I /Y /D .\Engine\LuaFiles\*.* .\%TargetFolder%\Bin\LuaFiles\

:: ThirdParty
xcopy /E /I /Y /D .\ThirdParty\fmod_2_03_12\lib\x64\fmod.dll .\%TargetFolder%\Bin\
xcopy /E /I /Y /D .\ThirdParty\HBAOPlus-3.1.0\lib\GFSDK_SSAO_D3D11.win64.dll .\%TargetFolder%\Bin\

:: Blast 5.0.6
xcopy /Y /D ".\ThirdParty\blast-5.0.6\release\blast-sdk\bin\NvBlast*.dll" ".\%TargetFolder%\Bin\"

if /I "%~1"=="Release" (
    xcopy /E /I /Y /D .\vcpkg_installed\x64-windows\x64-windows\bin\*.dll .\%TargetFolder%\Bin\
    xcopy /E /I /Y .\ThirdParty\physx-5.6.1\bin_release\*.dll .\%TargetFolder%\Bin\
    xcopy /E /I /Y .\ThirdParty\NvCloth_1.1.6\bin\Release\NvCloth_x64.dll .\%TargetFolder%\Bin\
) else (
    xcopy /E /I /Y /D .\vcpkg_installed\x64-windows\x64-windows\debug\bin\*.dll .\%TargetFolder%\Bin\
    xcopy /E /I /Y .\ThirdParty\physx-5.6.1\bin_debug\*.dll .\%TargetFolder%\Bin\
    xcopy /E /I /Y .\ThirdParty\NvCloth_1.1.6\bin\Release\NvCloth_x64.dll .\%TargetFolder%\Bin\
)

exit /b
