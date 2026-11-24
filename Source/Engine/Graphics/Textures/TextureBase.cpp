// Copyright (c) Wojciech Figat. All rights reserved.

#include "TextureBase.h"
#include "TextureData.h"
#include "TextureSampler.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Color32.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/Textures/GPUTexture.h"
#include "Engine/Graphics/RenderTools.h"
#include "Engine/Graphics/PixelFormatExtensions.h"
#include "Engine/Debug/Exceptions/ArgumentOutOfRangeException.h"
#include "Engine/Debug/Exceptions/InvalidOperationException.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Profiler/ProfilerMemory.h"
#include "Engine/Content/Factories/BinaryAssetFactory.h"
#include "Engine/Graphics/PixelFormatSampler.h"
#include "Engine/Scripting/Enums.h"
#include "Engine/Streaming/Streaming.h"
#include "Engine/Tools/TextureTool/TextureTool.h"
#include "Engine/Threading/Threading.h"
#if GPU_ENABLE_RESOURCE_NAMING && !USE_EDITOR
#include "Engine/Content/Content.h"
#include "Engine/Content/Cache/AssetsCache.h"
#endif

bool TextureSampler::Init(TextureBase* texture, bool hdr, int32 mipIndex)
{
    CHECK_RETURN(texture, true);
    PROFILE_CPU();
    PROFILE_MEM(GraphicsTextures);

    // Lock asset chunks (if not virtual)
    Lock = texture->LockData();

    // Get the mip data
    {
        PROFILE_CPU_NAMED("GetMipData");
        texture->GetMipData(mipIndex, MipData, RowPitch, SlicePitch, true);
        if (MipData.IsInvalid())
        {
            LOG(Warning, "Failed to get texture mip{} data '{}'.", mipIndex, texture->GetPath());
            return true;
        }
    }
    Size.X = Math::Max(1, texture->Width() >> mipIndex);
    Size.Y = Math::Max(1, texture->Height() >> mipIndex);
    Format = texture->Format();

    // Decompress or convert data if need to
    DataPtr = MipData.Get();
    if (PixelFormatExtensions::IsCompressed(Format) || PixelFormatSampler::Get(Format) == nullptr)
    {
        PROFILE_CPU_NAMED("Decompress");

        // Prepare source data descriptor (no data copy, just link the mip data)
        TextureData src;
        src.Width = Size.X;
        src.Height = Size.Y;
        src.Depth = 1;
        src.Format = Format;
        src.Items.Resize(1);
        src.Items[0].Mips.Resize(1);
        auto& srcMip = src.Items[0].Mips[0];
        srcMip.Data.Link(MipData);
        srcMip.DepthPitch = SlicePitch;
        srcMip.RowPitch = RowPitch;
        srcMip.Lines = src.Height;

        // Decompress texture
        auto dstFormat = hdr ? PixelFormat::R16G16B16A16_Float : PixelFormat::R8G8B8A8_UNorm;
        if (TextureTool::Convert(Tmp, src, dstFormat))
        {
            LOG(Warning, "Failed to decompress data from {} to {}.", ScriptingEnum::ToString(Format), ScriptingEnum::ToString(dstFormat));
            return true;
        }

        // Override source data and format
        Format = Tmp.Format;
        RowPitch = Tmp.Items[0].Mips[0].RowPitch;
        SlicePitch = Tmp.Items[0].Mips[0].DepthPitch;
        DataPtr = Tmp.Items[0].Mips[0].Data.Get();
    }

    // Check if can even sample the given format
    Sampler = PixelFormatSampler::Get(Format);
    if (Sampler == nullptr)
    {
        LOG(Warning, "Texture format {0} cannot be sampled.", ScriptingEnum::ToString(Format));
        return true;
    }

    return false;
}

TextureMipData::TextureMipData()
    : RowPitch(0)
    , DepthPitch(0)
    , Lines(0)
{
}

TextureMipData::TextureMipData(const TextureMipData& other)
    : RowPitch(other.RowPitch)
    , DepthPitch(other.DepthPitch)
    , Lines(other.Lines)
    , Data(other.Data)
{
}

TextureMipData::TextureMipData(TextureMipData&& other) noexcept
    : RowPitch(other.RowPitch)
    , DepthPitch(other.DepthPitch)
    , Lines(other.Lines)
    , Data(MoveTemp(other.Data))
{
}

TextureMipData& TextureMipData::operator=(const TextureMipData& other)
{
    if (this == &other)
        return *this;
    RowPitch = other.RowPitch;
    DepthPitch = other.DepthPitch;
    Lines = other.Lines;
    Data = other.Data;
    return *this;
}

