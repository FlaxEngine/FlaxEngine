@echo off

rem Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

set MSBUILD_PATH=

if not exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" goto VsWhereNotFound
for /f "delims=" %%i in ('"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere" -latest -products * -requires Microsoft.Component.MSBuild -property installationPath') do (
	if exist "%%i\MSBuild\15.0\Bin\MSBuild.exe" (
		set MSBUILD_PATH="%%i\MSBuild\15.0\Bin\MSBuild.exe"
		goto End
	)
)
for /f "delims=" %%i in ('"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere" -latest -prerelease -products * -requires Microsoft.Component.MSBuild -property installationPath') do (
	if exist "%%i\MSBuild\15.0\Bin\MSBuild.exe" (
		set MSBUILD_PATH="%%i\MSBuild\15.0\Bin\MSBuild.exe"
		goto End
	)
	if exist "%%i\MSBuild\Current\Bin\MSBuild.exe" (
		set MSBUILD_PATH="%%i\MSBuild\Current\Bin\MSBuild.exe"
		goto End
	)
)
:VsWhereNotFound

if exist "%ProgramFiles(x86)%\MSBuild\14.0\bin\MSBuild.exe" (
	set MSBUILD_PATH="%ProgramFiles(x86)%\MSBuild\14.0\bin\MSBuild.exe"
	goto End
)

call :GetInstallPath Microsoft\VisualStudio\SxS\VS7 15.0 MSBuild\15.0\bin\MSBuild.exe
if not errorlevel 1 goto End
call :GetInstallPath Microsoft\MSBuild\ToolsVersions\14.0 MSBuildToolsPath MSBuild.exe
if not errorlevel 1 goto End
call :GetInstallPath Microsoft\MSBuild\ToolsVersions\12.0 MSBuildToolsPath MSBuild.exe
if not errorlevel 1 goto End
call :GetInstallPath Microsoft\MSBuild\ToolsVersions\4.0 MSBuildToolsPath MSBuild.exe
if not errorlevel 1 goto End

exit /B 1
:End
exit /B 0

:GetInstallPath
for /f "tokens=2,*" %%A in ('REG.exe query HKCU\SOFTWARE\%1 /v %2 2^>Nul') do (
	if exist "%%B%%3" (
		set MSBUILD_PATH="%%B%3"
		exit /B 0
	)
)
for /f "tokens=2,*" %%A in ('REG.exe query HKLM\SOFTWARE\%1 /v %2 2^>Nul') do (
	if exist "%%B%3" (
		set MSBUILD_PATH="%%B%3"
		exit /B 0
	)
)
for /f "tokens=2,*" %%A in ('REG.exe query HKCU\SOFTWARE\Wow6432Node\%1 /v %2 2^>Nul') do (
	if exist "%%B%%3" (
		set MSBUILD_PATH="%%B%3"
		exit /B 0
	)
)
for /f "tokens=2,*" %%A in ('REG.exe query HKLM\SOFTWARE\Wow6432Node\%1 /v %2 2^>Nul') do (
	if exist "%%B%3" (
		set MSBUILD_PATH="%%B%3"
		exit /B 0
	)
)
exit /B 1
