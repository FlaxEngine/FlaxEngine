@echo off

rem Copyright (c) Wojciech Figat. All rights reserved.

set MSBUILD_PATH=

rem Look for MSBuild version 17.0 or later
if not exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" goto VsWhereNotFound
for /f "delims=" %%i in ('"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere" -version 17.0 -latest -products * -requires Microsoft.Component.MSBuild -property installationPath') do (
	if exist "%%i\MSBuild\Current\Bin\MSBuild.exe" (
		set MSBUILD_PATH="%%i\MSBuild\Current\Bin\MSBuild.exe"
		goto End
	)
)

rem Look for MSBuild version 17.0 or later in pre-release versions
for /f "delims=" %%i in ('"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere" -version 17.0 -latest -prerelease -products * -requires Microsoft.Component.MSBuild -property installationPath') do (
	if exist "%%i\MSBuild\Current\Bin\MSBuild.exe" (
		set MSBUILD_PATH="%%i\MSBuild\Current\Bin\MSBuild.exe"
		goto End
	)
)
echo GetMSBuildPath ERROR: Could not find MSBuild version 17.0 or later.
exit /B 1
:VsWhereNotFound
echo GetMSBuildPath ERROR: vswhere.exe was not found.
exit /B 1
:End
exit /B 0