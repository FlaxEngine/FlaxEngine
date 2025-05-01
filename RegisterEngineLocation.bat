@echo off

rem Copyright (c) Wojciech Figat. All rights reserved.

setlocal
pushd %~dp0
echo Registering Flax Engine project files...

rem Check the current versions config
if not exist "%appdata%\Flax\Versions.txt" (
	echo The installed engine versions file is missing. Please ensure that Flax Launcher was installed.
	pause
	goto Exit
)
set EngineLocation=%cd%
find /c "%EngineLocation%" "%appdata%\Flax\Versions.txt"
if %errorlevel% equ 1 goto NotFound
echo Already registered.
goto Done

rem Register the location (append to the end)
:NotFound
echo Location '%EngineLocation%' is not registered. Adding it to the list of engine versions.
echo %EngineLocation%>>"%appdata%\Flax\Versions.txt"
goto Done

rem Done.
:Done
popd
echo Done!
exit /B 0

:Exit
popd
exit /B 1