TextureMipData& TextureMipData::operator=(TextureMipData&& other) noexcept
{
    if (this == &other)
        return *this;
    RowPitch = other.RowPitch;
    DepthPitch = other.DepthPitch;
    Lines = other.Lines;
    Data = MoveTemp(other.Data);
    return *this;
}

void TextureMipData::Init(int32 width, int32 height, PixelFormat format)
{
    uint32 rowPitch, slicePicth;
    RenderTools::ComputePitch(format, width, height, rowPitch, slicePicth);
    uint32 lines = slicePicth / rowPitch;
    if (RowPitch == rowPitch && DepthPitch == slicePicth && Lines == lines && Data.Length() == slicePicth)
        return;
    RowPitch = rowPitch;
    DepthPitch = slicePicth;
    Lines = lines;
    Data.Allocate(slicePicth);
}

bool TextureMipData::GetPixels(Array<Color32>& pixels, int32 width, int32 height, PixelFormat format) const
{
    const int32 size = width * height;
    if (Data.IsInvalid() || size < 1)
        return true;
    pixels.Resize(size);
    return TextureTool::Convert(width, height, PixelFormat::R8G8B8A8_UNorm, Span<byte>(ToSpan(pixels)), width * sizeof(Color32), format, Data, RowPitch);
}

bool TextureMipData::GetPixels(Array<Color>& pixels, int32 width, int32 height, PixelFormat format) const
{
    const int32 size = width * height;
    if (Data.IsInvalid() || size < 1)
        return true;
    pixels.Resize(size);
    return TextureTool::Convert(width, height, PixelFormat::R32G32B32A32_Float, Span<byte>(ToSpan(pixels)), width * sizeof(Color), format, Data, RowPitch);
}

bool TextureMipData::SetPixels(const Span<Color32>& pixels, int32 width, int32 height, PixelFormat format)
{
    Init(width, height, format);
    return TextureTool::Convert(width, height, format, Data, RowPitch, PixelFormat::R8G8B8A8_UNorm, Span<byte>(pixels), width * sizeof(Color32));
}

bool TextureMipData::SetPixels(const Span<Color>& pixels, int32 width, int32 height, PixelFormat format)
{
    Init(width, height, format);
    return TextureTool::Convert(width, height, format, Data, RowPitch, PixelFormat::R32G32B32A32_Float, Span<byte>(pixels), width * sizeof(Color));
}

void TextureMipData::Copy(void* data, uint32 dataRowPitch, uint32 dataDepthPitch, uint32 dataDepthSlices, uint32 targetRowPitch)
{
    // Check if target row pitch is the same
    if (targetRowPitch == dataRowPitch || targetRowPitch == 0)
    {
        Lines = dataDepthPitch / dataRowPitch;
        DepthPitch = dataDepthPitch;
        RowPitch = dataRowPitch;

        // Single memory copy
        Data.Copy((byte*)data, dataDepthPitch * dataDepthSlices);
    }
    else
    {
        Lines = dataDepthPitch / dataRowPitch;
        DepthPitch = targetRowPitch * Lines;
        RowPitch = targetRowPitch;

        // Convert row by row
        Data.Allocate(DepthPitch * dataDepthSlices);
        for (uint32 depth = 0; depth < dataDepthSlices; depth++)
        {
            byte* src = (byte*)data + depth * dataDepthPitch;
            byte* dst = Data.Get() + depth * DepthPitch;
            for (uint32 row = 0; row < Lines; row++)
            {
                Platform::MemoryCopy(dst + row * RowPitch, src + row * dataRowPitch, RowPitch);
            }
        }
    }
}

int32 TextureData::GetArraySize() const
{
    return Items.Count();
}

int32 TextureData::GetMipLevels() const
{
    return Items.HasItems() ? Items[0].Mips.Count() : 0;
}

void TextureData::Clear()
{
    Items.Resize(0, false);
}

bool TextureData::Init(PixelFormat format, int32 width, int32 height, int32 depth, int32 mipLevels, int32 arraySize)
{
    // Check inputs
    CHECK_RETURN(Math::IsInRange(width, 1, GPU_MAX_TEXTURE_SIZE), true);
    CHECK_RETURN(Math::IsInRange(height, 1, GPU_MAX_TEXTURE_SIZE), true);
    CHECK_RETURN(Math::IsInRange(depth, 1, GPU_MAX_TEXTURE_SIZE), true);
    CHECK_RETURN(Math::IsInRange(arraySize, 1, GPU_MAX_TEXTURE_ARRAY_SIZE), true);

    // Auto-compute mip levels from dimensions
    int32 mipLevelsMax = MipLevelsCount(width, height, depth);
    if (mipLevels == -1)
        mipLevels = mipLevelsMax;
    else
    {
        CHECK_RETURN(Math::IsInRange(mipLevels, 1, mipLevelsMax), true);
    }

    // Skip when texture won't change
    if (format == Format && width == Width && height == Height && depth == Depth && arraySize == GetArraySize() && mipLevels == GetMipLevels())
        return false;

    // Initialize
    Format = format;
    Width = width;
    Height = height;
    Depth = depth;
    Items.Resize(arraySize);
    for (auto& item : Items)
        item.Mips.Resize(mipLevels);
    return false;
}

