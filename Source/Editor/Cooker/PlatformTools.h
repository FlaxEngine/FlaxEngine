// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#pragma once

#include "CookingData.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Editor/Scripting/ScriptsBuilder.h"
#include "Engine/Graphics/PixelFormat.h"

class TextureBase;

/// <summary>
/// The platform support tools base interface.
/// </summary>
class FLAXENGINE_API PlatformTools
{
public:

    /// <summary>
    /// Finalizes an instance of the <see cref="PlatformTools"/> class.
    /// </summary>
    virtual ~PlatformTools() = default;

    /// <summary>
    /// Gets the name of the platform for UI and logging.
    /// </summary>
    /// <returns>The name.</returns>
    virtual const Char* GetDisplayName() const = 0;

    /// <summary>
    /// Gets the name of the platform for filesystem cache directories, deps folder.
    /// </summary>
    /// <returns>The name.</returns>
    virtual const Char* GetName() const = 0;

    /// <summary>
    /// Gets the type of the platform.
    /// </summary>
    /// <returns>The platform type.</returns>
    virtual PlatformType GetPlatform() const = 0;

    /// <summary>
    /// Gets the architecture of the platform.
    /// </summary>
    /// <returns>The architecture type.</returns>
    virtual ArchitectureType GetArchitecture() const = 0;

    /// <summary>
    /// Gets the value indicating whenever platform requires AOT.
    /// </summary>
    /// <returns>True if platform uses AOT and needs C# assemblies to be precompiled, otherwise false.</returns>
    virtual bool UseAOT() const
    {
        return false;
    }

    /// <summary>
    /// Gets the texture format that is supported by the platform for a given texture.
    /// </summary>
    /// <param name="data">The cooking data.</param>
    /// <param name="texture">The texture.</param>
    /// <param name="format">The texture format.</param>
    /// <returns>The target texture format for the platform.</returns>
    virtual PixelFormat GetTextureFormat(CookingData& data, TextureBase* texture, PixelFormat format)
    {
        return format;
    }

    /// <summary>
    /// Checks if the given file is a native code.
    /// </summary>
    /// <param name="data">The cooking data.</param>
    /// <param name="file">The file path.</param>
    /// <returns>True if it's a native file, otherwise false.<returns>
    virtual bool IsNativeCodeFile(CookingData& data, const String& file)
    {
        return false;
    }

public:

    /// <summary>
    /// Called when game building starts.
    /// </summary>
    /// <param name="data">The cooking data.</param>
    virtual void OnBuildStarted(CookingData& data)
    {
    }

    /// <summary>
    /// Called when game building ends.
    /// </summary>
    /// <param name="data">The cooking data.</param>
    /// <param name="failed">True if build failed, otherwise false.</param>
    virtual void OnBuildEnded(CookingData& data, bool failed)
    {
    }

    /// <summary>
    /// Called before scripts compilation. Can be used to inject custom configuration or prepare data.
    /// </summary>
    /// <param name="data">The cooking data.</param>
    /// <returns>True if failed, otherwise false.<returns>
    virtual bool OnScriptsCompilationStart(CookingData& data)
    {
        return false;
    }

    /// <summary>
    /// Called after scripts compilation. Can be used to cleanup or prepare data.
    /// </summary>
    /// <param name="data">The cooking data.</param>
    /// <returns>True if failed, otherwise false.<returns>
    virtual bool OnScriptsCompilationEnd(CookingData& data)
    {
        return false;
    }

    /// <summary>
    /// Called after compiled scripts deploy. Can be used to override or patch the output files.
    /// </summary>
    /// <param name="data">The cooking data.</param>
    /// <returns>True if failed, otherwise false.<returns>
    virtual bool OnScriptsStepDone(CookingData& data)
    {
        return false;
    }

    /// <summary>
    /// Called during binaries deployment.
    /// </summary>
    /// <param name="data">The cooking data.</param>
    /// <returns>True if failed, otherwise false.<returns>
    virtual bool OnDeployBinaries(CookingData& data)
    {
        return false;
    }

    /// <summary>
    /// The C# scripts AOT configuration options.
    /// </summary>
    struct AotConfig
    {
        String AotCompilerPath;
        String AotCompilerArgs;
        String AssemblerPath;
        String AssemblerArgs;
        String ArchiverPath;
        String ArchiverArgs;
        String AuxToolPath;
        String AuxToolArgs;
        String AotCachePath;
        Dictionary<String, String> EnvVars;
        Array<String> AssembliesSearchDirs;
        Array<String> Assemblies;

        AotConfig(CookingData& data)
        {
            Platform::GetEnvironmentVariables(EnvVars);
            EnvVars[TEXT("MONO_PATH")] = data.DataOutputPath / TEXT("Mono/lib/mono/4.5");
            AssembliesSearchDirs.Add(data.DataOutputPath / TEXT("Mono/lib/mono/4.5"));
        }
    };

    /// <summary>
    /// Called to configure AOT options.
    /// </summary>
    /// <param name="data">The cooking data.</param>
    /// <param name="config">The configuration.</param>
    virtual void OnConfigureAOT(CookingData& data, AotConfig& config)
    {
    }

    /// <summary>
    /// Called to execute AOT for the given assembly.
    /// </summary>
    /// <param name="data">The cooking data.</param>
    /// <param name="config">The configuration.</param>
    /// <param name="assemblyPath">The input managed library file path.</param>
    /// <returns>True if failed, otherwise false.<returns>
    virtual bool OnPerformAOT(CookingData& data, AotConfig& config, const String& assemblyPath)
    {
        return false;
    }

    /// <summary>
    /// Called after AOT execution for the assemblies.
    /// </summary>
    /// <param name="data">The cooking data.</param>
    /// <param name="config">The configuration.</param>
    /// <returns>True if failed, otherwise false.<returns>
    virtual bool OnPostProcessAOT(CookingData& data, AotConfig& config)
    {
        return false;
    }

    /// <summary>
    /// Called during staged build post-processing.
    /// </summary>
    /// <param name="data">The cooking data.</param>
    /// <returns>True if failed, otherwise false.<returns>
    virtual bool OnPostProcess(CookingData& data)
    {
        return false;
    }

    /// <summary>
    /// Called to run the cooked game build on device.
    /// </summary>
    /// <param name="data">The cooking data.</param>
    /// <param name="executableFile">The game executable file path to run (or tool path to run if build should run on remote device). Empty if not supported.</param>
    /// <param name="commandLineFormat">The command line for executable file. Use `{0}` to insert custom command line for passing to the cooked game.</param>
    /// <param name="workingDir">Overriden custom working directory to use. Leave empty if use cooked data output folder.</param>
    virtual void OnRun(CookingData& data, String& executableFile, String& commandLineFormat, String& workingDir)
    {
    }
};
