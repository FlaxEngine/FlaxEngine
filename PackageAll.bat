@echo off

rem Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

setlocal
pushd
echo Performing the full package...

rem Run the build tool.
call "Development\Scripts\Windows\CallBuildTool.bat" -deploy -deployEditor -deployPlatforms -dotnet=8 -verbose -log -logFile="Cache\Intermediate\PackageLog.txt" %*
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
