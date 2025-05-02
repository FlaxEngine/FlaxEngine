// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Graphics/Config.h"
#include "Engine/Graphics/Materials/MaterialInfo.h"
#include "../Config.h"

class MemoryReadStream;

// Shader file data mapping to asset chunks (allows to support shader precompiled for multiple rendering backends)
#define SHADER_FILE_CHUNK_MATERIAL_PARAMS 0
#define SHADER_FILE_CHUNK_INTERNAL_D3D_SM5_CACHE 1
#define SHADER_FILE_CHUNK_INTERNAL_D3D_SM4_CACHE 2
#define SHADER_FILE_CHUNK_INTERNAL_GLSL_410_CACHE 3
#define SHADER_FILE_CHUNK_INTERNAL_GLSL_440_CACHE 4
#define SHADER_FILE_CHUNK_INTERNAL_VULKAN_SM5_CACHE 5
#define SHADER_FILE_CHUNK_INTERNAL_GENERIC_CACHE 6
#define SHADER_FILE_CHUNK_INTERNAL_D3D_SM6_CACHE 7
#define SHADER_FILE_CHUNK_VISJECT_SURFACE 14
#define SHADER_FILE_CHUNK_SOURCE 15

/// <summary>
/// Contains shader data that is used during creating shaders/materials
/// </summary>
class ShaderStorage
{
public:
    /// <summary>
    /// Different shader cache storage modes (disabled, inside asset and in project cache)
    /// </summary>
    enum class CachingMode
    {
        Disabled = 0,
        AssetInternal,
        ProjectCache
    };

    /// <summary>
    /// Current shaders caching mode to use
    /// </summary>
    static CachingMode CurrentCachingMode;

    /// <summary>
    /// Gets caching mode to use for shaders
    /// </summary>
    /// <returns>Caching mode</returns>
    static CachingMode GetCachingMode();

public:
    /// <summary>
    /// Packed version of the Magic Code for shader files
    /// </summary>
    static const int32 MagicCode;

public:
    /// <summary>
    /// File header, version 18
    /// [Deprecated on 24.07.2019, expires on 10.05.2021]
    /// </summary>
    struct Header18
    {
        static const int32 Version = 18;

        /// <summary>
        /// The material version (used by the material assets).
        /// </summary>
        int32 MaterialVersion;

        /// <summary>
        /// The material information (used by the material assets).
        /// </summary>
        MaterialInfo8 MaterialInfo;
    };
    /// <summary>
    /// File header, version 19
    /// [Deprecated on 13.07.2022, expires on 13.07.2024]
    /// </summary>
    struct Header19
    {
        static const int32 Version = 19;

        union
        {
            struct
            {
            } Shader;

            struct
            {
                /// <summary>
                /// The material graph version.
                /// </summary>
                int32 GraphVersion;

                /// <summary>
                /// The material additional information.
                /// </summary>
                MaterialInfo9 Info;
            } Material;

            struct
            {
                /// <summary>
                /// The particle emitter graph version.
                /// </summary>
                int32 GraphVersion;

                /// <summary>
                /// The custom particles data size (in bytes).
                /// </summary>
                int32 CustomDataSize;
            } ParticleEmitter;
        };

        Header19()
        {
        }
    };

    /// <summary>
    /// File header, version 20
    /// </summary>
    struct Header20
    {
        static const int32 Version = 20;

        union
        {
            struct
            {
            } Shader;

            struct
            {
                /// <summary>
                /// The material graph version.
                /// </summary>
                int32 GraphVersion;

                /// <summary>
                /// The material additional information.
                /// </summary>
                MaterialInfo10 Info;
            } Material;

            struct
            {
                /// <summary>
                /// The particle emitter graph version.
                /// </summary>
                int32 GraphVersion;

                /// <summary>
                /// The custom particles data size (in bytes).
                /// </summary>
                int32 CustomDataSize;
            } ParticleEmitter;
        };

        Header20()
        {
        }
    };

    /// <summary>
    /// Current header type
    /// </summary>
    typedef Header20 Header;
};
