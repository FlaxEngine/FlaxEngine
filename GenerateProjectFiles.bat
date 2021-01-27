@echo off

rem Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

setlocal
pushd
echo Generating Flax Engine project files...

rem Run Flax.Build to generate Visual Studio solution and project files (also pass the arguments)
call "Development\Scripts\Windows\CallBuildTool.bat" -genproject %*
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