bool TextureData::GetPixels(Array<Color32>& pixels, int32 mipIndex, int32 arrayIndex) const
{
    if (Items.IsValidIndex(arrayIndex) && Items.Get()[arrayIndex].Mips.IsValidIndex(mipIndex))
    {
        const int32 mipWidth = Math::Max(1, Width >> mipIndex);
        const int32 mipHeight = Math::Max(1, Height >> mipIndex);
        return Items.Get()[arrayIndex].Mips.Get()[mipIndex].GetPixels(pixels, mipWidth, mipHeight, Format);
    }
    return true;
}

bool TextureData::GetPixels(Array<Color>& pixels, int32 mipIndex, int32 arrayIndex) const
{
    if (Items.IsValidIndex(arrayIndex) && Items.Get()[arrayIndex].Mips.IsValidIndex(mipIndex))
    {
        const int32 mipWidth = Math::Max(1, Width >> mipIndex);
        const int32 mipHeight = Math::Max(1, Height >> mipIndex);
        return Items.Get()[arrayIndex].Mips.Get()[mipIndex].GetPixels(pixels, mipWidth, mipHeight, Format);
    }
    return true;
}

bool TextureData::SetPixels(const Span<Color32>& pixels, int32 mipIndex, int32 arrayIndex)
{
    if (Items.IsValidIndex(arrayIndex) && Items.Get()[arrayIndex].Mips.IsValidIndex(mipIndex))
    {
        const int32 mipWidth = Math::Max(1, Width >> mipIndex);
        const int32 mipHeight = Math::Max(1, Height >> mipIndex);
        return Items.Get()[arrayIndex].Mips.Get()[mipIndex].SetPixels(pixels, mipWidth, mipHeight, Format);
    }
    return true;
}

bool TextureData::SetPixels(const Span<Color>& pixels, int32 mipIndex, int32 arrayIndex)
{
    if (Items.IsValidIndex(arrayIndex) && Items.Get()[arrayIndex].Mips.IsValidIndex(mipIndex))
    {
        const int32 mipWidth = Math::Max(1, Width >> mipIndex);
        const int32 mipHeight = Math::Max(1, Height >> mipIndex);
        return Items.Get()[arrayIndex].Mips.Get()[mipIndex].SetPixels(pixels, mipWidth, mipHeight, Format);
    }
    return true;
}

bool TextureData::GenerateMip(int32 mipIndex, bool linear)
{
    PROFILE_CPU();
    if (PixelFormatExtensions::IsCompressed(Format))
    {
        LOG(Warning, "Cannot generate mip map for compressed format data.");
        return true;
    }

    for (auto& item : Items)
    {
        if (mipIndex < 1 || mipIndex >= item.Mips.Count())
        {
            LOG(Warning, "Invalid mip map to generate.");
            return true;
        }
        const auto& srcMip = item.Mips[mipIndex - 1];
        auto& dstMip = item.Mips[mipIndex];
        if (srcMip.RowPitch == 0 || srcMip.DepthPitch == 0 || srcMip.Data.IsInvalid())
        {
            LOG(Warning, "Missing data for source mip map.");
            return true;
        }

        // Allocate data
        const int32 dstMipWidth = Math::Max(1, Width >> mipIndex);
        const int32 dstMipHeight = Math::Max(1, Height >> mipIndex);
        const int32 pixelStride = PixelFormatExtensions::SizeInBytes(Format);
        dstMip.RowPitch = dstMipWidth * pixelStride;
        dstMip.DepthPitch = dstMip.RowPitch * dstMipHeight;
        dstMip.Data.Allocate(dstMip.DepthPitch);

        // Perform filtering
        const int32 width = Math::Max(1, Width >> (mipIndex - 1));
        const int32 height = Math::Max(1, Height >> (mipIndex - 1));
        if (TextureTool::GenerateMip(width, height, Format, srcMip.Data, srcMip.RowPitch, dstMip.Data, dstMip.RowPitch, linear))
            return true;
    }

    return false;
}

