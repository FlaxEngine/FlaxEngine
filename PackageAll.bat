@echo off

rem Copyright (c) Wojciech Figat. All rights reserved.

setlocal
pushd
echo Performing the full package...

rem Run the build tool.
call "Development\Scripts\Windows\CallBuildTool.bat" -deploy -deployEditor -deployPlatforms -verbose -log -logFile="Cache\Intermediate\PackageLog.txt" %*
if errorlevel 1 goto BuildToolFailed

popd
echo Done!
exit /B 0

:BuildToolFailed
echo Flax.Build tool failed.
goto Exit

:Exit
popd
exit /B 1
