@echo off

rem Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

set MSBUILD_EXE=

rem Try to get the MSBuild 15 path using vswhere (see https://github.com/Microsoft/vswhere).
if not exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" goto no_vswhere
for /f "delims=" %%i in ('"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere" -latest -products * -requires Microsoft.Component.MSBuild -property installationPath') do (
	if exist "%%i\MSBuild\15.0\Bin\MSBuild.exe" (
		set MSBUILD_EXE="%%i\MSBuild\15.0\Bin\MSBuild.exe"
		goto Succeeded
	)
)
:no_vswhere

rem Check for MSBuild 15. This is installed alongside Visual Studio 2017, so we get the path relative to that.
call :ReadInstallPath Microsoft\VisualStudio\SxS\VS7 15.0 MSBuild\15.0\bin\MSBuild.exe
if not errorlevel 1 goto Succeeded

rem Try to get the MSBuild 14.0 path directly (see https://msdn.microsoft.com/en-us/library/hh162058(v=vs.120).aspx).
if exist "%ProgramFiles(x86)%\MSBuild\14.0\bin\MSBuild.exe" (
	set MSBUILD_EXE="%ProgramFiles(x86)%\MSBuild\14.0\bin\MSBuild.exe"
	goto Succeeded
)

rem Try to get the MSBuild 14.0 path from the registry.
call :ReadInstallPath Microsoft\MSBuild\ToolsVersions\14.0 MSBuildToolsPath MSBuild.exe
if not errorlevel 1 goto Succeeded

rem Check for older versions of MSBuild. These are registered as separate versions in the registry.
call :ReadInstallPath Microsoft\MSBuild\ToolsVersions\12.0 MSBuildToolsPath MSBuild.exe
if not errorlevel 1 goto Succeeded
call :ReadInstallPath Microsoft\MSBuild\ToolsVersions\4.0 MSBuildToolsPath MSBuild.exe
if not errorlevel 1 goto Succeeded

rem Searching failed.
exit /B 1

rem Searching done!
:Succeeded
exit /B 0

rem Subroutine to query the registry under HKCU/HKLM Win32/Wow64 software registry keys for a certain install directory.
rem Arguments: 
rem      %1 = Registry path under the 'SOFTWARE' registry key
rem      %2 = Value name
rem      %3 = Relative path under this directory to look for an installed executable.
:ReadInstallPath
for /f "tokens=2,*" %%A in ('REG.exe query HKCU\SOFTWARE\%1 /v %2 2^>Nul') do (
	if exist "%%B%%3" (
		set MSBUILD_EXE="%%B%3"
		exit /B 0
	)
)
for /f "tokens=2,*" %%A in ('REG.exe query HKLM\SOFTWARE\%1 /v %2 2^>Nul') do (
	if exist "%%B%3" (
		set MSBUILD_EXE="%%B%3"
		exit /B 0
	)
)
for /f "tokens=2,*" %%A in ('REG.exe query HKCU\SOFTWARE\Wow6432Node\%1 /v %2 2^>Nul') do (
	if exist "%%B%%3" (
		set MSBUILD_EXE="%%B%3"
		exit /B 0
	)
)
for /f "tokens=2,*" %%A in ('REG.exe query HKLM\SOFTWARE\Wow6432Node\%1 /v %2 2^>Nul') do (
	if exist "%%B%3" (
		set MSBUILD_EXE="%%B%3"
		exit /B 0
	)
)
exit /B 1