REGISTER_BINARY_ASSET_ABSTRACT(TextureBase, "FlaxEngine.TextureBase");

TextureBase::TextureBase(const SpawnParams& params, const AssetInfo* info)
    : BinaryAsset(params, info)
    , _texture(this, GPU_GET_RESOURCE_NAME_FROM_ASSET(info))
    , _customData(nullptr)
    , _parent(this)
{
}

Float2 TextureBase::Size() const
{
    return Float2(static_cast<float>(_texture.TotalWidth()), static_cast<float>(_texture.TotalHeight()));
}

int32 TextureBase::GetArraySize() const
{
    return _texture.TotalArraySize();
}

int32 TextureBase::GetMipLevels() const
{
    return _texture.TotalMipLevels();
}

int32 TextureBase::GetResidentMipLevels() const
{
    return _texture.GetTexture()->ResidentMipLevels();
}

uint64 TextureBase::GetCurrentMemoryUsage() const
{
    return _texture.GetTexture()->GetMemoryUsage();
}

uint64 TextureBase::GetTotalMemoryUsage() const
{
    return _texture.GetTotalMemoryUsage();
}

int32 TextureBase::GetTextureGroup() const
{
    return _texture._header.TextureGroup;
}

void TextureBase::SetTextureGroup(int32 textureGroup)
{
    if (_texture._header.TextureGroup != textureGroup)
    {
        _texture._header.TextureGroup = textureGroup;
        _texture.RequestStreamingUpdate();
    }
}

bool TextureBase::HasStreamingError() const
{
    return _texture.Streaming.Error;
}

void TextureBase::SetStreamingVisible() const
{
    if (_texture.GetTexture())
        _texture.GetTexture()->LastRenderTime = Platform::GetTimeSeconds();
}

BytesContainer TextureBase::GetMipData(int32 mipIndex, int32& rowPitch, int32& slicePitch)
{
    BytesContainer result;
    if (IsVirtual() && _customData == nullptr)
    {
        LOG(Error, "Missing virtual texture init data.");
        return result;
    }
    if (WaitForLoaded())
        return result;
    uint32 rowPitch1, slicePitch1;
    GetMipData(mipIndex, result, rowPitch1, slicePitch1, true);
    rowPitch = rowPitch1;
    slicePitch = slicePitch1;
    return result;
}

bool TextureBase::GetTextureData(TextureData& result, bool copyData)
{
    PROFILE_CPU_NAMED("Texture.GetTextureData");
    if (!IsVirtual() && WaitForLoaded())
    {
        LOG(Error, "Asset load failed.");
        return true;
    }
    auto dataLock = LockData();

    // Setup description
    result.Width = _texture.TotalWidth();
    result.Height = _texture.TotalHeight();
    result.Depth = 1;
    result.Format = _texture.GetHeader()->Format;
    result.Items.Resize(_texture.TotalArraySize());

    // Setup mips data
    for (int32 arraySlice = 0; arraySlice < result.Items.Count(); arraySlice++)
    {
        auto& slice = result.Items[arraySlice];
        slice.Mips.Resize(_texture.TotalMipLevels());
        for (int32 mipIndex = 0; mipIndex < slice.Mips.Count(); mipIndex++)
        {
            auto& mip = slice.Mips[mipIndex];
            int32 rowPitch, slicePitch;
            BytesContainer mipData = GetMipData(mipIndex, rowPitch, slicePitch);
            if (mipData.IsInvalid())
            {
                LOG(Error, "Failed to get texture mip data.");
                return true;
            }
            if (mipData.Length() != slicePitch * _texture.TotalArraySize())
            {
                LOG(Error, "Invalid custom texture data (slice pitch * array size is different than data bytes count).");
                return true;
            }
            mip.RowPitch = rowPitch;
            mip.DepthPitch = slicePitch;
            mip.Lines = Math::Max(1, Height() >> mipIndex);
            if (copyData)
                mip.Data.Copy(mipData.Get() + (arraySlice * slicePitch), slicePitch);
            else
                mip.Data.Link(mipData.Get() + (arraySlice * slicePitch), slicePitch);
        }
    }

    return false;
}

