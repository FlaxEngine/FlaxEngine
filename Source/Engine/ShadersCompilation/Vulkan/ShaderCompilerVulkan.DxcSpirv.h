// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if COMPILE_WITH_VK_SHADER_COMPILER && COMPILE_WITH_VK_DXC_SPIRV

/// <summary>
/// Returns true if the shader source uses inline ray queries (HLSL RayQuery).
/// </summary>
bool ShaderCompilerVulkan_UsesRayQuery(const char* source, int32 length);

#endif
