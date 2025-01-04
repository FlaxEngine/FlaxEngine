// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "IMaterial.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Graphics/GPUPipelineState.h"
#include "Engine/Renderer/Config.h"

/// <summary>
/// Current materials shader version.
/// </summary>
#define MATERIAL_GRAPH_VERSION 170

class Material;
class GPUShader;
class GPUConstantBuffer;
class MemoryReadStream;

// Draw pipeline constant buffer (with per-draw constants at slot 2)
GPU_CB_STRUCT(MaterialShaderDataPerDraw {
    Float3 DrawPadding;
    uint32 DrawObjectIndex;
    });

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
            SAFE_DELETE_GPU_RESOURCES(PS);
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
    MaterialShader(const StringView& name);

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
    static MaterialShader* Create(const StringView& name, MemoryReadStream& shaderCacheStream, const MaterialInfo& info);

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
    GPUShader* GetShader() const override;
    bool IsReady() const override;
};