bool TextureBase::GetTextureMipData(TextureMipData& result, int32 mipIndex, int32 arrayIndex, bool copyData)
{
    PROFILE_CPU_NAMED("Texture.GetTextureMipData");
    if (!IsVirtual() && WaitForLoaded())
    {
        LOG(Error, "Asset load failed.");
        return true;
    }
    if (mipIndex < 0 || mipIndex > GetMipLevels() || arrayIndex < 0 || arrayIndex > GetArraySize())
    {
        Log::ArgumentOutOfRangeException();
        return true;
    }

    // Get raw texture data
    int32 rowPitch, slicePitch;
    BytesContainer mipData = GetMipData(mipIndex, rowPitch, slicePitch);
    if (mipData.IsInvalid())
    {
        LOG(Error, "Failed to get texture mip data.");
        return true;
    }
    if (mipData.Length() != slicePitch * _texture.TotalArraySize())
    {
        LOG(Error, "Invalid custom texture data (slice pitch * array size is different than data bytes count).");
        return true;
    }

    // Fill result
    result.RowPitch = rowPitch;
    result.DepthPitch = slicePitch;
    result.Lines = Math::Max(1, Height() >> mipIndex);
    if (copyData)
        result.Data.Copy(mipData.Get() + (arrayIndex * slicePitch), slicePitch);
    else
        result.Data.Link(mipData.Get() + (arrayIndex * slicePitch), slicePitch);
    return false;
}

bool TextureBase::GetPixels(Array<Color32>& pixels, int32 mipIndex, int32 arrayIndex)
{
    PROFILE_CPU_NAMED("Texture.GetPixels");
    ScopeLock lock(Locker);

    // Get mip data
    auto dataLock = LockData();
    TextureMipData mipData;
    if (GetTextureMipData(mipData, mipIndex, arrayIndex, false))
        return true;
    const int32 mipWidth = Math::Max(1, Width() >> mipIndex);
    const int32 mipHeight = Math::Max(1, Height() >> mipIndex);

    // Convert into pixels
    return mipData.GetPixels(pixels, mipWidth, mipHeight, Format());
}

bool TextureBase::GetPixels(Array<Color>& pixels, int32 mipIndex, int32 arrayIndex)
{
    PROFILE_CPU_NAMED("Texture.GetPixels");

    // Get mip data
    auto dataLock = LockData();
    TextureMipData mipData;
    if (GetTextureMipData(mipData, mipIndex, arrayIndex, false))
        return true;
    const int32 mipWidth = Math::Max(1, Width() >> mipIndex);
    const int32 mipHeight = Math::Max(1, Height() >> mipIndex);

    // Convert into pixels
    return mipData.GetPixels(pixels, mipWidth, mipHeight, Format());
}

bool TextureBase::SetPixels(const Span<Color32>& pixels, int32 mipIndex, int32 arrayIndex, bool generateMips)
{
    PROFILE_CPU_NAMED("Texture.SetPixels");
    if (!IsVirtual())
    {
        LOG(Error, "Texture must be virtual.");
        return true;
    }
    ScopeLock lock(Locker);
    if (_customData == nullptr || Width() == 0)
    {
        LOG(Error, "Texture must be initialized.");
        return true;
    }
    const PixelFormat format = Format();
    const int32 width = Math::Max(1, Width() >> mipIndex);
    const int32 height = Math::Max(1, Height() >> mipIndex);
    auto& mipData = _customData->Mips[mipIndex];
    const int32 rowPitch = mipData.RowPitch;
    const int32 sliceSize = mipData.SlicePitch;
    if (pixels.Length() != width * height)
    {
        Log::ArgumentOutOfRangeException();
        return true;
    }

    // Convert pixels to the texture format
    ASSERT(mipData.Data.IsAllocated());
    byte* dst = mipData.Data.Get() + sliceSize * arrayIndex;
    bool error = true;
    switch (format)
    {
    case PixelFormat::R8G8B8A8_SInt:
    case PixelFormat::R8G8B8A8_Typeless:
    case PixelFormat::R8G8B8A8_SNorm:
    case PixelFormat::R8G8B8A8_UInt:
    case PixelFormat::R8G8B8A8_UNorm:
    case PixelFormat::R8G8B8A8_UNorm_sRGB:
    case PixelFormat::R8G8_B8G8_UNorm:
    case PixelFormat::B8G8R8A8_Typeless:
    case PixelFormat::B8G8R8A8_UNorm:
    case PixelFormat::B8G8R8A8_UNorm_sRGB:
    case PixelFormat::B8G8R8X8_Typeless:
    case PixelFormat::B8G8R8X8_UNorm:
    case PixelFormat::B8G8R8X8_UNorm_sRGB:
        if (rowPitch == width * sizeof(Color32))
        {
            Platform::MemoryCopy(dst, pixels.Get(), sliceSize);
            error = false;
        }
        break;
    }
    if (error)
    {
        // Try to use texture sampler utility
        auto sampler = PixelFormatSampler::Get(format);
        if (sampler)
        {
            for (int32 y = 0; y < height; y++)
            {
                for (int32 x = 0; x < width; x++)
                {
                    Color c(pixels.Get()[x + y * width]);
                    sampler->Store(dst, x, y, rowPitch, c);
                }
            }
            error = false;
        }
    }
    if (error)
    {
        LOG(Error, "Unsupported texture data format {0}.", ScriptingEnum::ToString(format));
        return true;
    }

    // Generate mips optionally
    if (generateMips && mipIndex + 1 < _customData->Mips.Count())
    {
        for (int32 i = mipIndex + 1; i < _customData->Mips.Count(); i++)
            _customData->GenerateMip(i);
    }

    // Request texture data streaming to GPU
    _texture.GetTexture()->SetResidentMipLevels(0);
    _texture.RequestStreamingUpdate();

    return false;
}

