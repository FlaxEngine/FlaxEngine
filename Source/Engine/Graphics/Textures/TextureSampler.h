// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Math/Vector4.h"
#include "Engine/Core/Math/Color.h"
#include "Engine/Graphics/PixelFormatSampler.h"
#include "Engine/Graphics/Textures/TextureData.h"

// Utility for sampling texture assets data on CPU. Enables loading any texture mip, decompressing it and sampling with linear or point sampler.
struct FLAXENGINE_API TextureSampler
{
    FlaxStorage::LockData Lock;
    BytesContainer MipData;
    TextureData Tmp;
    uint32 RowPitch = 0, SlicePitch = 0;
    PixelFormat Format = PixelFormat::Unknown;
    Int2 Size = Int2::Zero;
    byte* DataPtr = nullptr;
    const PixelFormatSampler* Sampler = nullptr;

public:
    bool Init(TextureBase* texture, bool hdr = false, int32 mipIndex = 0);

    FORCE_INLINE void Store(int32 x, int32 y, const Color& color) const
    {
        Sampler->Store(DataPtr, x, y, RowPitch, color);
    }

    FORCE_INLINE Float4 Sample(int32 x) const
    {
        return Sampler->Sample(DataPtr, x);
    }

    FORCE_INLINE Color SamplePoint(const Float2& uv) const
    {
        return Sampler->SamplePoint(DataPtr, uv, Size, RowPitch);
    }

    FORCE_INLINE Color SamplePoint(int32 x, int32 y) const
    {
        return Sampler->SamplePoint(DataPtr, x, y, RowPitch);
    }

    FORCE_INLINE Color SampleLinear(const Float2& uv) const
    {
        return Sampler->SampleLinear(DataPtr, uv, Size, RowPitch);
    }
};
