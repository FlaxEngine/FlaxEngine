// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "MaterialShader.h"
#include "Engine/Core/Log.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Level/LargeWorlds.h"
#include "Engine/Renderer/RenderList.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/GPUContext.h"
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

GPU_CB_STRUCT(MaterialShaderDataPerView {
    Matrix ViewMatrix;
    Matrix ViewProjectionMatrix;
    Matrix PrevViewProjectionMatrix;
    Matrix MainViewProjectionMatrix;
    Float4 MainScreenSize;
    Float3 ViewPos;
    float ViewFar;
    Float3 ViewDir;
    float TimeParam;
    Float4 ViewInfo;
    Float4 ScreenSize;
    Float4 TemporalAAJitter;
    Float3 LargeWorldsChunkIndex;
    float LargeWorldsChunkSize;
    });

IMaterial::BindParameters::BindParameters(::GPUContext* context, const ::RenderContext& renderContext)
    : GPUContext(context)
    , RenderContext(renderContext)
    , TimeParam(Time::Draw.UnscaledTime.GetTotalSeconds())
{
}

IMaterial::BindParameters::BindParameters(::GPUContext* context, const ::RenderContext& renderContext, const ::DrawCall& drawCall, bool instanced)
    : GPUContext(context)
    , RenderContext(renderContext)
    , DrawCall(&drawCall)
    , TimeParam(Time::Draw.UnscaledTime.GetTotalSeconds())
    , Instanced(instanced)
{
}

GPUConstantBuffer* IMaterial::BindParameters::PerViewConstants = nullptr;
GPUConstantBuffer* IMaterial::BindParameters::PerDrawConstants = nullptr;

void IMaterial::BindParameters::BindViewData()
{
    // Lazy-init
    if (!PerViewConstants)
    {
        PerViewConstants = GPUDevice::Instance->CreateConstantBuffer(sizeof(MaterialShaderDataPerView), TEXT("PerViewConstants"));
        PerDrawConstants = GPUDevice::Instance->CreateConstantBuffer(sizeof(MaterialShaderDataPerDraw), TEXT("PerDrawConstants"));
    }

    // Setup data
    const auto& view = RenderContext.View;
    MaterialShaderDataPerView cb;
    Matrix::Transpose(view.Frustum.GetMatrix(), cb.ViewProjectionMatrix);
    Matrix::Transpose(view.View, cb.ViewMatrix);
    Matrix::Transpose(view.PrevViewProjection, cb.PrevViewProjectionMatrix);
    Matrix::Transpose(view.MainViewProjection, cb.MainViewProjectionMatrix);
    cb.MainScreenSize = view.MainScreenSize;
    cb.ViewPos = view.Position;
    cb.ViewFar = view.Far;
    cb.ViewDir = view.Direction;
    cb.TimeParam = TimeParam;
    cb.ViewInfo = view.ViewInfo;
    cb.ScreenSize = view.ScreenSize;
    cb.TemporalAAJitter = view.TemporalAAJitter;
    cb.LargeWorldsChunkIndex = LargeWorlds::Enable ? (Float3)Int3(view.Origin / LargeWorlds::ChunkSize) : Float3::Zero;
    cb.LargeWorldsChunkSize = LargeWorlds::ChunkSize;

    // Update constants
    GPUContext->UpdateCB(PerViewConstants, &cb);
    GPUContext->BindCB(1, PerViewConstants);
}

void IMaterial::BindParameters::BindDrawData()
{
    // Write draw call to the object buffer
    ASSERT(DrawCall);
    auto& objectBuffer = RenderContext.List->TempObjectBuffer;
    objectBuffer.Clear();
    ShaderObjectData objData;
    objData.Store(*DrawCall);
    objectBuffer.Write(objData);
    objectBuffer.Flush(GPUContext);
    ObjectBuffer = objectBuffer.GetBuffer()->View();

    // Setup data
    MaterialShaderDataPerDraw perDraw;
    perDraw.DrawPadding = Float3::Zero;
    perDraw.DrawObjectIndex = 0;

    // Update constants
    GPUContext->UpdateCB(PerDrawConstants, &perDraw);
    GPUContext->BindCB(2, PerDrawConstants);
}

GPUPipelineState* MaterialShader::PipelineStateCache::InitPS(CullMode mode, bool wireframe)
{
    Desc.CullMode = mode;
    Desc.Wireframe = wireframe;
    auto ps = GPUDevice::Instance->CreatePipelineState();
    ps->Init(Desc);
    return ps;
}

MaterialShader::MaterialShader(const StringView& name)
    : _isLoaded(false)
    , _shader(nullptr)
{
    ASSERT(GPUDevice::Instance);
    _shader = GPUDevice::Instance->CreateShader(name);
}

MaterialShader::~MaterialShader()
{
    ASSERT(!_isLoaded && _shader);
    SAFE_DELETE_GPU_RESOURCE(_shader);
}

MaterialShader* MaterialShader::Create(const StringView& name, MemoryReadStream& shaderCacheStream, const MaterialInfo& info)
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

GPUShader* MaterialShader::GetShader() const
{
    return _shader;
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
        Platform::MemoryClear(_cbData.Get(), cbSize);
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