bool TextureBase::SetPixels(const Span<Color>& pixels, int32 mipIndex, int32 arrayIndex, bool generateMips)
{
    PROFILE_CPU_NAMED("Texture.SetPixels");
    if (!IsVirtual())
    {
        LOG(Error, "Texture must be virtual.");
        return true;
    }
    ScopeLock lock(Locker);
    if (_customData == nullptr || Width() == 0)
    {
        LOG(Error, "Texture must be initialized.");
        return true;
    }
    const PixelFormat format = Format();
    const int32 width = Math::Max(1, Width() >> mipIndex);
    const int32 height = Math::Max(1, Height() >> mipIndex);
    auto& mipData = _customData->Mips[mipIndex];
    const int32 rowPitch = mipData.RowPitch;
    const int32 sliceSize = mipData.SlicePitch;
    if (pixels.Length() != width * height)
    {
        Log::ArgumentOutOfRangeException();
        return true;
    }

    // Convert pixels to the texture format
    ASSERT(mipData.Data.IsAllocated());
    byte* dst = mipData.Data.Get() + sliceSize * arrayIndex;
    bool error = true;
    switch (format)
    {
    case PixelFormat::R32G32B32A32_Typeless:
    case PixelFormat::R32G32B32A32_Float:
        if (rowPitch == width * sizeof(Color))
        {
            Platform::MemoryCopy(dst, pixels.Get(), sliceSize);
            error = false;
        }
        break;
    }
    if (error)
    {
        // Try to use texture sampler utility
        auto sampler = PixelFormatSampler::Get(format);
        if (sampler)
        {
            for (int32 y = 0; y < height; y++)
            {
                for (int32 x = 0; x < width; x++)
                {
                    Color c(pixels.Get()[x + y * width]);
                    sampler->Store(dst, x, y, rowPitch, c);
                }
            }
            error = false;
        }
    }
    if (error)
    {
        LOG(Error, "Unsupported texture data format {0}.", ScriptingEnum::ToString(format));
        return true;
    }

    // Generate mips optionally
    if (generateMips && mipIndex + 1 < _customData->Mips.Count())
    {
        for (int32 i = mipIndex + 1; i < _customData->Mips.Count(); i++)
            _customData->GenerateMip(i);
    }

    // Request texture data streaming to GPU
    _texture.GetTexture()->SetResidentMipLevels(0);
    _texture.RequestStreamingUpdate();

    return false;
}

bool TextureBase::Init(InitData* initData)
{
    // Validate state
    if (!IsVirtual())
    {
        LOG(Error, "Texture must be virtual.");
        Delete(initData);
        return true;
    }
    if (initData->Format == PixelFormat::Unknown ||
        Math::IsNotInRange(initData->Width, 1, GPU_MAX_TEXTURE_SIZE) ||
        Math::IsNotInRange(initData->Height, 1, GPU_MAX_TEXTURE_SIZE) ||
        Math::IsNotInRange(initData->ArraySize, 1, GPU_MAX_TEXTURE_ARRAY_SIZE) ||
        Math::IsNotInRange(initData->Mips.Count(), 1, GPU_MAX_TEXTURE_MIP_LEVELS))
    {
        Log::ArgumentOutOfRangeException();
        Delete(initData);
        return true;
    }
    ScopeLock lock(Locker);

    // Release texture
    _texture.UnloadTexture();

    // Prepare descriptor
    if (_customData != nullptr)
        Delete(_customData);
    _customData = initData;

    // Create texture
    TextureHeader textureHeader;
    textureHeader.Format = _customData->Format;
    textureHeader.Width = _customData->Width;
    textureHeader.Height = _customData->Height;
    textureHeader.IsCubeMap = _customData->ArraySize == 6;
    textureHeader.MipLevels = _customData->Mips.Count();
    textureHeader.Type = TextureFormatType::ColorRGBA;
    textureHeader.NeverStream = true;
    if (_texture.Create(textureHeader))
    {
        LOG(Warning, "Cannot initialize texture.");
        return true;
    }

    // Speed up update texture uplaod to GPU by alocating texture async task now to run before next frame rendering
    Streaming::UpdateResource(&_texture);

    return false;
}

