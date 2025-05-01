// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_NULL

#include "Engine/Graphics/Textures/GPUTexture.h"

/// <summary>
/// Texture object for Null backend.
/// </summary>
class GPUTextureNull : public GPUTexture
{
public:
    // [GPUTexture]
    GPUTextureView* View(int32 arrayOrDepthIndex) const override
    {
        return nullptr;
    }
    GPUTextureView* View(int32 arrayOrDepthIndex, int32 mipMapIndex) const override
    {
        return nullptr;
    }
    GPUTextureView* ViewArray() const override
    {
        return nullptr;
    }
    GPUTextureView* ViewVolume() const override
    {
        return nullptr;
    }
    GPUTextureView* ViewReadOnlyDepth() const override
    {
        return nullptr;
    }
    void* GetNativePtr() const override
    {
        return nullptr;
    }
    bool GetData(int32 arrayIndex, int32 mipMapIndex, TextureMipData& data, uint32 mipRowPitch) override
    {
        return true;
    }

protected:
    // [GPUTexture]
    bool OnInit() override
    {
        return false;
    }
    void OnResidentMipsChanged() override
    {
    }
};

#endif
