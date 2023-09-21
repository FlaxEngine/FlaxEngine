@echo off

rem Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

for %%I in (Source\Logo.png) do if %%~zI LSS 2000 (
	goto Error_MissingLFS
)

where >nul 2>nul dotnet
if %errorlevel% neq 0 goto Error_MissingDotNet

FOR /F "delims=" %%i IN ('dotnet --version') DO echo Using dotnet %%i
if errorlevel 1 goto Error_MissingDotNet

:Compile
md Cache\Intermediate >nul 2>nul
dir /s /b Source\Tools\Flax.Build\*.cs >Cache\Intermediate\Flax.Build.Files.txt
fc /b Cache\Intermediate\Build\Flax.Build.Files.txt Cache\Intermediate\Build\Flax.Build.PrevFiles.txt >nul 2>nul
if not errorlevel 1 goto SkipClean

copy /y Cache\Intermediate\Build\Flax.Build.Files.txt Cache\Intermediate\Build\Flax.Build.PrevFiles.txt >nul

echo Performing Clean/Restore
dotnet build --nologo --verbosity quiet Source\Tools\Flax.Build\Flax.Build.csproj --configuration Release /target:Restore,Clean /property:RestorePackagesConfig=True --arch x64
:SkipClean

echo Performing Build
dotnet build --nologo --verbosity quiet Source\Tools\Flax.Build\Flax.Build.csproj --configuration Release /target:Build --no-self-contained --arch x64
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
echo CallBuildTool ERROR: Missing Visual Studio 2015 or newer.
goto Exit
:Error_MissingDotNet
echo CallBuildTool ERROR: .NET SDK not found. Please install the .NET SDK using the link below and rerun the script
echo https://dotnet.microsoft.com/en-us/download/dotnet/7.0
goto Exit
:Error_CompilationFailed
echo CallBuildTool ERROR: Failed to compile Flax.Build project.
goto Exit
:Error_FlaxBuildFailed
echo CallBuildTool ERROR: Flax.Build tool failed.
goto Exit
:Exit
exit /B 1
