// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "IMaterial.h"
#include "Engine/Graphics/GPUPipelineState.h"
#include "Engine/Renderer/Config.h"

/// <summary>
/// Current materials shader version.
/// </summary>
#define MATERIAL_GRAPH_VERSION 148

class Material;
class GPUShader;
class GPUConstantBuffer;
class MemoryReadStream;

/// <summary>
/// Represents material shader that can be used to render objects, visuals or effects. Contains a dedicated shader.
/// </summary>
class MaterialShader : public IMaterial
{
protected:

    struct PipelineStateCache
    {
        GPUPipelineState* PS[6];
        GPUPipelineState::Description Desc;

        PipelineStateCache()
        {
            Platform::MemoryClear(PS, sizeof(PS));
        }

        void Init(GPUPipelineState::Description& desc)
        {
            Desc = desc;
        }

        GPUPipelineState* GetPS(CullMode mode, bool wireframe)
        {
            const int32 index = static_cast<int32>(mode) + (wireframe ? 3 : 0);
            auto ps = PS[index];
            if (!ps)
                PS[index] = ps = InitPS(mode, wireframe);
            return ps;
        }

        GPUPipelineState* InitPS(CullMode mode, bool wireframe);

        void Release()
        {
            for (int32 i = 0; i < ARRAY_COUNT(PS); i++)
            {
                SAFE_DELETE_GPU_RESOURCE(PS[i]);
            }
        }
    };

protected:

    bool _isLoaded;
    GPUShader* _shader;
    GPUConstantBuffer* _cb;
    Array<byte> _cbData;
    MaterialInfo _info;

protected:

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="name">Material resource name</param>
    MaterialShader(const String& name);

public:

    /// <summary>
    /// Finalizes an instance of the <see cref="MaterialShader"/> class.
    /// </summary>
    virtual ~MaterialShader();

public:

    /// <summary>
    /// Creates and loads the material from the data.
    /// </summary>
    /// <param name="name">Material resource name</param>
    /// <param name="shaderCacheStream">Stream with compiled shader data</param>
    /// <param name="info">Loaded material info structure</param>
    /// <returns>The created and loaded material or null if failed.</returns>
    static MaterialShader* Create(const String& name, MemoryReadStream& shaderCacheStream, const MaterialInfo& info);

    /// <summary>
    /// Creates the dummy material used by the Null rendering backend to mock object but not perform any rendering.
    /// </summary>
    /// <param name="shaderCacheStream">The shader cache stream.</param>
    /// <param name="info">The material information.</param>
    /// <returns>The created and loaded material or null if failed.</returns>
    static MaterialShader* CreateDummy(MemoryReadStream& shaderCacheStream, const MaterialInfo& info);

    /// <summary>
    /// Clears the loaded data.
    /// </summary>
    virtual void Unload();

protected:

    bool Load(MemoryReadStream& shaderCacheStream, const MaterialInfo& info);
    virtual bool Load() = 0;

public:

    // [IMaterial]
    const MaterialInfo& GetInfo() const override;
    bool IsReady() const override;
};
