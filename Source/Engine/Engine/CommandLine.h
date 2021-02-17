// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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

        /// <summary>
        /// -debug !ip:port! (Mono debugger address)
        /// </summary>
        Nullable<String> DebuggerAddress;

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
