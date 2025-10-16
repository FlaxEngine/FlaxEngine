// Copyright (c) Wojciech Figat. All rights reserved.

#include "Shader.h"
#include "Engine/Core/Log.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/Shaders/GPUShader.h"
#if GPU_ENABLE_RESOURCE_NAMING && !USE_EDITOR
#include "Engine/Content/Content.h"
#include "Engine/Content/Cache/AssetsCache.h"
#endif
#include "Engine/Content/Upgraders/ShaderAssetUpgrader.h"
#include "Engine/Content/Factories/BinaryAssetFactory.h"
#include "Engine/Serialization/MemoryReadStream.h"

REGISTER_BINARY_ASSET_WITH_UPGRADER(Shader, "FlaxEngine.Shader", ShaderAssetUpgrader, false);

Shader::Shader(const SpawnParams& params, const AssetInfo* info)
    : ShaderAssetTypeBase<BinaryAsset>(params, info)
{
    ASSERT(GPUDevice::Instance);
    StringView name = info->Path;
#if GPU_ENABLE_RESOURCE_NAMING && !USE_EDITOR
    name = Content::GetRegistry()->GetEditorAssetPath(info->ID);
#endif
    _shader = GPUDevice::Instance->CreateShader(name);
    ASSERT(_shader);
    GPU = _shader;
}

Shader::~Shader()
{
    Delete(_shader);
}

Asset::LoadResult Shader::load()
{
    // Special case for Null renderer that doesn't need shaders
    if (IsNullRenderer())
    {
        return LoadResult::Ok;
    }

    // Load shader cache (it may call compilation or gather cached data)
    ShaderCacheResult shaderCache;
    if (LoadShaderCache(shaderCache))
    {
        LOG(Error, "Cannot load \'{0}\' shader cache.", ToString());
        return LoadResult::Failed;
    }
    MemoryReadStream shaderCacheStream(shaderCache.Data.Get(), shaderCache.Data.Length());

    // Create shader from cache
    if (_shader->Create(shaderCacheStream))
    {
        LOG(Error, "Cannot load shader \'{0}\'", ToString());
        return LoadResult::Failed;
    }

#if COMPILE_WITH_SHADER_COMPILER
    RegisterForShaderReloads(this, shaderCache);
#endif
    return LoadResult::Ok;
}

void Shader::unload(bool isReloading)
{
#if COMPILE_WITH_SHADER_COMPILER
    UnregisterForShaderReloads(this);
#endif

    _shader->ReleaseGPU();
}
