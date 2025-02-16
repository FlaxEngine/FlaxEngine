// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_DIRECTX11 || GRAPHICS_API_DIRECTX12

#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Graphics//RenderTools.h"
#include "Engine/Graphics/Enums.h"
#include "IncludeDirectXHeaders.h"
#include "Engine/Core/Log.h"

/// <summary>
/// Set of utilities for rendering on DirectX platform.
/// </summary>
namespace RenderToolsDX
{
#if GRAPHICS_API_DIRECTX11

    /// <summary>
    /// Converts Flax GPUResourceUsage to DirectX 11 resource D3D11_USAGE enum type.
    /// </summary>
    /// <param name="usage">The Flax resource usage.</param>
    /// <returns>The DirectX 11 resource usage.</returns>
    inline D3D11_USAGE ToD3D11Usage(const GPUResourceUsage usage)
    {
        switch (usage)
        {
        case GPUResourceUsage::Dynamic:
            return D3D11_USAGE_DYNAMIC;
        case GPUResourceUsage::Staging:
        case GPUResourceUsage::StagingUpload:
        case GPUResourceUsage::StagingReadback:
            return D3D11_USAGE_STAGING;
        default:
            return D3D11_USAGE_DEFAULT;
        }
    }

    /// <summary>
    /// Gets the cpu access flags from the resource usage.
    /// </summary>
    /// <param name="usage">The Flax resource usage.</param>
    /// <returns>The DirectX 11 resource CPU usage.</returns>
    inline UINT GetDX11CpuAccessFlagsFromUsage(const GPUResourceUsage usage)
    {
        switch (usage)
        {
        case GPUResourceUsage::Dynamic:
            return D3D11_CPU_ACCESS_WRITE;
        case GPUResourceUsage::Staging:
            return D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
        case GPUResourceUsage::StagingReadback:
            return D3D11_CPU_ACCESS_READ;
        case GPUResourceUsage::StagingUpload:
            return D3D11_CPU_ACCESS_WRITE;
        default:
            return 0;
        }
    }

#endif

    /// <summary>
    /// Converts Flax Pixel Format to the DXGI Format.
    /// </summary>
    /// <param name="format">The Flax Pixel Format.</param>
    /// <returns>The DXGI Format</returns>
    extern DXGI_FORMAT ToDxgiFormat(PixelFormat format);

    // Aligns location to the next multiple of align value.
    template<typename T>
    T Align(T location, T align)
    {
        ASSERT(!((0 == align) || (align & (align - 1))));
        return ((location + (align - 1)) & ~(align - 1));
    }

    extern const Char* GetFeatureLevelString(const D3D_FEATURE_LEVEL featureLevel);

    // Calculate a subresource index for a texture
    FORCE_INLINE uint32 CalcSubresourceIndex(uint32 mipSlice, uint32 arraySlice, uint32 mipLevels)
    {
        return mipSlice + arraySlice * mipLevels;
    }

    inline uint32 CountAdapterOutputs(IDXGIAdapter* adapter)
    {
        uint32 count = 0;
        while (true)
        {
            IDXGIOutput* output;
            HRESULT hr = adapter->EnumOutputs(count, &output);
            if (FAILED(hr))
            {
                break;
            }
            count++;
        }
        return count;
    }

    extern String GetD3DErrorString(HRESULT errorCode);

    inline void ValidateD3DResult(HRESULT result, const char* file = "", uint32 line = 0)
    {
        ASSERT(FAILED(result));
        const String& errorString = GetD3DErrorString(result);
        LOG(Fatal, "DirectX error: {0} at {1}:{2}", errorString, String(file), line);
    }

    inline void LogD3DResult(HRESULT result, const char* file = "", uint32 line = 0)
    {
        ASSERT(FAILED(result));
        const String& errorString = GetD3DErrorString(result);
        LOG(Error, "DirectX error: {0} at {1}:{2}", errorString, String(file), line);
    }
};

#if GPU_ENABLE_ASSERTION

// DirectX results validation
#define VALIDATE_DIRECTX_CALL(x) { HRESULT result = x; if (FAILED(result)) RenderToolsDX::ValidateD3DResult(result, __FILE__, __LINE__); }
#define LOG_DIRECTX_RESULT(result) if (FAILED(result)) RenderToolsDX::LogD3DResult(result, __FILE__, __LINE__)
#define LOG_DIRECTX_RESULT_WITH_RETURN(result, returnValue) if (FAILED(result)) { RenderToolsDX::LogD3DResult(result, __FILE__, __LINE__); return returnValue; }

#else

#define VALIDATE_DIRECTX_CALL(x) x
#define LOG_DIRECTX_RESULT(result) if(FAILED(result)) RenderToolsDX::LogD3DResult(result)
#define LOG_DIRECTX_RESULT_WITH_RETURN(result, returnValue) if(FAILED(result)) { RenderToolsDX::LogD3DResult(result); return returnValue; }

#endif

#if GPU_ENABLE_DIAGNOSTICS || COMPILE_WITH_SHADER_COMPILER || GPU_ENABLE_RESOURCE_NAMING

