@echo off

rem Copyright (c) Wojciech Figat. All rights reserved.

if not exist "Development\Scripts\Windows\GetMSBuildPath.bat" goto Error_InvalidLocation

for %%I in (Source\Logo.png) do if %%~zI LSS 2000 (
	goto Error_MissingLFS
)

call "Development\Scripts\Windows\GetMSBuildPath.bat"
if errorlevel 1 goto Error_NoVisualStudioEnvironment

:Compile
md Cache\Intermediate >nul 2>nul
dir /s /b Source\Tools\Flax.Build\*.cs >Cache\Intermediate\Flax.Build.Files.txt
fc /b Cache\Intermediate\Build\Flax.Build.Files.txt Cache\Intermediate\Build\Flax.Build.PrevFiles.txt >nul 2>nul
if not errorlevel 1 goto SkipClean

copy /y Cache\Intermediate\Build\Flax.Build.Files.txt Cache\Intermediate\Build\Flax.Build.PrevFiles.txt >nul
%MSBUILD_PATH% /nologo /verbosity:quiet Source\Tools\Flax.Build\Flax.Build.csproj /property:Configuration=Release /target:Restore,Clean /property:RestorePackagesConfig=True /p:RuntimeIdentifiers=win-x64
:SkipClean
%MSBUILD_PATH% /nologo /verbosity:quiet Source\Tools\Flax.Build\Flax.Build.csproj /property:Configuration=Release /target:Build /property:SelfContained=False /property:RuntimeIdentifiers=win-x64
if errorlevel 1 goto Error_CompilationFailed

Binaries\Tools\Flax.Build.exe %*
if errorlevel 1 goto Error_FlaxBuildFailed
exit /B 0

:Error_MissingLFS
echo CallBuildTool ERROR: Repository was not cloned using Git LFS
goto Exit
:Error_InvalidLocation
echo CallBuildTool ERROR: The script is in invalid directory.
goto Exit
:Error_NoVisualStudioEnvironment
echo CallBuildTool ERROR: Missing Visual Studio 2022 or newer.
goto Exit
:Error_CompilationFailed
echo CallBuildTool ERROR: Failed to compile Flax.Build project.
goto Exit
:Error_FlaxBuildFailed
echo CallBuildTool ERROR: Flax.Build tool failed.
goto Exit
:Exit
exit /B 1
