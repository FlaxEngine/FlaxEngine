// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "GPUBufferDescription.h"
#include "PixelFormatExtensions.h"
#include "Engine/Core/Types/String.h"

void GPUBufferDescription::Clear()
{
    Platform::MemoryClear(this, sizeof(GPUBufferDescription));
}

GPUBufferDescription GPUBufferDescription::Buffer(uint32 size, GPUBufferFlags flags, PixelFormat format, const void* initData, uint32 stride, GPUResourceUsage usage)
{
    GPUBufferDescription desc;
    desc.Size = size;
    desc.Stride = stride;
    desc.Flags = flags;
    desc.Format = format;
    desc.InitData = initData;
    desc.Usage = usage;
    return desc;
}

GPUBufferDescription GPUBufferDescription::Typed(int32 count, PixelFormat viewFormat, bool isUnorderedAccess, GPUResourceUsage usage)
{
    auto bufferFlags = GPUBufferFlags::ShaderResource;
    if (isUnorderedAccess)
        bufferFlags |= GPUBufferFlags::UnorderedAccess;
    const auto stride = PixelFormatExtensions::SizeInBytes(viewFormat);
    return Buffer(count * stride, bufferFlags, viewFormat, nullptr, stride, usage);
}

GPUBufferDescription GPUBufferDescription::Typed(const void* data, int32 count, PixelFormat viewFormat, bool isUnorderedAccess, GPUResourceUsage usage)
{
    auto bufferFlags = GPUBufferFlags::ShaderResource;
    if (isUnorderedAccess)
        bufferFlags |= GPUBufferFlags::UnorderedAccess;
    const auto stride = PixelFormatExtensions::SizeInBytes(viewFormat);
    return Buffer(count * stride, bufferFlags, viewFormat, data, stride, usage);
}

bool GPUBufferDescription::Equals(const GPUBufferDescription& other) const
{
    return Size == other.Size
            && Stride == other.Stride
            && Flags == other.Flags
            && Format == other.Format
            && Usage == other.Usage
            && InitData == other.InitData;
}

String GPUBufferDescription::ToString() const
{
    // TODO: add tool to Format to string

    String flags;
    if (Flags == GPUBufferFlags::None)
    {
        flags = TEXT("None");
    }
    else
    {
        // TODO: create tool to auto convert flag enums to string

#define CONVERT_FLAGS_FLAGS_2_STR(value) if(Flags & GPUBufferFlags::value) { if (flags.HasChars()) flags += TEXT('|'); flags += TEXT(#value); }
        CONVERT_FLAGS_FLAGS_2_STR(ShaderResource);
        CONVERT_FLAGS_FLAGS_2_STR(VertexBuffer);
        CONVERT_FLAGS_FLAGS_2_STR(IndexBuffer);
        CONVERT_FLAGS_FLAGS_2_STR(UnorderedAccess);
        CONVERT_FLAGS_FLAGS_2_STR(Append);
        CONVERT_FLAGS_FLAGS_2_STR(Counter);
        CONVERT_FLAGS_FLAGS_2_STR(Argument);
        CONVERT_FLAGS_FLAGS_2_STR(Structured);
#undef CONVERT_FLAGS_FLAGS_2_STR
    }

    return String::Format(TEXT("Size: {0}, Stride: {1}, Flags: {2}, Format: {3}, Usage: {4}"),
                          Size,
                          Stride,
                          flags,
                          (int32)Format,
                          (int32)Usage);
}

uint32 GetHash(const GPUBufferDescription& key)
{
    uint32 hashCode = key.Size;
    hashCode = (hashCode * 397) ^ key.Stride;
    hashCode = (hashCode * 397) ^ (uint32)key.Flags;
    hashCode = (hashCode * 397) ^ (uint32)key.Format;
    hashCode = (hashCode * 397) ^ (uint32)key.Usage;
    return hashCode;
}