#include "Engine/Utilities/StringConverter.h"

// Link DXGI lib
#pragma comment(lib, "dxguid.lib")

#endif

#if GPU_ENABLE_RESOURCE_NAMING

// SetDebugObjectName - ANSI

template<typename T>
inline void SetDebugObjectName(T* resource, const char* data, UINT size)
{
#if PLATFORM_XBOX_SCARLETT || PLATFORM_XBOX_ONE
    const StringAsUTF16<> nameUTF16(data, size);
    resource->SetName(nameUTF16.Get());
#else
    resource->SetPrivateData(WKPDID_D3DDebugObjectName, size, data);
#endif
}

template<uint32 NameLength>
inline void SetDebugObjectName(IDXGIObject* resource, const char (&name)[NameLength])
{
    SetDebugObjectName(resource, name, NameLength - 1);
}

#if GRAPHICS_API_DIRECTX11

template<uint32 NameLength>
inline void SetDebugObjectName(ID3D10DeviceChild* resource, const char(&name)[NameLength])
{
	SetDebugObjectName(resource, name, NameLength - 1);
}

template<uint32 NameLength>
inline void SetDebugObjectName(ID3D11DeviceChild* resource, const char(&name)[NameLength])
{
	SetDebugObjectName(resource, name, NameLength - 1);
}

#endif

#if GRAPHICS_API_DIRECTX12

template<uint32 NameLength>
inline void SetDebugObjectName(ID3D12DeviceChild* resource, const char (&name)[NameLength])
{
#if PLATFORM_XBOX_SCARLETT || PLATFORM_XBOX_ONE
    const StringAsUTF16<> nameUTF16(name);
    resource->SetName(nameUTF16.Get());
#else
    resource->SetPrivateData(WKPDID_D3DDebugObjectName, NameLength - 1, name);
#endif
}

#endif

#if GRAPHICS_API_DIRECTX11

template<uint32 NameLength>
inline void SetDebugObjectName(ID3D10Resource* resource, const char(&name)[NameLength])
{
	resource->SetPrivateData(WKPDID_D3DDebugObjectName, NameLength - 1, name);
}

template<uint32 NameLength>
inline void SetDebugObjectName(ID3D11Resource* resource, const char(&name)[NameLength])
{
	resource->SetPrivateData(WKPDID_D3DDebugObjectName, NameLength - 1, name);
}

#endif

#if GRAPHICS_API_DIRECTX12

template<uint32 NameLength>
inline void SetDebugObjectName(ID3D12Resource* resource, const char (&name)[NameLength])
{
#if PLATFORM_XBOX_SCARLETT || PLATFORM_XBOX_ONE
    const StringAsUTF16<> nameUTF16(name);
    resource->SetName(nameUTF16.Get());
#else
    resource->SetPrivateData(WKPDID_D3DDebugObjectName, NameLength - 1, name);
#endif
}

#endif

template<typename T, uint32 NameLength>
inline void SetDebugObjectName(ComPtr<T> resource, const char (&name)[NameLength])
{
    SetDebugObjectName(resource.Get(), name);
}

// SetDebugObjectName - UNICODE

template<typename T>
inline void SetDebugObjectName(T* resource, const Char* data, UINT size)
{
#if PLATFORM_XBOX_SCARLETT || PLATFORM_XBOX_ONE
    if (data && size > 0)
        resource->SetName(data);
#else
    char* ansi = (char*)Allocator::Allocate(size + 1);
    StringUtils::ConvertUTF162ANSI(data, ansi, size);
    ansi[size] ='\0';
    SetDebugObjectName(resource, ansi, size);
    Allocator::Free(ansi);
#endif
}

template<typename T>
inline void SetDebugObjectName(T* resource, const String& name)
{
    SetDebugObjectName(resource, *name, name.Length());
}

template<typename T>
inline void SetDebugObjectName(ComPtr<T> resource, const Char* data, UINT size)
{
    SetDebugObjectName(resource.Get(), data, size);
}

template<typename T, uint32 NameLength>
inline void SetDebugObjectName(T* resource, const Char (&name)[NameLength])
{
    SetDebugObjectName(resource, name, 2 * (NameLength - 1));
}

template<typename T, uint32 NameLength>
inline void SetDebugObjectName(ComPtr<T> resource, const Char (&name)[NameLength])
{
    SetDebugObjectName(resource.Get(), name);
}

template<typename T>
inline void SetDebugObjectName(ComPtr<T> resource, const String& name)
{
    SetDebugObjectName(resource.Get(), *name, name.Length());
}

// Macros

#define DX_SET_DEBUG_NAME(resource, name) SetDebugObjectName(resource, name)
#define DX_SET_DEBUG_NAME_EX(resource, parent, type, id) \
{\
	String dx_s = String::Format(TEXT("{0}:{1}{2}"), parent, type, id);\
	SetDebugObjectName(resource, *dx_s, dx_s.Length() * 2);\
}

#else

#define DX_SET_DEBUG_NAME(resource, name)
#define DX_SET_DEBUG_NAME_EX(resource, parent, type, id)

#endif

#endif
