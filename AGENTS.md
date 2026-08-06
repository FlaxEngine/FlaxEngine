# AGENTS.md

## Repo Purpose

Flax Engine is a modern 3D game engine written in C++ and C#.
This repository contains the engine, editor, tooling, shaders, tests, assets, and platform-specific sources, excluding NDA-protected platform support.

## High-Level Structure

- `Source/Engine/`: engine runtime code and managed/runtime integration.
- `Source/Editor/`: editor code in both C++ and C#.
- `Source/Tools/`: build system and developer tooling, including `Flax.Build`.
- `Source/Platforms/`: platform-specific code, dependencies, and binaries.
- `Source/ThirdParty/`: vendored third-party code. Avoid changes here unless the task explicitly requires it.
- `Source/Engine/Tests/`: native and managed engine tests.
- `Source/Tools/Flax.Build.Tests/`: .NET tests for the build tool.
- `Content/`: engine/editor assets.
- `Development/Scripts/`: helper scripts for project generation and builds.

## Working Assumptions

- Generated solutions and project files are not committed. On a clean clone, generate them first.
- Git LFS is required. The Windows build scripts explicitly check for LFS-populated files.
- On Windows, the main entry points are `GenerateProjectFiles.bat` and `Development\Scripts\Windows\CallBuildTool.bat`.
- Prefer edits in `Source/Engine`, `Source/Editor`, `Source/Tools`, or docs. Do not modify `Binaries/`, `Cache/`, or generated project files unless the task explicitly targets generated output.

## Windows Setup And Build

Use these commands from the repo root.

Generate project files:

```powershell
.\GenerateProjectFiles.bat -vs2022 -log -verbose -printSDKs -dotnet=8
```

Alternative default generation:

```powershell
.\GenerateProjectFiles.bat
```

Build the editor target:

```powershell
.\Development\Scripts\Windows\CallBuildTool.bat -build -log -dotnet=8 -arch=x64 -platform=Windows -configuration=Development -buildtargets=FlaxEditor
```

Run the editor:

```powershell
.\Binaries\Editor\Win64\Development\FlaxEditor.exe
```

Visual Studio workflow after generation:

- Open `Flax.sln`.
- Use solution configuration `Editor.Development` and platform `Win64`.
- Set `Flax` or `FlaxEngine` as the startup project.

## Tests And Validation

The checked-in CI workflow in `.github/workflows/tests.yml` is the most reliable source for current validation commands.

Build native tests:

```powershell
.\Development\Scripts\Windows\CallBuildTool.bat -build -log -dotnet=8 -arch=x64 -platform=Windows -configuration=Development -buildtargets=FlaxTestsTarget
```

Run native tests:

```powershell
.\Binaries\Editor\Win64\Development\FlaxTests.exe -headless
```

Build Flax.Build tests:

```powershell
dotnet msbuild Source\Tools\Flax.Build.Tests\Flax.Build.Tests.csproj /m /t:Restore,Build /p:Configuration=Debug /p:Platform=AnyCPU /nologo
```

Run Flax.Build tests:

```powershell
dotnet test -f net8.0 Binaries\Tests\Flax.Build.Tests.dll
```

Run managed engine tests after copying runtime dependencies:

```powershell
xcopy /y Binaries\Editor\Win64\Development\FlaxEngine.CSharp.dll Binaries\Tests
xcopy /y Binaries\Editor\Win64\Development\FlaxEngine.CSharp.runtimeconfig.json Binaries\Tests
xcopy /y Binaries\Editor\Win64\Development\Newtonsoft.Json.dll Binaries\Tests
dotnet test -f net8.0 Binaries\Tests\FlaxEngine.CSharp.dll
```

If a change is localized, prefer the narrowest possible target build and only run the relevant tests for that area.

## Style And Conventions

- The root `Source/.editorconfig` sets CRLF line endings, UTF-8, final newline, and spaces for indentation.
- Use 4 spaces for C++, C#, shaders, and Python. Use 2 spaces for XAML and MSBuild files.
- Keep the existing copyright header in source files.
- Public APIs in both C++ headers and C# commonly use XML-style documentation comments such as `/// <summary>`.
- Preserve local file style instead of mass-normalizing. The C# codebase mixes block namespaces and file-scoped namespaces.
- Follow existing naming patterns: PascalCase for types and public members; private C# fields often use `_camelCase`.
- In C++ headers, prefer forward declarations where practical and keep includes minimal.

## Build System Notes

- `FlaxEditor` is the main standalone editor target.
- `FlaxGame` is the standalone game target.
- `FlaxTestsTarget` builds the native test executable.
- `Flax.Build` supports project generation switches such as `-vs2022`, `-vs2026`, `-vscode`, and `-rider`.
- `GenerateProjectFiles.bat` also builds C# bindings for `FlaxEditor` on Windows.

## Agent Guidance

- Treat `.github/workflows/tests.yml` as the source of truth for CI-backed validation.
- Do not assume generated artifacts already exist in the repo.
- Avoid broad style-only rewrites.
- Avoid touching `Source/ThirdParty/` unless explicitly requested.
- If a change affects developer workflow or build/test steps, update the relevant root docs as part of the task.
- No dedicated repo-wide formatter or linter command is checked in. Prefer build and test validation over inventing formatting steps.
- PVS-Studio is mentioned in `README.md`, but it is not wired here as a standard local validation command.
