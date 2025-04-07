@echo off
:: Copyright (c) Wojciech Figat. All rights reserved.

setlocal
pushd

echo Generating Flax Engine project files...

:: Change the path to the script root
cd /D "%~dp0"

:: Run Flax.Build to generate Visual Studio solution and project files (also pass the arguments)
call "Development\Scripts\Windows\CallBuildTool.bat" -genproject  %*
if errorlevel 1 goto BuildToolFailed

:: Build bindings for all editor configurations
echo Building C# bindings...
Binaries\Tools\Flax.Build.exe -build -BuildBindingsOnly -arch=x64 -platform=Windows --buildTargets=FlaxEditor

popd
echo Done!
exit /B 0

:BuildToolFailed
echo Flax.Build tool failed.
pause
goto Exit

:Exit
popd
exit /B 1
