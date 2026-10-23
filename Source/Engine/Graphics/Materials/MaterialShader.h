// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "IMaterial.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Graphics/GPUPipelineState.h"
#include "Engine/Renderer/Config.h"

/// <summary>
/// Current materials shader version.
/// </summary>
#define MATERIAL_GRAPH_VERSION 187

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
        GPUPipelineState* PS[3];
        GPUPipelineState::Description Desc;

        PipelineStateCache()
        {
            Platform::MemoryClear(this, sizeof(*this));
        }

        void Init(const GPUPipelineState::Description& desc)
        {
            Desc = desc;
        }

        GPUPipelineState* GetPS(const MaterialShader* owner, CullMode cullMode)
        {
            auto ps = PS[(int32)cullMode];
            if (!ps)
                PS[(int32)cullMode] = ps = InitPS(owner, cullMode);
            return ps;
        }

        GPUPipelineState* InitPS(const MaterialShader* owner, CullMode mode);
#if COMPILE_WITH_PSO_PRECACHING
        static void PrecachePSO(PipelineStateCache* psCache, const PrecacheParameters& params, const MaterialShader* owner, CullMode mode);
#endif

        void Release()
        {
            SAFE_DELETE_GPU_RESOURCES(PS);
        }
    };

protected:
    bool _isLoaded;
    DrawPass _drawModes = DrawPass::None;
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

    FORCE_INLINE CullMode GetCullMode(DrawPass pass, bool worldDeterminantSign) const
    {
        CullMode cullMode = pass == DrawPass::Depth ? CullMode::TwoSided : _info.CullMode;
        if (cullMode != CullMode::TwoSided && worldDeterminantSign)
        {
            // Invert culling when scale is negative
            cullMode = cullMode == CullMode::Normal ? CullMode::Inverted : CullMode::Normal;
        }
        return cullMode;
    }

public:
    // [IMaterial]
    DrawPass GetDrawModes() const override;
    const MaterialInfo& GetInfo() const override;
    GPUShader* GetShader() const override;
    bool IsReady() const override;
};
