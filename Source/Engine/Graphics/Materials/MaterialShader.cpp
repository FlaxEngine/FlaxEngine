// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "MaterialShader.h"
#include "Engine/Core/Log.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Renderer/RenderList.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/Shaders/GPUConstantBuffer.h"
#include "Engine/Graphics/Shaders/GPUShader.h"
#include "DecalMaterialShader.h"
#include "PostFxMaterialShader.h"
#include "ForwardMaterialShader.h"
#include "DeferredMaterialShader.h"
#include "GUIMaterialShader.h"
#include "TerrainMaterialShader.h"
#include "ParticleMaterialShader.h"

GPUPipelineState* MaterialShader::PipelineStateCache::GetPS(CullMode mode, bool wireframe)
{
    const int32 index = static_cast<int32>(mode) + (wireframe ? 3 : 0);
    if (PS[index])
        return PS[index];

    Desc.CullMode = mode;
    Desc.Wireframe = wireframe;
    PS[index] = GPUDevice::Instance->CreatePipelineState();
    PS[index]->Init(Desc);
    return PS[index];
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
    default:
        LOG(Fatal, "Unknown material type.");
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
    const auto cb0 = _shader->GetCB(0);
    if (cb0)
    {
        _cb0Data.Resize(cb0->GetSize(), false);
    }
    const auto cb1 = _shader->GetCB(1);
    if (cb1)
    {
        _cb1Data.Resize(cb1->GetSize(), false);
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
    _cb0Data.Resize(0, false);
    _cb1Data.Resize(0, false);
    _shader->ReleaseGPU();
}
