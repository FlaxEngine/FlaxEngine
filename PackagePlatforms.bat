@echo off

rem Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

setlocal
pushd
echo Building and packaging platforms data...

rem Make sure the batch file exists in the root folder.
if not exist "%~dp0\Source" goto Error_BatchFileInWrongLocation
if not exist "Development\Scripts\Windows\CallBuildTool.bat" goto Error_BatchFileInWrongLocation

rem Run Flax.Build to generate Visual Studio solution and project files (also pass the arguments)
call "Development\Scripts\Windows\CallBuildTool.bat" -deploy -deployPlatforms -verbose -log -logFile="Cache\Intermediate\PackageLog.txt" %*
if errorlevel 1 goto Error_BuildToolFailed

rem Done.
popd
echo Done!
exit /B 0

:Error_BatchFileInWrongLocation
echo.
echo The batch file does not appear to be located in the root directory. This script must be run from within that directory.
echo.
goto Exit

:Error_BuildToolFailed
echo.
echo Flax.Build tool failed.
echo.
goto Exit

:Exit
rem Restore original directory before exit.
popd
exit /B 1
