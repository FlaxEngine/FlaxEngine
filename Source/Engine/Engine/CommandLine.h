// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/String.h"
#include "Engine/Core/Types/Nullable.h"

/// <summary>
/// Command line options helper.
/// </summary>
class CommandLine
{
public:

    struct OptionsData
    {
        /// <summary>
        /// The command line.
        /// </summary>
        const Char* CmdLine = nullptr;

        /// <summary>
        /// -widowed (use windowed mode)
        /// </summary>
        Nullable<bool> Windowed;

        /// <summary>
        /// -fullscreen (use fullscreen)
        /// </summary>
        Nullable<bool> Fullscreen;

        /// <summary>
        /// -vsync (enable vertical synchronization)
        /// </summary>
        Nullable<bool> VSync;

        /// <summary>
        /// -novsync (disable vertical synchronization)
        /// </summary>
        Nullable<bool> NoVSync;

        /// <summary>
        /// -nolog (disable output log file)
        /// </summary>
        Nullable<bool> NoLog;

        /// <summary>
        /// -std (redirect log to standard output)
        /// </summary>
        Nullable<bool> Std;

#if !BUILD_RELEASE

        /// <summary>
        /// -debug !ip:port! (Mono debugger address)
        /// </summary>
        Nullable<String> DebuggerAddress;

        /// <summary>
        /// -debugwait (instructs Mono debugger to wait for client attach for 5 seconds)
        /// </summary>
        Nullable<bool> WaitForDebugger;

#endif

#if PLATFORM_HAS_HEADLESS_MODE

        /// <summary>
        /// -headless (Run without windows, used by CL)
        /// </summary>
        Nullable<bool> Headless;

#endif

        /// <summary>
        /// -d3d12 (forces to use DirectX 12 rendering backend if available)
        /// </summary>
        Nullable<bool> D3D12;

        /// <summary>
        /// -d3d11 (forces to use DirectX 11 rendering backend if available)
        /// </summary>
        Nullable<bool> D3D11;

        /// <summary>
        /// -d3d10 (forces to use DirectX 10 rendering backend if available)
        /// </summary>
        Nullable<bool> D3D10;

        /// <summary>
        /// -null (forces to use Null rendering backend if available)
        /// </summary>
        Nullable<bool> Null;

        /// <summary>
        /// -vulkan (forces to use Vulkan rendering backend if available)
        /// </summary>
        Nullable<bool> Vulkan;

        /// <summary>
        /// -nvidia (hints to use NVIDIA GPU if available)
        /// </summary>
        Nullable<bool> NVIDIA;

        /// <summary>
        /// -amd (hints to use AMD GPU if available)
        /// </summary>
        Nullable<bool> AMD;

        /// <summary>
        /// -intel (hints to use Intel GPU if available)
        /// </summary>
        Nullable<bool> Intel;

        /// <summary>
        /// -monolog (enables advanced debugging for Mono runtime)
        /// </summary>
        Nullable<bool> MonoLog;

        /// <summary>
        /// -mute (disables audio playback and uses Null Audio Backend)
        /// </summary>
        Nullable<bool> Mute;

        /// <summary>
        /// -lowdpi (disables High DPI awareness support)
        /// </summary>
        Nullable<bool> LowDPI;

#if USE_EDITOR
        /// <summary>
        /// -project !path! (Startup project path)
        /// </summary>
        String Project;

        /// <summary>
        /// -new (generates the project files inside the specified project folder or uses current workspace folder)
        /// </summary>
        Nullable<bool> NewProject;

        /// <summary>
        /// -genprojectfiles (generates the scripts project files)
        /// </summary>
        Nullable<bool> GenProjectFiles;

        /// <summary>
        /// -clearcache (clear project cache folder)
        /// </summary>
        Nullable<bool> ClearCache;

        /// <summary>
        /// -clearcooker (clear Game Cooker cache folder)
        /// </summary>
        Nullable<bool> ClearCookerCache;

        /// <summary>
        /// The build preset (-build !preset.target! or -build !preset!) (run game building on startup and exit app on end).
        /// </summary>
        Nullable<String> Build;

        /// <summary>
        /// -skipcompile (skips the scripts compilation on editor startup, useful when launching engine from IDE)
        /// </summary>
        Nullable<bool> SkipCompile;

        /// <summary>
        /// -shaderdebug (enables debugging data generation for shaders and disables shader compiler optimizations)
        /// </summary>
        Nullable<bool> ShaderDebug;

        /// <summary>
        /// -exit (exits the editor after startup and performing all queued actions). Usefull when invoking editor from CL/CD.
        /// </summary>
        Nullable<bool> Exit;

        /// <summary>
        /// -play !guid! ( Scene to play, can be empty to use default )
        /// </summary>
        Nullable<String> Play;
#endif

#if USE_EDITOR || !BUILD_RELEASE
        /// <summary>
        /// -shaderprofile (enables debugging data generation for shaders but leaves shader compiler optimizations active for performance profiling)
        /// </summary>
        Nullable<bool> ShaderProfile;
#endif
    };

    /// <summary>
    /// The input options.
    /// </summary>
    static OptionsData Options;

public:

    /// <summary>
    /// Parses the input command line.
    /// </summary>
    /// <param name="cmdLine">The command line.</param>
    /// <returns>True if failed, otherwise false.</returns>
    static bool Parse(const Char* cmdLine);
};
