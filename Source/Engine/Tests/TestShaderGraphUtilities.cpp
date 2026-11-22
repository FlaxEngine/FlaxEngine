// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine/Visject/ShaderGraphUtilities.h"
#include "Engine/Visject/VisjectMeta.h"
#include <ThirdParty/catch2/catch.hpp>

#if USE_EDITOR

TEST_CASE("ShaderGraphUtilities")
{
    SECTION("GPU texture type from parameter metadata")
    {
        // No entry is a 2D texture
        VisjectMeta meta;
        CHECK(ShaderGraphUtilities::GetGPUTextureType(meta) == MaterialParameterType::GPUTexture);

        // Single byte with the material parameter type (written by SurfaceMeta.SetGPUTextureType in the editor)
        byte type = (byte)MaterialParameterType::GPUTextureArray;
        meta.AddEntry(ShaderGraphUtilities::GPUTextureTypeMetaTypeID, &type, sizeof(type));
        CHECK(ShaderGraphUtilities::GetGPUTextureType(meta) == MaterialParameterType::GPUTextureArray);
        meta.Release();
        type = (byte)MaterialParameterType::GPUTextureVolume;
        meta.AddEntry(ShaderGraphUtilities::GPUTextureTypeMetaTypeID, &type, sizeof(type));
        CHECK(ShaderGraphUtilities::GetGPUTextureType(meta) == MaterialParameterType::GPUTextureVolume);
        meta.Release();
        type = (byte)MaterialParameterType::GPUTextureCube;
        meta.AddEntry(ShaderGraphUtilities::GPUTextureTypeMetaTypeID, &type, sizeof(type));
        CHECK(ShaderGraphUtilities::GetGPUTextureType(meta) == MaterialParameterType::GPUTextureCube);
        meta.Release();

        // Invalid data falls back to a 2D texture
        type = (byte)MaterialParameterType::Texture;
        meta.AddEntry(ShaderGraphUtilities::GPUTextureTypeMetaTypeID, &type, sizeof(type));
        CHECK(ShaderGraphUtilities::GetGPUTextureType(meta) == MaterialParameterType::GPUTexture);
        meta.Release();
        int32 wrongSize = (int32)MaterialParameterType::GPUTextureArray;
        meta.AddEntry(ShaderGraphUtilities::GPUTextureTypeMetaTypeID, (byte*)&wrongSize, sizeof(wrongSize));
        CHECK(ShaderGraphUtilities::GetGPUTextureType(meta) == MaterialParameterType::GPUTexture);
    }
}

#endif
