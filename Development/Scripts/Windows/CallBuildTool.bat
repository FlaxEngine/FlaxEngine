@echo off

rem Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

rem Make sure the batch file exists in the root folder.
if not exist "Development\Scripts\Windows\GetMSBuildPath.bat" goto Error_BatchFileInWrongLocation

rem Get the path to MSBuild executable.
call "Development\Scripts\Windows\GetMSBuildPath.bat"
if errorlevel 1 goto Error_NoVisualStudioEnvironment

rem If using VS2017, check that NuGet package manager is installed.
if not exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" goto NoVsWhere

set MSBUILD_15_EXE=
for /f "delims=" %%i in ('"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere" -latest -products * -requires Microsoft.Component.MSBuild -property installationPath') do (
	for %%j in (15.0, Current) do (
		if exist "%%i\MSBuild\%%j\Bin\MSBuild.exe" (
			set MSBUILD_15_EXE="%%i\MSBuild\%%j\Bin\MSBuild.exe"
			goto FoundMsBuild15
		)
	)
)
:FoundMsBuild15

set MSBUILD_15_EXE_WITH_NUGET=
for /f "delims=" %%i in ('"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere" -latest -products * -requires Microsoft.Component.MSBuild -requires Microsoft.VisualStudio.Component.NuGet -property installationPath') do (
	if exist "%%i\MSBuild\15.0\Bin\MSBuild.exe" (
		set MSBUILD_15_EXE_WITH_NUGET="%%i\MSBuild\15.0\Bin\MSBuild.exe"
		goto FoundMsBuild15WithNuget
	)
)
:FoundMsBuild15WithNuget

if not [%MSBUILD_15_EXE%] == [] (
	set MSBUILD_EXE=%MSBUILD_15_EXE%
	goto NoVsWhere
)

if not [%MSBUILD_EXE%] == [%MSBUILD_15_EXE_WITH_NUGET%] goto Error_RequireNugetPackageManager

:NoVsWhere

rem Check to see if the build tool source files have changed. Some can be included conditionally based on NDA access.
md Cache\Intermediate >nul 2>nul
dir /s /b Source\Tools\Flax.Build\*.cs >Cache\Intermediate\Flax.Build.Files.txt
fc /b Cache\Intermediate\Build\Flax.Build.Files.txt Cache\Intermediate\Build\Flax.Build.PrevFiles.txt >nul 2>nul
if not errorlevel 1 goto SkipClean
copy /y Cache\Intermediate\Build\Flax.Build.Files.txt Cache\Intermediate\Build\Flax.Build.PrevFiles.txt >nul
%MSBUILD_EXE% /nologo /verbosity:quiet Source\Tools\Flax.Build\Flax.Build.csproj /property:Configuration=Release /property:Platform=AnyCPU /target:Clean
:SkipClean
%MSBUILD_EXE% /nologo /verbosity:quiet Source\Tools\Flax.Build\Flax.Build.csproj /property:Configuration=Release /property:Platform=AnyCPU /target:Build
if errorlevel 1 goto Error_CompileFailed

rem Run the build tool using the provided arguments.
Binaries\Tools\Flax.Build.exe %*
if errorlevel 1 goto Error_FlaxBuildFailed

rem Done.
exit /B 0

:Error_BatchFileInWrongLocation
echo.
echo CallBuildTool ERROR: The batch file does not appear to be located in the root directory. This script must be run from within that directory.
echo.
goto Exit

:Error_NoVisualStudioEnvironment
echo.
echo CallBuildTool ERROR: We couldn't find a valid installation of Visual Studio. This program requires Visual Studio 2015. Please check that you have Visual Studio installed, then verify that the HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Microsoft\VisualStudio\14.0\InstallDir (or HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\VisualStudio\14.0\InstallDir on 32-bit machines) registry value is set.  Visual Studio configures this value when it is installed, and this program expects it to be set to the '\Common7\IDE\' sub-folder under a valid Visual Studio installation directory.
echo.
goto Exit

:Error_RequireNugetPackageManager
echo.
echo CallBuildTool ERROR: NuGet Package Manager is requried to be installed to use %MSBUILD_EXE%. Please run the Visual Studio Installer and add it from the individual components list (in the 'Code Tools' category).
echo.
goto Exit

:Error_CompileFailed
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
rem Exit with error.
exit /B 1
