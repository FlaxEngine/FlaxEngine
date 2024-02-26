// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_DIRECTX11 || GRAPHICS_API_DIRECTX12

#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Graphics//RenderTools.h"
#include "Engine/Graphics/Enums.h"
#include "IncludeDirectXHeaders.h"
#include "Engine/Core/Types/StringBuilder.h"
#include "Engine/Core/Log.h"
#include "Engine/Utilities/StringConverter.h"
#include <winerror.h>

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
    /// summary DXGI Pixel Format to the Flax Pixel Format.
    /// </summary>
    /// <param name="format">The DXGI format.</param>
    /// <returns>The Flax Pixel Format</returns>
    FORCE_INLINE PixelFormat ToPixelFormat(const DXGI_FORMAT format)
    {
        return static_cast<PixelFormat>(format);
    }

    /// <summary>
    /// Converts Flax Pixel Format to the DXGI Format.
    /// </summary>
    /// <param name="format">The Flax Pixel Format.</param>
    /// <returns>The DXGI Format</returns>
    FORCE_INLINE DXGI_FORMAT ToDxgiFormat(const PixelFormat format)
    {
        return static_cast<DXGI_FORMAT>(format);
    }

    // Aligns location to the next multiple of align value.
    template<typename T>
    T Align(T location, T align)
    {
        ASSERT(!((0 == align) || (align & (align - 1))));
        return ((location + (align - 1)) & ~(align - 1));
    }

    static const Char* GetFeatureLevelString(const D3D_FEATURE_LEVEL featureLevel)
    {
        switch (featureLevel)
        {
        case D3D_FEATURE_LEVEL_9_1:
            return TEXT("9.1");
        case D3D_FEATURE_LEVEL_9_2:
            return TEXT("9.2");
        case D3D_FEATURE_LEVEL_9_3:
            return TEXT("9.3");
        case D3D_FEATURE_LEVEL_10_0:
            return TEXT("10");
        case D3D_FEATURE_LEVEL_10_1:
            return TEXT("10.1");
        case D3D_FEATURE_LEVEL_11_0:
            return TEXT("11");
        case D3D_FEATURE_LEVEL_11_1:
            return TEXT("11.1");
#if GRAPHICS_API_DIRECTX12
        case D3D_FEATURE_LEVEL_12_0:
            return TEXT("12");
        case D3D_FEATURE_LEVEL_12_1:
            return TEXT("12.1");
#endif
        default:
            return TEXT("?");
        }
    }

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

    static String GetD3DErrorString(HRESULT errorCode)
    {
        StringBuilder sb(256);

        // Switch error code
#define D3DERR(x) case x: sb.Append(TEXT(#x)); break
        switch (errorCode)
        {
            // Windows
        D3DERR(S_OK);
        D3DERR(E_FAIL);
        D3DERR(E_INVALIDARG);
        D3DERR(E_OUTOFMEMORY);
        D3DERR(E_NOINTERFACE);
        D3DERR(E_NOTIMPL);

            // DirectX
#if WITH_D3DX_LIBS
		D3DERR(D3DERR_INVALIDCALL);
		D3DERR(D3DERR_WASSTILLDRAWING);
#endif

            // DirectX 11
        D3DERR(D3D11_ERROR_FILE_NOT_FOUND);
        D3DERR(D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS);

            // DirectX 12
            //D3DERR(D3D12_ERROR_FILE_NOT_FOUND);
            //D3DERR(D3D12_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS);
            //D3DERR(D3D12_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS);

            // DXGI
        D3DERR(DXGI_ERROR_INVALID_CALL);
        D3DERR(DXGI_ERROR_NOT_FOUND);
        D3DERR(DXGI_ERROR_MORE_DATA);
        D3DERR(DXGI_ERROR_UNSUPPORTED);
        D3DERR(DXGI_ERROR_DEVICE_REMOVED);
        D3DERR(DXGI_ERROR_DEVICE_HUNG);
        D3DERR(DXGI_ERROR_DEVICE_RESET);
        D3DERR(DXGI_ERROR_WAS_STILL_DRAWING);
        D3DERR(DXGI_ERROR_FRAME_STATISTICS_DISJOINT);
        D3DERR(DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE);
        D3DERR(DXGI_ERROR_DRIVER_INTERNAL_ERROR);
        D3DERR(DXGI_ERROR_NONEXCLUSIVE);
        D3DERR(DXGI_ERROR_NOT_CURRENTLY_AVAILABLE);
        D3DERR(DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED);
        D3DERR(DXGI_ERROR_REMOTE_OUTOFMEMORY);
        D3DERR(DXGI_ERROR_ACCESS_LOST);
        D3DERR(DXGI_ERROR_WAIT_TIMEOUT);
        D3DERR(DXGI_ERROR_SESSION_DISCONNECTED);
        D3DERR(DXGI_ERROR_RESTRICT_TO_OUTPUT_STALE);
        D3DERR(DXGI_ERROR_CANNOT_PROTECT_CONTENT);
        D3DERR(DXGI_ERROR_ACCESS_DENIED);
        D3DERR(DXGI_ERROR_NAME_ALREADY_EXISTS);
        D3DERR(DXGI_ERROR_SDK_COMPONENT_MISSING);
#if GRAPHICS_API_DIRECTX12
        D3DERR(DXGI_ERROR_NOT_CURRENT);
        D3DERR(DXGI_ERROR_HW_PROTECTION_OUTOFMEMORY);
        D3DERR(D3D12_ERROR_DRIVER_VERSION_MISMATCH);
#endif

        default:
        {
            sb.AppendFormat(TEXT("0x{0:x}"), static_cast<unsigned int>(errorCode));
        }
        break;
        }
#undef D3DERR

        if (errorCode == DXGI_ERROR_DEVICE_REMOVED || errorCode == DXGI_ERROR_DEVICE_RESET || errorCode == DXGI_ERROR_DRIVER_INTERNAL_ERROR)
        {
            HRESULT reason = S_OK;
            const RendererType rendererType = GPUDevice::Instance ? GPUDevice::Instance->GetRendererType() : RendererType::Unknown;
            void* nativePtr = GPUDevice::Instance ? GPUDevice::Instance->GetNativePtr() : nullptr;
#if GRAPHICS_API_DIRECTX12
            if (rendererType == RendererType::DirectX12 && nativePtr)
            {
                reason = ((ID3D12Device*)nativePtr)->GetDeviceRemovedReason();
            }
#endif
#if GRAPHICS_API_DIRECTX11
            if ((rendererType == RendererType::DirectX11 ||
                rendererType == RendererType::DirectX10_1 ||
                rendererType == RendererType::DirectX10) && nativePtr)
            {
                reason = ((ID3D11Device*)nativePtr)->GetDeviceRemovedReason();
            }
#endif
            const Char* reasonStr = nullptr;
            switch (reason)
            {
            case DXGI_ERROR_DEVICE_HUNG:
                reasonStr = TEXT("HUNG");
                break;
            case DXGI_ERROR_DEVICE_REMOVED:
                reasonStr = TEXT("REMOVED");
                break;
            case DXGI_ERROR_DEVICE_RESET:
                reasonStr = TEXT("RESET");
                break;
            case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
                reasonStr = TEXT("INTERNAL_ERROR");
                break;
            case DXGI_ERROR_INVALID_CALL:
                reasonStr = TEXT("INVALID_CALL");
                break;
            }
            if (reasonStr != nullptr)
                sb.AppendFormat(TEXT(", Device Removed Reason: {0}"), reasonStr);
        }

        return sb.ToString();
    }

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

#if GPU_ENABLE_DIAGNOSTICS || COMPILE_WITH_SHADER_COMPILER

// Link DXGI lib
#pragma comment(lib, "dxguid.lib")

#endif

#if GPU_ENABLE_DIAGNOSTICS && GPU_ENABLE_RESOURCE_NAMING

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
