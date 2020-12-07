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
    return (_paddingStyle == NoPadding ? 0 : 1);
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
    // Copy pixel data to the texture
    uint8* start = &_data[slot->Y * _width * _bytesPerPixel + slot->X * _bytesPerPixel];

    // Account for same padding on each sides
    const uint32 padding = GetPaddingAmount();
    const uint32 allPadding = padding * 2;

    // The width of the source texture without padding (actual width)
    const uint32 sourceWidth = slot->Width - allPadding;
    const uint32 sourceHeight = slot->Height - allPadding;

    CopyRowData copyRowData;
    copyRowData.DestData = start;
    copyRowData.SrcData = data.Get();
    copyRowData.DestTextureWidth = _width;
    copyRowData.SrcTextureWidth = sourceWidth;
    copyRowData.RowWidth = slot->Width;

    // Apply the padding for bilinear filtering
    // Not used if no padding (assumes sampling outside boundaries of the sub texture is not possible)
    if (padding > 0)
    {
        // Copy first color row into padding. 
        copyRowData.SrcRow = 0;
        copyRowData.DestRow = 0;

        if (_paddingStyle == DilateBorder)
        {
            copyRow(copyRowData);
        }
        else
        {
            zeroRow(copyRowData);
        }
    }

    // Copy each row of the texture
    for (uint32 row = padding; row < slot->Height - padding; row++)
    {
        copyRowData.SrcRow = row - padding;
        copyRowData.DestRow = row;

        copyRow(copyRowData);
    }

    if (padding > 0)
    {
        // Copy last color row into padding row for bilinear filtering
        copyRowData.SrcRow = sourceHeight - 1;
        copyRowData.DestRow = slot->Height - padding;

        if (_paddingStyle == DilateBorder)
        {
            copyRow(copyRowData);
        }
        else
        {
            zeroRow(copyRowData);
        }
    }
}

void FontTextureAtlas::copyRow(const CopyRowData& copyRowData) const
{
    const byte* data = copyRowData.SrcData;
    byte* start = copyRowData.DestData;
    const uint32 sourceWidth = copyRowData.SrcTextureWidth;
    const uint32 destWidth = copyRowData.DestTextureWidth;
    const uint32 srcRow = copyRowData.SrcRow;
    const uint32 destRow = copyRowData.DestRow;
    const uint32 padding = GetPaddingAmount();

    const byte* sourceDataAddr = &data[(srcRow * sourceWidth) * _bytesPerPixel];
    byte* destDataAddr = &start[(destRow * destWidth + padding) * _bytesPerPixel];
    Platform::MemoryCopy(destDataAddr, sourceDataAddr, sourceWidth * _bytesPerPixel);

    if (padding > 0)
    {
        byte* destPaddingPixelLeft = &start[(destRow * destWidth) * _bytesPerPixel];
        byte* destPaddingPixelRight = destPaddingPixelLeft + ((copyRowData.RowWidth - 1) * _bytesPerPixel);
        if (_paddingStyle == DilateBorder)
        {
            const byte* firstPixel = sourceDataAddr;
            const byte* lastPixel = sourceDataAddr + ((sourceWidth - 1) * _bytesPerPixel);
            Platform::MemoryCopy(destPaddingPixelLeft, firstPixel, _bytesPerPixel);
            Platform::MemoryCopy(destPaddingPixelRight, lastPixel, _bytesPerPixel);
        }
        else
        {
            Platform::MemoryClear(destPaddingPixelLeft, _bytesPerPixel);
            Platform::MemoryClear(destPaddingPixelRight, _bytesPerPixel);
        }
    }
}

void FontTextureAtlas::zeroRow(const CopyRowData& copyRowData) const
{
    const uint32 destWidth = copyRowData.DestTextureWidth;
    const uint32 destRow = copyRowData.DestRow;
    byte* destData = &copyRowData.DestData[destRow * destWidth * _bytesPerPixel];
    Platform::MemoryClear(destData, copyRowData.RowWidth * _bytesPerPixel);
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
