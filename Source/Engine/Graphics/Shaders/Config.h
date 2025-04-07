// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Config.h"
#include "Engine/Core/Enums.h"
#include "Engine/Graphics/Enums.h"

// [Deprecated in v1.10]
#define INPUT_LAYOUT_ELEMENT_ALIGN 0xffffffff
// [Deprecated in v1.10]
#define INPUT_LAYOUT_ELEMENT_PER_VERTEX_DATA 0
// [Deprecated in v1.10]
#define INPUT_LAYOUT_ELEMENT_PER_INSTANCE_DATA 1

/// <summary>
/// Maximum amount of input elements for vertex shader programs
/// [Deprecated in v1.10]
/// </summary>
#define VERTEX_SHADER_MAX_INPUT_ELEMENTS 16

/// <summary>
/// Maximum allowed amount of shader program permutations
/// </summary>
#define SHADER_PERMUTATIONS_MAX_COUNT 16

/// <summary>
/// Maximum allowed amount of parameters for permutation
/// </summary>
#define SHADER_PERMUTATIONS_MAX_PARAMS_COUNT 4

/// <summary>
/// Maximum allowed amount of constant buffers that can be binded to the pipeline (maximum slot index is MAX_CONSTANT_BUFFER_SLOTS-1)
/// [Deprecated in v1.10]
/// </summary>
#define MAX_CONSTANT_BUFFER_SLOTS 4

/// <summary>
/// Shader program type
/// </summary>
DECLARE_ENUM_EX_6(ShaderStage, int32, 0, Vertex, Hull, Domain, Geometry, Pixel, Compute);

/// <summary>
/// Shader macro definition structure
/// </summary>
struct ShaderMacro
{
    /// <summary>
    /// Macro name
    /// </summary>
    const char* Name;

    /// <summary>
    /// Macro value
    /// </summary>
    const char* Definition;
};
