// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "FontTextureAtlas.h"
#include "Engine/Core/Types/DataContainer.h"
#include "Engine/Content/Factories/BinaryAssetFactory.h"
#include "Engine/Graphics/PixelFormat.h"
#include "Engine/Graphics/PixelFormatExtensions.h"
#include "Engine/Graphics/GPUDevice.h"

REGISTER_BINARY_ASSET(FontTextureAtlas, "FlaxEngine.FontTextureAtlas", nullptr, true);

FontTextureAtlas::FontTextureAtlas(const SpawnParams& params, const AssetInfo* info)
    : Texture(params, info)
    , _width(0)
    , _height(0)
    , _isDirty(true)
    , _root(nullptr)
{
}

uint32 FontTextureAtlas::GetPaddingAmount() const
{
    return _paddingStyle == NoPadding ? 0 : 1;
}

void FontTextureAtlas::Setup(PixelFormat format, PaddingStyle paddingStyle)
{
    _format = format;
    _bytesPerPixel = PixelFormatExtensions::SizeInBytes(format);
    _paddingStyle = paddingStyle;
}

void FontTextureAtlas::Init(uint32 width, uint32 height)
{
    ASSERT(_root == nullptr);

    // Setup
    uint32 padding = GetPaddingAmount();
    _width = width;
    _height = height;
    _root = New<Slot>(padding, padding, _width - padding, _height - padding);
    _isDirty = false;

    // Reserve upload data memory
    _data.Resize(_width * _height * _bytesPerPixel, false);
    Platform::MemoryClear(_data.Get(), _data.Capacity());
}

FontTextureAtlas::Slot* FontTextureAtlas::AddEntry(uint32 targetWidth, uint32 targetHeight, const Array<byte>& data)
{
    // Check for invalid size
    if (targetWidth == 0 || targetHeight == 0)
        return nullptr;

    // Try to find slot for the texture
    Slot* slot = nullptr;
    const uint32 padding = GetPaddingAmount();
    const uint32 allPadding = padding * 2;
    for (int32 i = 0; i < _freeSlots.Count(); i++)
    {
        Slot* e = _freeSlots[i];
        if (e->Width == targetWidth + allPadding && e->Height == targetHeight + allPadding)
        {
            slot = e;
            _freeSlots.RemoveAt(i);
            break;
        }
    }
    if (!slot)
    {
        slot = _root->Insert(targetWidth, targetHeight, GetPaddingAmount() * 2);
    }

    // Check if can fit it
    if (slot)
    {
        // Copy data to into the atlas memory
        CopyDataIntoSlot(slot, data);

        // Set dirty state
        markAsDirty();
    }

    // Returns result
    return slot;
}

bool FontTextureAtlas::Invalidate(uint32 x, uint32 y, uint32 width, uint32 height)
{
    Slot* slot = invalidate(_root, x, y, width, height);
    if (slot)
    {
        _freeSlots.Add(slot);
    }
    return slot != nullptr;
}

void FontTextureAtlas::CopyDataIntoSlot(const Slot* slot, const Array<byte>& data)
{
    uint8* start = &_data[slot->Y * _width * _bytesPerPixel + slot->X * _bytesPerPixel];
    const uint32 padding = GetPaddingAmount();
    const uint32 allPadding = padding * 2;
    const uint32 srcWidth = slot->Width - allPadding;
    const uint32 srcHeight = slot->Height - allPadding;

    RowData rowData;
    rowData.DstData = start;
    rowData.SrcData = data.Get();
    rowData.DstTextureWidth = _width;
    rowData.SrcTextureWidth = srcWidth;
    rowData.RowWidth = slot->Width;

    // Start with padding
    if (padding > 0)
    {
        rowData.SrcRow = 0;
        rowData.DstRow = 0;

        if (_paddingStyle == DilateBorder)
        {
            copyRow(rowData);
        }
        else
        {
            zeroRow(rowData);
        }
    }

    // Copy each row of the texture
    for (uint32 row = padding; row < slot->Height - padding; row++)
    {
        rowData.SrcRow = row - padding;
        rowData.DstRow = row;

        copyRow(rowData);
    }

    // Finish with padding
    if (padding > 0)
    {
        rowData.SrcRow = srcHeight - 1;
        rowData.DstRow = slot->Height - padding;
        if (_paddingStyle == DilateBorder)
            copyRow(rowData);
        else
            zeroRow(rowData);
    }
}

