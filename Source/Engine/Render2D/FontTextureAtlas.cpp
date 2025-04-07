// Copyright (c) Wojciech Figat. All rights reserved.

#include "FontTextureAtlas.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/DataContainer.h"
#include "Engine/Content/Factories/BinaryAssetFactory.h"
#include "Engine/Graphics/PixelFormat.h"
#include "Engine/Graphics/PixelFormatExtensions.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Graphics/Async/GPUTask.h"

REGISTER_BINARY_ASSET(FontTextureAtlas, "FlaxEngine.FontTextureAtlas", true);

FontTextureAtlas::FontTextureAtlas(const SpawnParams& params, const AssetInfo* info)
    : Texture(params, info)
    , _width(0)
    , _height(0)
    , _isDirty(true)
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
    // Setup
    _width = width;
    _height = height;
    const uint32 padding = GetPaddingAmount() * 2; // Double the padding so each slot has own border around it
    _atlas.Init(_width, _height, padding);
    _isDirty = false;

    // Reserve upload data memory
    _data.Resize(_width * _height * _bytesPerPixel, false);
    Platform::MemoryClear(_data.Get(), _data.Capacity());
}

FontTextureAtlasSlot* FontTextureAtlas::AddEntry(uint32 width, uint32 height, const Array<byte>& data)
{
    if (width == 0 || height == 0)
        return nullptr;

    // Try to find slot for the texture
    FontTextureAtlasSlot* slot = nullptr;
    for (int32 i = 0; i < _freeSlots.Count(); i++)
    {
        FontTextureAtlasSlot* e = _freeSlots[i];
        if (e->Width == width && e->Height == height)
        {
            slot = e;
            _freeSlots.RemoveAt(i);
            break;
        }
    }
    if (!slot)
    {
        slot = _atlas.Insert(width, height);
    }

    if (slot)
    {
        // Copy data to into the atlas memory
        CopyDataIntoSlot(slot, data);
        _isDirty = true;
    }

    return slot;
}

bool FontTextureAtlas::Invalidate(const FontTextureAtlasSlot* slot)
{
    if (slot)
    {
        // Push back to free slots list to be used on the next insert (in theory slot handle is still valid but we keep it free)
        _freeSlots.AddUnique((FontTextureAtlasSlot*)slot);
        return true;
    }
    return false;
}

bool FontTextureAtlas::Invalidate(uint32 x, uint32 y, uint32 width, uint32 height)
{
    for (const FontTextureAtlasSlot& node : _atlas.Nodes)
    {
        if (node.X == x && node.Y == y && node.Width == width && node.Height == height)
            return Invalidate(&node);
    }
    return false;
}

void FontTextureAtlas::CopyDataIntoSlot(const FontTextureAtlasSlot* slot, const Array<byte>& data)
{
    RowData rowData;
    rowData.DstData = _data.Get() + (slot->Y * _width + slot->X) * _bytesPerPixel;
    rowData.SrcData = data.Get();
    rowData.DstWidth = _width;
    rowData.SrcWidth = slot->Width;
    rowData.Padding = GetPaddingAmount();

    // Start with padding
    if (rowData.Padding > 0)
    {
        rowData.SrcRow = 0;
        rowData.DstRow = -1;
        if (_paddingStyle == DilateBorder)
            copyRow(rowData);
        else
            zeroRow(rowData);
    }

    // Actual data copy
    for (uint32 row = 0; row < slot->Height; row++)
    {
        rowData.SrcRow = row;
        rowData.DstRow = row;
        copyRow(rowData);
    }

    // Finish with padding
    if (rowData.Padding > 0)
    {
        rowData.SrcRow = slot->Height - 1;
        rowData.DstRow = slot->Height;
        if (_paddingStyle == DilateBorder)
            copyRow(rowData);
        else
            zeroRow(rowData);
    }
}

byte* FontTextureAtlas::GetSlotData(const FontTextureAtlasSlot* slot, uint32& width, uint32& height, uint32& stride)
{
    width = slot->Width;
    height = slot->Height;
    stride = _width * _bytesPerPixel;
    return &_data[slot->Y * _width * _bytesPerPixel + slot->X * _bytesPerPixel];
}

void FontTextureAtlas::copyRow(const RowData& copyRowData) const
{
    const byte* srcData = (const byte*)((intptr)copyRowData.SrcData + (intptr)copyRowData.SrcRow * copyRowData.SrcWidth * _bytesPerPixel);
    byte* dstData = (byte*)((intptr)copyRowData.DstData + (intptr)copyRowData.DstRow * copyRowData.DstWidth * _bytesPerPixel);
    Platform::MemoryCopy(dstData, srcData, copyRowData.SrcWidth * _bytesPerPixel);

    if (copyRowData.Padding > 0)
    {
        const uint32 padSize = copyRowData.Padding * _bytesPerPixel;
        byte* dstPaddingPixelLeft = (byte*)((intptr)copyRowData.DstData + (intptr)copyRowData.DstRow * copyRowData.DstWidth * _bytesPerPixel - padSize);
        byte* dstPaddingPixelRight = dstPaddingPixelLeft + copyRowData.SrcWidth * _bytesPerPixel + padSize;
        if (_paddingStyle == DilateBorder)
        {
            // Dilate left and right sides of the padded row
            const byte* firstPixel = srcData;
            const byte* lastPixel = srcData + (copyRowData.SrcWidth - 1) * _bytesPerPixel;
            Platform::MemoryCopy(dstPaddingPixelLeft, firstPixel, padSize);
            Platform::MemoryCopy(dstPaddingPixelRight, lastPixel, padSize);
        }
        else
        {
            // Clear left and right sides of the padded row
            Platform::MemoryClear(dstPaddingPixelLeft, padSize);
            Platform::MemoryClear(dstPaddingPixelRight, padSize);
        }
    }
}

void FontTextureAtlas::zeroRow(const RowData& copyRowData) const
{
    byte* dstData = (byte*)((intptr)copyRowData.DstData + (intptr)copyRowData.DstRow * copyRowData.DstWidth * _bytesPerPixel);
    uint32 dstSize = copyRowData.SrcWidth * _bytesPerPixel;
    if (copyRowData.Padding > 0)
    {
        // Extend clear by left and right borders of the padded row
        const uint32 padSize = copyRowData.Padding * _bytesPerPixel;
        dstData -= padSize;
        dstSize += padSize * 2;
    }
    Platform::MemoryClear(dstData, dstSize);
}

void FontTextureAtlas::unload(bool isReloading)
{
    Texture::unload(isReloading);

    Clear();
    _data.Resize(0);
}

void FontTextureAtlas::Clear()
{
    _freeSlots.Clear();
    _atlas.Clear();
}

void FontTextureAtlas::Flush()
{
    if (_isDirty)
    {
        EnsureTextureCreated();

        // Upload data to the GPU
        BytesContainer data;
        data.Link(_data);
        auto task = _texture->UploadMipMapAsync(data, 0);
        if (task)
            task->Start();

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
