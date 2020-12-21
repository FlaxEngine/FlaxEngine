@echo off

rem Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

if not exist "Development\Scripts\Windows\GetMSBuildPath.bat" goto Error_InvalidLocation
call "Development\Scripts\Windows\GetMSBuildPath.bat"
if errorlevel 1 goto Error_MissingVisualStudio

md Cache\Intermediate >nul 2>nul
dir /s /b Source\Tools\Flax.Build\*.cs >Cache\Intermediate\Flax.Build.Files.txt
fc /b Cache\Intermediate\Build\Flax.Build.Files.txt Cache\Intermediate\Build\Flax.Build.PrevFiles.txt >nul 2>nul
if not errorlevel 1 goto SkipClean
copy /y Cache\Intermediate\Build\Flax.Build.Files.txt Cache\Intermediate\Build\Flax.Build.PrevFiles.txt >nul
%MSBUILD_PATH% /nologo /verbosity:quiet Source\Tools\Flax.Build\Flax.Build.csproj /property:Configuration=Release /property:Platform=AnyCPU /target:Clean
:SkipClean
%MSBUILD_PATH% /nologo /verbosity:quiet Source\Tools\Flax.Build\Flax.Build.csproj /property:Configuration=Release /property:Platform=AnyCPU /target:Build
if errorlevel 1 goto Error_CompilationFailed

Binaries\Tools\Flax.Build.exe %*
if errorlevel 1 goto Error_FlaxBuildFailed
exit /B 0

:Error_InvalidLocation
echo.
echo CallBuildTool ERROR: The batch file is in a wrong directory.
echo.
goto Exit
:Error_MissingVisualStudio
echo.
echo CallBuildTool ERROR: Missing Visual Studio 2015 or newer.
echo.
goto Exit
:Error_CompilationFailed
echo.
echo CallBuildTool ERROR: Failed to compile Flax.Build project.
echo.
goto Exit
:Error_FlaxBuildFailed
echo.
echo CallBuildTool ERROR: Flax.Build tool failed.
echo.
goto Exit
:Exit
exit /B 1
