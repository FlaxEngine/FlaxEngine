@echo off

rem Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

if not exist "Development\Scripts\Windows\GetMSBuildPath.bat" goto Error_InvalidLocation

for %%I in (Source\Logo.png) do if %%~zI LSS 2000 (
	goto Error_MissingLFS
)

call "Development\Scripts\Windows\GetMSBuildPath.bat"
if errorlevel 1 goto Error_NoVisualStudioEnvironment

if not exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" goto Compile
for /f "delims=" %%i in ('"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere" -latest -products * -requires Microsoft.Component.MSBuild -property installationPath') do (
	for %%j in (15.0, Current) do (
		if exist "%%i\MSBuild\%%j\Bin\MSBuild.exe" (
			set MSBUILD_PATH="%%i\MSBuild\%%j\Bin\MSBuild.exe"
			goto Compile
		)
	)
)

:Compile
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

:Error_MissingLFS
echo CallBuildTool ERROR: Repository was not cloned using Git LFS
goto Exit
:Error_InvalidLocation
echo CallBuildTool ERROR: The script is in invalid directory.
goto Exit
:Error_NoVisualStudioEnvironment
echo CallBuildTool ERROR: Missing Visual Studio 2015 or newer.
goto Exit
:Error_CompilationFailed
echo CallBuildTool ERROR: Failed to compile Flax.Build project.
goto Exit
:Error_FlaxBuildFailed
echo CallBuildTool ERROR: Flax.Build tool failed.
goto Exit
:Exit
exit /B 1
