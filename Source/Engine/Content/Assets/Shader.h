// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "../BinaryAsset.h"
#include "Engine/Graphics/Shaders/Cache/ShaderAssetBase.h"

class GPUShader;

/// <summary>
/// The shader asset. Contains a program that runs on the GPU and is able to perform rendering calculation using textures, vertices and other resources.
/// </summary>
API_CLASS(NoSpawn) class FLAXENGINE_API Shader : public ShaderAssetTypeBase<BinaryAsset>
{
    DECLARE_BINARY_ASSET_HEADER(Shader, ShadersSerializedVersion);
private:
    GPUShader* _shader;

public:
    /// <summary>
    /// Finalizes an instance of the <see cref="Shader"/> class.
    /// </summary>
    ~Shader();

public:
    /// <summary>
    /// The GPU shader object (not null).
    /// </summary>
    API_FIELD(ReadOnly) GPUShader* GPU;

    /// <summary>
    /// Gets the GPU shader object.
    /// </summary>
    FORCE_INLINE GPUShader* GetShader() const
    {
        return GPU;
    }

protected:
    // [BinaryAsset]
    LoadResult load() override;
    void unload(bool isReloading) override;
};