#if !COMPILE_WITHOUT_CSHARP

bool TextureBase::InitCSharp(void* ptr)
{
    PROFILE_CPU_NAMED("Texture.Init");
    struct InternalInitData
    {
        PixelFormat Format;
        int32 Width;
        int32 Height;
        int32 ArraySize;
        int32 MipLevels;
        int32 GenerateMips;
        int32 DataRowPitch[14];
        int32 DataSlicePitch[14];
        byte* Data[14];
    };
    auto initDataObj = (InternalInitData*)ptr;
    auto initData = New<InitData>();

    initData->Format = initDataObj->Format;
    initData->Width = initDataObj->Width;
    initData->Height = initDataObj->Height;
    initData->ArraySize = initDataObj->ArraySize;
    initData->Mips.Resize(initDataObj->GenerateMips ? MipLevelsCount(initDataObj->Width, initDataObj->Height) : initDataObj->MipLevels);

    // Copy source mips data
    for (int32 mipIndex = 0; mipIndex < initDataObj->MipLevels; mipIndex++)
    {
        auto& mip = initData->Mips[mipIndex];
        mip.RowPitch = initDataObj->DataRowPitch[mipIndex];
        mip.SlicePitch = initDataObj->DataSlicePitch[mipIndex];
        mip.Data.Copy(initDataObj->Data[mipIndex], mip.SlicePitch * initData->ArraySize);
    }

    // Generate mips
    for (int32 mipIndex = initDataObj->MipLevels; mipIndex < initData->Mips.Count(); mipIndex++)
    {
        initData->GenerateMip(mipIndex, initDataObj->GenerateMips & 2);
    }

    return Init(initData);
}

TextureData* TextureBase::GetTextureData()
{
    auto result = New<TextureData>();
    if (GetTextureData(*result, false))
    {
        Delete(result);
        result = nullptr;
    }
    return result;
}

#endif

uint64 TextureBase::GetMemoryUsage() const
{
    Locker.Lock();
    uint64 result = BinaryAsset::GetMemoryUsage();
    result += sizeof(TextureBase) - sizeof(BinaryAsset);
    if (_customData)
    {
        result += sizeof(InitData);
        for (auto& mip : _customData->Mips)
            result += mip.Data.Length();
    }
    Locker.Unlock();
    return result;
}

void TextureBase::CancelStreaming()
{
    Asset::CancelStreaming();
    _texture.CancelStreamingTasks();
}

int32 TextureBase::CalculateChunkIndex(int32 mipIndex) const
{
    // Mips are in 0-13 chunks
    return mipIndex;
}

CriticalSection& TextureBase::GetOwnerLocker() const
{
    return _parent->Locker;
}

void TextureBase::unload(bool isReloading)
{
    if (!isReloading)
    {
        // Release texture
        _texture.UnloadTexture();
        SAFE_DELETE(_customData);
    }
}

Task* TextureBase::RequestMipDataAsync(int32 mipIndex)
{
    if (_customData)
        return nullptr;

    auto chunkIndex = CalculateChunkIndex(mipIndex);
    return (Task*)_parent->RequestChunkDataAsync(chunkIndex);
}

FlaxStorage::LockData TextureBase::LockData()
{
    return _parent->Storage ? _parent->Storage->Lock() : FlaxStorage::LockData();
}

void TextureBase::GetMipData(int32 mipIndex, BytesContainer& data, uint32& rowPitch, uint32& slicePitch, bool canLoad) const
{
    if (Format() == PixelFormat::Unknown)
    {
        data.Release();
        rowPitch = slicePitch = 0;
        return;
    }
    if (_customData)
    {
        auto& mip = _customData->Mips[mipIndex];
        data.Link(mip.Data);
        rowPitch = mip.RowPitch;
        slicePitch = mip.SlicePitch;
        return;
    }

    int32 chunkIndex = CalculateChunkIndex(mipIndex);
    if (canLoad)
        _parent->LoadChunk(chunkIndex);
    _parent->GetChunkData(chunkIndex, data);
    const int32 mipWidth = Math::Max(1, Width() >> mipIndex);
    const int32 mipHeight = Math::Max(1, Height() >> mipIndex);
    RenderTools::ComputePitch(Format(), mipWidth, mipHeight, rowPitch, slicePitch);
}

