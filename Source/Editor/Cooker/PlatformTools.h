// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "CookingData.h"
#include "Editor/Scripting/ScriptsBuilder.h"
#include "Engine/Graphics/PixelFormat.h"

class TextureBase;

/// <summary>
/// The game cooker cache interface.
/// </summary>
class FLAXENGINE_API IBuildCache
{
public:
    /// <summary>
    /// Removes all cached entries for assets that contain a given asset type. This forces rebuild for them.
    /// </summary>
    virtual void InvalidateCachePerType(const StringView& typeName) = 0;

    /// <summary>
    /// Removes all cached entries for assets that contain a given asset type. This forces rebuild for them.
    /// </summary>
    template<typename T>
    FORCE_INLINE void InvalidateCachePerType()
    {
        InvalidateCachePerType(T::TypeName);
    }

    /// <summary>
    /// Removes all cached entries for assets that contain a shader. This forces rebuild for them.
    /// </summary>
    void InvalidateCacheShaders();

    /// <summary>
    /// Removes all cached entries for assets that contain a texture. This forces rebuild for them.
    /// </summary>
    void InvalidateCacheTextures();
};

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
    virtual const Char* GetDisplayName() const = 0;

    /// <summary>
    /// Gets the name of the platform for filesystem cache directories, deps folder.
    /// </summary>
    virtual const Char* GetName() const = 0;

    /// <summary>
    /// Gets the type of the platform.
    /// </summary>
    virtual PlatformType GetPlatform() const = 0;

    /// <summary>
    /// Gets the architecture of the platform.
    /// </summary>
    virtual ArchitectureType GetArchitecture() const = 0;

    /// <summary>
    /// Gets the value indicating whenever platform requires AOT (needs C# assemblies to be precompiled).
    /// </summary>
    virtual DotNetAOTModes UseAOT() const
    {
        return DotNetAOTModes::None;
    }

    /// <summary>
    /// Gets the value indicating whenever platform supports using system-installed .Net Runtime.
    /// </summary>
    virtual bool UseSystemDotnet() const
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
    virtual bool IsNativeCodeFile(CookingData& data, const String& file);

public:
    /// <summary>
    /// Loads the build cache. Allows to invalidate any cached asset types based on the build settings for incremental builds (eg. invalidate textures/shaders).
    /// </summary>
    /// <param name="data">The cooking data.</param>
    /// <param name="data">The build cache interface.</param>
    /// <param name="data">The loaded cache data. Can be empty when starting a fresh build.</param>
    virtual void LoadCache(CookingData& data, IBuildCache* cache, const Span<byte>& bytes)
    {
    }

    /// <summary>
    /// Saves the build cache. Allows to store any build settings to be used for cache invalidation on incremental builds.
    /// </summary>
    /// <param name="data">The cooking data.</param>
    /// <param name="data">The build cache interface.</param>
    /// <returns>Data to cache, will be restored during next incremental build.<returns>
    virtual Array<byte> SaveCache(CookingData& data, IBuildCache* cache)
    {
        return Array<byte>();
    }

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
