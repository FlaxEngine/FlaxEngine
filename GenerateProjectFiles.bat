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

set "BindingsDotnetArg="
:FindDotnetArg
if "%~1"=="" goto BuildBindings
set "Arg=%~1"

if /i "%Arg:~0,8%"=="-dotnet=" set "BindingsDotnetArg=%Arg%"
if /i "%Arg%"=="-dotnet" (
    set "BindingsDotnetArg=-dotnet=%~2"
    shift /1
)

shift /1
goto FindDotnetArg

:BuildBindings
:: Build bindings for all editor configurations
echo Building C# bindings...
Binaries\Tools\Flax.Build.exe -build -BuildBindingsOnly -arch=x64 -platform=Windows --buildTargets=FlaxEditor %BindingsDotnetArg%
if errorlevel 1 goto BuildToolFailed

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