void FontTextureAtlas::copyRow(const RowData& copyRowData) const
{
    const byte* data = copyRowData.SrcData;
    byte* start = copyRowData.DstData;
    const uint32 srdWidth = copyRowData.SrcTextureWidth;
    const uint32 dstWidth = copyRowData.DstTextureWidth;
    const uint32 srcRow = copyRowData.SrcRow;
    const uint32 dstRow = copyRowData.DstRow;
    const uint32 padding = GetPaddingAmount();

    const byte* srcData = &data[srcRow * srdWidth * _bytesPerPixel];
    byte* dstData = &start[(dstRow * dstWidth + padding) * _bytesPerPixel];
    Platform::MemoryCopy(dstData, srcData, srdWidth * _bytesPerPixel);

    if (padding > 0)
    {
        byte* dstPaddingPixelLeft = &start[dstRow * dstWidth * _bytesPerPixel];
        byte* dstPaddingPixelRight = dstPaddingPixelLeft + (copyRowData.RowWidth - 1) * _bytesPerPixel;
        if (_paddingStyle == DilateBorder)
        {
            const byte* firstPixel = srcData;
            const byte* lastPixel = srcData + (srdWidth - 1) * _bytesPerPixel;
            Platform::MemoryCopy(dstPaddingPixelLeft, firstPixel, _bytesPerPixel);
            Platform::MemoryCopy(dstPaddingPixelRight, lastPixel, _bytesPerPixel);
        }
        else
        {
            Platform::MemoryClear(dstPaddingPixelLeft, _bytesPerPixel);
            Platform::MemoryClear(dstPaddingPixelRight, _bytesPerPixel);
        }
    }
}

void FontTextureAtlas::zeroRow(const RowData& copyRowData) const
{
    const uint32 dstWidth = copyRowData.DstTextureWidth;
    const uint32 dstRow = copyRowData.DstRow;
    byte* dstData = &copyRowData.DstData[dstRow * dstWidth * _bytesPerPixel];
    Platform::MemoryClear(dstData, copyRowData.RowWidth * _bytesPerPixel);
}

void FontTextureAtlas::unload(bool isReloading)
{
    Texture::unload(isReloading);

    Clear();
    _data.Clear();
}

void FontTextureAtlas::Clear()
{
    SAFE_DELETE(_root);
    _freeSlots.Clear();
}

void FontTextureAtlas::Flush()
{
    if (_isDirty)
    {
        EnsureTextureCreated();

        // Upload data to the GPU
        BytesContainer data;
        data.Link(_data);
        _texture->UploadMipMapAsync(data, 0)->Start();

        // Clear dirty flag
        _isDirty = false;
    }
}

void FontTextureAtlas::EnsureTextureCreated() const
{
    if (_texture->IsAllocated() == false)
    {
        // Initialize atlas texture
        if (_texture->Init(GPUTextureDescription::New2D(_width, _height, 1, _format, GPUTextureFlags::ShaderResource)))
        {
            LOG(Warning, "Cannot initialize font atlas texture.");
        }
    }
}

bool FontTextureAtlas::HasDataSyncWithGPU() const
{
    return _isDirty == false;
}

FontTextureAtlas::Slot* FontTextureAtlas::invalidate(Slot* parent, uint32 x, uint32 y, uint32 width, uint32 height)
{
    if (parent->X == x && parent->Y == y && parent->Width == width && parent->Height == height)
    {
        return parent;
    }
    Slot* result = parent->Left ? invalidate(parent->Left, x, y, width, height) : nullptr;
    if (result)
        return result;
    return parent->Right ? invalidate(parent->Right, x, y, width, height) : nullptr;
}

void FontTextureAtlas::markAsDirty()
{
    _isDirty = true;
}
