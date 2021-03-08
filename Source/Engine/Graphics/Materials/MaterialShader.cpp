// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "MaterialShader.h"
#include "Engine/Core/Log.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Renderer/RenderList.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/Shaders/GPUConstantBuffer.h"
#include "Engine/Graphics/Shaders/GPUShader.h"
#include "Engine/Engine/Time.h"
#include "DecalMaterialShader.h"
#include "PostFxMaterialShader.h"
#include "ForwardMaterialShader.h"
#include "DeferredMaterialShader.h"
#include "GUIMaterialShader.h"
#include "TerrainMaterialShader.h"
#include "ParticleMaterialShader.h"
#include "DeformableMaterialShader.h"
#include "VolumeParticleMaterialShader.h"

IMaterial::BindParameters::BindParameters(::GPUContext* context, const ::RenderContext& renderContext)
    : GPUContext(context)
    , RenderContext(renderContext)
    , FirstDrawCall(nullptr)
    , DrawCallsCount(0)
    , TimeParam(Time::Draw.UnscaledTime.GetTotalSeconds())
{
}

IMaterial::BindParameters::BindParameters(::GPUContext* context, const ::RenderContext& renderContext, const DrawCall& drawCall)
    : GPUContext(context)
    , RenderContext(renderContext)
    , FirstDrawCall(&drawCall)
    , DrawCallsCount(1)
    , TimeParam(Time::Draw.UnscaledTime.GetTotalSeconds())
{
}

IMaterial::BindParameters::BindParameters(::GPUContext* context, const ::RenderContext& renderContext, const DrawCall* firstDrawCall, int32 drawCallsCount)
    : GPUContext(context)
    , RenderContext(renderContext)
    , FirstDrawCall(firstDrawCall)
    , DrawCallsCount(drawCallsCount)
    , TimeParam(Time::Draw.UnscaledTime.GetTotalSeconds())
{
}

GPUPipelineState* MaterialShader::PipelineStateCache::InitPS(CullMode mode, bool wireframe)
{
    Desc.CullMode = mode;
    Desc.Wireframe = wireframe;
    auto ps = GPUDevice::Instance->CreatePipelineState();
    ps->Init(Desc);
    return ps;
}

MaterialShader::MaterialShader(const String& name)
    : _isLoaded(false)
    , _shader(nullptr)
{
    ASSERT(GPUDevice::Instance);
    _shader = GPUDevice::Instance->CreateShader(name);
}

MaterialShader::~MaterialShader()
{
    ASSERT(!_isLoaded);
    SAFE_DELETE_GPU_RESOURCE(_shader);
}

MaterialShader* MaterialShader::Create(const String& name, MemoryReadStream& shaderCacheStream, const MaterialInfo& info)
{
    MaterialShader* material;
    switch (info.Domain)
    {
    case MaterialDomain::Surface:
        material = info.BlendMode == MaterialBlendMode::Opaque ? (MaterialShader*)New<DeferredMaterialShader>(name) : (MaterialShader*)New<ForwardMaterialShader>(name);
        break;
    case MaterialDomain::PostProcess:
        material = New<PostFxMaterialShader>(name);
        break;
    case MaterialDomain::Decal:
        material = New<DecalMaterialShader>(name);
        break;
    case MaterialDomain::GUI:
        material = New<GUIMaterialShader>(name);
        break;
    case MaterialDomain::Terrain:
        material = New<TerrainMaterialShader>(name);
        break;
    case MaterialDomain::Particle:
        material = New<ParticleMaterialShader>(name);
        break;
    case MaterialDomain::Deformable:
        material = New<DeformableMaterialShader>(name);
        break;
    case MaterialDomain::VolumeParticle:
        material = New<VolumeParticleMaterialShader>(name);
        break;
    default:
        LOG(Error, "Unknown material type.");
        return nullptr;
    }
    if (material->Load(shaderCacheStream, info))
    {
        Delete(material);
        return nullptr;
    }
    return material;
}

class DummyMaterial : public MaterialShader
{
public:

    DummyMaterial()
        : MaterialShader(String::Empty)
    {
    }

public:

    // [Material]
    void Bind(BindParameters& params) override
    {
    }

protected:

    // [Material]
    bool Load() override
    {
        return false;
    }
};

MaterialShader* MaterialShader::CreateDummy(MemoryReadStream& shaderCacheStream, const MaterialInfo& info)
{
    MaterialShader* material = New<DummyMaterial>();
    if (material->Load(shaderCacheStream, info))
    {
        Delete(material);
        return nullptr;
    }
    return material;
}

const MaterialInfo& MaterialShader::GetInfo() const
{
    return _info;
}

bool MaterialShader::IsReady() const
{
    return _isLoaded;
}

bool MaterialShader::Load(MemoryReadStream& shaderCacheStream, const MaterialInfo& info)
{
    ASSERT(!_isLoaded);

    // Cache material info
    _info = info;

    // Create shader
    if (_shader->Create(shaderCacheStream))
    {
        LOG(Warning, "Cannot load shader.");
        return true;
    }

    // Init memory for a constant buffer
    _cb = _shader->GetCB(0);
    if (_cb)
    {
        int32 cbSize = _cb->GetSize();
        if (cbSize == 0)
        {
            // Handle unused constant buffer (eg. postFx returning solid color)
            cbSize = 1024;
            _cb = nullptr;
        }
        _cbData.Resize(cbSize, false);
    }

    // Initialize the material based on type (create pipeline states and setup)
    if (Load())
    {
        return true;
    }

    _isLoaded = true;
    return false;
}

void MaterialShader::Unload()
{
    _isLoaded = false;
    _cb = nullptr;
    _cbData.Resize(0, false);
    _shader->ReleaseGPU();
}