bool TextureBase::init(AssetInitData& initData)
{
    if (IsVirtual())
        return false;

    // Get texture header for asset custom data (fast access)
    TextureHeader textureHeader;
    if (initData.CustomData.Length() == sizeof(TextureHeader))
    {
        Platform::MemoryCopy(&textureHeader, initData.CustomData.Get(), sizeof(textureHeader));
    }
    else if (initData.CustomData.Length() == sizeof(TextureHeader_Deprecated))
    {
        textureHeader = TextureHeader(*(TextureHeader_Deprecated*)initData.CustomData.Get());
    }
    else
    {
        LOG(Error, "Missing texture header.");
        return true;
    }

    return _texture.Create(textureHeader);
}

Asset::LoadResult TextureBase::load()
{
    // Loading textures is very fast xD
    return LoadResult::Ok;
}

TextureBase::InitData::MipData::MipData(MipData&& other) noexcept
    : Data(MoveTemp(other.Data))
    , RowPitch(other.RowPitch)
    , SlicePitch(other.SlicePitch)
{
}

TextureBase::InitData::InitData(InitData&& other) noexcept
    : Format(other.Format)
    , Width(other.Width)
    , Height(other.Height)
    , ArraySize(other.ArraySize)
    , Mips(MoveTemp(other.Mips))
{
}

bool TextureBase::InitData::GenerateMip(int32 mipIndex, bool linear)
{
    if (mipIndex < 1 || mipIndex >= Mips.Count())
    {
        LOG(Warning, "Invalid mip map to generate.");
        return true;
    }
    if (ArraySize < 1)
    {
        LOG(Warning, "Invalid array size.");
        return true;
    }
    if (PixelFormatExtensions::IsCompressed(Format))
    {
        LOG(Warning, "Cannot generate mip map for compressed format data.");
        return true;
    }
    const auto& srcMip = Mips[mipIndex - 1];
    auto& dstMip = Mips[mipIndex];
    if (srcMip.RowPitch == 0 || srcMip.SlicePitch == 0 || srcMip.Data.IsInvalid())
    {
        LOG(Warning, "Missing data for source mip map.");
        return true;
    }
    PROFILE_CPU_NAMED("Texture.GenerateMip");

    // Allocate data
    const int32 dstMipWidth = Math::Max(1, Width >> mipIndex);
    const int32 dstMipHeight = Math::Max(1, Height >> mipIndex);
    const int32 pixelStride = PixelFormatExtensions::SizeInBytes(Format);
    dstMip.RowPitch = dstMipWidth * pixelStride;
    dstMip.SlicePitch = dstMip.RowPitch * dstMipHeight;
    dstMip.Data.Allocate(dstMip.SlicePitch * ArraySize);

    // Perform filtering
    const int32 width = Math::Max(1, Width >> (mipIndex - 1));
    const int32 height = Math::Max(1, Height >> (mipIndex - 1));
    return TextureTool::GenerateMip(width, height, Format, srcMip.Data, srcMip.RowPitch, dstMip.Data, dstMip.RowPitch, linear, ArraySize);
}

void TextureBase::InitData::FromTextureData(const TextureData& textureData, bool generateMips)
{
    Format = textureData.Format;
    Width = textureData.Width;
    Height = textureData.Height;
    ArraySize = textureData.GetArraySize();
    if (generateMips)
        Mips.Resize(MipLevelsCount(textureData.Width, textureData.Height));
    else
        Mips.Resize(textureData.GetMipLevels());

    for (int32 mipIndex = 0; mipIndex < textureData.GetMipLevels(); mipIndex++)
    {
        auto& mip = Mips[mipIndex];
        auto& data = *textureData.GetData(0, mipIndex);
        mip.Data.Allocate(data.Data.Length() * ArraySize);
        mip.RowPitch = data.RowPitch;
        mip.SlicePitch = data.Data.Length();

        byte* dst = mip.Data.Get();
        for (int32 arrayIndex = 0; arrayIndex < ArraySize; arrayIndex++)
        {
            auto& d = *textureData.GetData(arrayIndex, mipIndex);
            ASSERT(data.RowPitch == d.RowPitch);
            ASSERT(data.Data.Length() == d.Data.Length());
            Platform::MemoryCopy(dst, d.Data.Get(), d.Data.Length());
            dst += data.Data.Length();
            ASSERT((int32)(dst - mip.Data.Get()) <= mip.Data.Length());
        }
    }

    if (generateMips)
    {
        for (int32 mipIndex = textureData.GetMipLevels(); mipIndex < Mips.Count(); mipIndex++)
        {
            GenerateMip(mipIndex, true);
        }
    }
}
