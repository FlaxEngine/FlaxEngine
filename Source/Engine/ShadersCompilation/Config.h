// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/String.h"
#include "Engine/Core/Types/Guid.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Graphics/Shaders/Config.h"

class MemoryWriteStream;

/// <summary>
/// Shader compilation options container
/// </summary>
struct FLAXENGINE_API ShaderCompilationOptions
{
public:

    /// <summary>
    /// Name of the target object (name of the shader or material for better logging readability)
    /// </summary>
    String TargetName;

    /// <summary>
    /// Unique ID of the target object
    /// </summary>
    Guid TargetID = Guid::Empty;

    /// <summary>
    /// Shader source code (null terminated)
    /// </summary>
    const char* Source = nullptr;

    /// <summary>
    /// Shader source code length
    /// </summary>
    uint32 SourceLength = 0;

public:

    /// <summary>
    /// Target shader profile
    /// </summary>
    ShaderProfile Profile = ShaderProfile::Unknown;

    /// <summary>
    /// Disables shaders compiler optimizations. Can be used to debug shaders on a target platform or to speed up the shaders compilation time.
    /// </summary>
    bool NoOptimize = false;

    /// <summary>
    /// Enables shader debug data generation (depends on the target platform rendering backend).
    /// </summary>
    bool GenerateDebugData = false;

    /// <summary>
    /// Enable/disable promoting warnings to compilation errors
    /// </summary>
    bool TreatWarningsAsErrors = false;

    /// <summary>
    /// Custom macros for the shader compilation
    /// </summary>
    Array<ShaderMacro> Macros;

public:

    /// <summary>
    /// Output stream to write compiled shader cache to
    /// </summary>
    MemoryWriteStream* Output = nullptr;
};
