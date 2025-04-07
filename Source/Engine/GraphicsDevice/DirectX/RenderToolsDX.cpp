// Copyright (c) Wojciech Figat. All rights reserved.

#if GRAPHICS_API_DIRECTX11 || GRAPHICS_API_DIRECTX12

#include "RenderToolsDX.h"
#include "GPUAdapterDX.h"
#include "GPUDeviceDX.h"
#include "Engine/Core/Types/DateTime.h"
#include "Engine/Core/Types/StringBuilder.h"
#include "Engine/Graphics/GPUDevice.h"
#include "IncludeDirectXHeaders.h"
#include <winerror.h>

#define GPU_DRIVER_DETECTION_WIN32_REGISTRY (PLATFORM_WINDOWS)
#define GPU_DRIVER_DETECTION_WIN32_SETUPAPI (PLATFORM_WINDOWS)

#if GPU_DRIVER_DETECTION_WIN32_SETUPAPI
#define _SETUPAPI_VER WINVER
typedef void* LPCDLGTEMPLATE;
#include <SetupAPI.h>
#pragma comment(lib, "SetupAPI.lib")
#endif

namespace Windows
{
    typedef struct _devicemodeW
    {
        WCHAR dmDeviceName[32];
        WORD dmSpecVersion;
        WORD dmDriverVersion;
        WORD dmSize;
        WORD dmDriverExtra;
        DWORD dmFields;

        union
        {
            struct
            {
                short dmOrientation;
                short dmPaperSize;
                short dmPaperLength;
                short dmPaperWidth;
                short dmScale;
                short dmCopies;
                short dmDefaultSource;
                short dmPrintQuality;
            } DUMMYSTRUCTNAME;

            POINTL dmPosition;

            struct
            {
                POINTL dmPosition;
                DWORD dmDisplayOrientation;
                DWORD dmDisplayFixedOutput;
            } DUMMYSTRUCTNAME2;
        } DUMMYUNIONNAME;

        short dmColor;
        short dmDuplex;
        short dmYResolution;
        short dmTTOption;
        short dmCollate;
        WCHAR dmFormName[32];
        WORD dmLogPixels;
        DWORD dmBitsPerPel;
        DWORD dmPelsWidth;
        DWORD dmPelsHeight;

        union
        {
            DWORD dmDisplayFlags;
            DWORD dmNup;
        } DUMMYUNIONNAME2;

        DWORD dmDisplayFrequency;
        DWORD dmICMMethod;
        DWORD dmICMIntent;
        DWORD dmMediaType;
        DWORD dmDitherType;
        DWORD dmReserved1;
        DWORD dmReserved2;
        DWORD dmPanningWidth;
        DWORD dmPanningHeight;
    } DEVMODEW, *PDEVMODEW, *NPDEVMODEW, *LPDEVMODEW;

    WIN_API BOOL WIN_API_CALLCONV EnumDisplaySettingsW(LPCWSTR lpszDeviceName, DWORD iModeNum, DEVMODEW* lpDevMode);
}

// @formatter:off

DXGI_FORMAT PixelFormatToDXGIFormat[110] =
{
    DXGI_FORMAT_UNKNOWN,
    DXGI_FORMAT_R32G32B32A32_TYPELESS,
    DXGI_FORMAT_R32G32B32A32_FLOAT,
    DXGI_FORMAT_R32G32B32A32_UINT,
    DXGI_FORMAT_R32G32B32A32_SINT,
    DXGI_FORMAT_R32G32B32_TYPELESS,
    DXGI_FORMAT_R32G32B32_FLOAT,
    DXGI_FORMAT_R32G32B32_UINT,
    DXGI_FORMAT_R32G32B32_SINT,
    DXGI_FORMAT_R16G16B16A16_TYPELESS,
    DXGI_FORMAT_R16G16B16A16_FLOAT,
    DXGI_FORMAT_R16G16B16A16_UNORM,
    DXGI_FORMAT_R16G16B16A16_UINT,
    DXGI_FORMAT_R16G16B16A16_SNORM,
    DXGI_FORMAT_R16G16B16A16_SINT,
    DXGI_FORMAT_R32G32_TYPELESS,
    DXGI_FORMAT_R32G32_FLOAT,
    DXGI_FORMAT_R32G32_UINT,
    DXGI_FORMAT_R32G32_SINT,
    DXGI_FORMAT_R32G8X24_TYPELESS,
    DXGI_FORMAT_D32_FLOAT_S8X24_UINT,
    DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS,
    DXGI_FORMAT_X32_TYPELESS_G8X24_UINT,
    DXGI_FORMAT_R10G10B10A2_TYPELESS,
    DXGI_FORMAT_R10G10B10A2_UNORM,
    DXGI_FORMAT_R10G10B10A2_UINT,
    DXGI_FORMAT_R11G11B10_FLOAT,
    DXGI_FORMAT_R8G8B8A8_TYPELESS,
    DXGI_FORMAT_R8G8B8A8_UNORM,
    DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
    DXGI_FORMAT_R8G8B8A8_UINT,
    DXGI_FORMAT_R8G8B8A8_SNORM,
    DXGI_FORMAT_R8G8B8A8_SINT,
    DXGI_FORMAT_R16G16_TYPELESS,
    DXGI_FORMAT_R16G16_FLOAT,
    DXGI_FORMAT_R16G16_UNORM,
    DXGI_FORMAT_R16G16_UINT,
    DXGI_FORMAT_R16G16_SNORM,
    DXGI_FORMAT_R16G16_SINT,
    DXGI_FORMAT_R32_TYPELESS,
    DXGI_FORMAT_D32_FLOAT,
    DXGI_FORMAT_R32_FLOAT,
    DXGI_FORMAT_R32_UINT,
    DXGI_FORMAT_R32_SINT,
    DXGI_FORMAT_R24G8_TYPELESS,
    DXGI_FORMAT_D24_UNORM_S8_UINT,
    DXGI_FORMAT_R24_UNORM_X8_TYPELESS,
    DXGI_FORMAT_X24_TYPELESS_G8_UINT,
    DXGI_FORMAT_R8G8_TYPELESS,
    DXGI_FORMAT_R8G8_UNORM,
    DXGI_FORMAT_R8G8_UINT,
    DXGI_FORMAT_R8G8_SNORM,
    DXGI_FORMAT_R8G8_SINT,
    DXGI_FORMAT_R16_TYPELESS,
    DXGI_FORMAT_R16_FLOAT,
    DXGI_FORMAT_D16_UNORM,
    DXGI_FORMAT_R16_UNORM,
    DXGI_FORMAT_R16_UINT,
    DXGI_FORMAT_R16_SNORM,
    DXGI_FORMAT_R16_SINT,
    DXGI_FORMAT_R8_TYPELESS,
    DXGI_FORMAT_R8_UNORM,
    DXGI_FORMAT_R8_UINT,
    DXGI_FORMAT_R8_SNORM,
    DXGI_FORMAT_R8_SINT,
    DXGI_FORMAT_A8_UNORM,
    DXGI_FORMAT_R1_UNORM,
    DXGI_FORMAT_R9G9B9E5_SHAREDEXP,
    DXGI_FORMAT_R8G8_B8G8_UNORM,
    DXGI_FORMAT_G8R8_G8B8_UNORM,
    DXGI_FORMAT_BC1_TYPELESS,
    DXGI_FORMAT_BC1_UNORM,
    DXGI_FORMAT_BC1_UNORM_SRGB,
    DXGI_FORMAT_BC2_TYPELESS,
    DXGI_FORMAT_BC2_UNORM,
    DXGI_FORMAT_BC2_UNORM_SRGB,
    DXGI_FORMAT_BC3_TYPELESS,
    DXGI_FORMAT_BC3_UNORM,
    DXGI_FORMAT_BC3_UNORM_SRGB,
    DXGI_FORMAT_BC4_TYPELESS,
    DXGI_FORMAT_BC4_UNORM,
    DXGI_FORMAT_BC4_SNORM,
    DXGI_FORMAT_BC5_TYPELESS,
    DXGI_FORMAT_BC5_UNORM,
    DXGI_FORMAT_BC5_SNORM,
    DXGI_FORMAT_B5G6R5_UNORM,
    DXGI_FORMAT_B5G5R5A1_UNORM,
    DXGI_FORMAT_B8G8R8A8_UNORM,
    DXGI_FORMAT_B8G8R8X8_UNORM,
    DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM,
    DXGI_FORMAT_B8G8R8A8_TYPELESS,
    DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
    DXGI_FORMAT_B8G8R8X8_TYPELESS,
    DXGI_FORMAT_B8G8R8X8_UNORM_SRGB,
    DXGI_FORMAT_BC6H_TYPELESS,
    DXGI_FORMAT_BC6H_UF16,
    DXGI_FORMAT_BC6H_SF16,
    DXGI_FORMAT_BC7_TYPELESS,
    DXGI_FORMAT_BC7_UNORM,
    DXGI_FORMAT_BC7_UNORM_SRGB,
    DXGI_FORMAT_UNKNOWN, // ASTC_4x4_UNorm
    DXGI_FORMAT_UNKNOWN, // ASTC_4x4_UNorm_sRGB
    DXGI_FORMAT_UNKNOWN, // ASTC_6x6_UNorm
    DXGI_FORMAT_UNKNOWN, // ASTC_6x6_UNorm_sRGB
    DXGI_FORMAT_UNKNOWN, // ASTC_8x8_UNorm
    DXGI_FORMAT_UNKNOWN, // ASTC_8x8_UNorm_sRGB
    DXGI_FORMAT_UNKNOWN, // ASTC_10x10_UNorm
    DXGI_FORMAT_UNKNOWN, // ASTC_10x10_UNorm_sRGB
    DXGI_FORMAT_YUY2,
    DXGI_FORMAT_NV12,
};

// @formatter:on

DXGI_FORMAT RenderToolsDX::ToDxgiFormat(PixelFormat format)
{
    return PixelFormatToDXGIFormat[(int32)format];
}

const Char* RenderToolsDX::GetFeatureLevelString(D3D_FEATURE_LEVEL featureLevel)
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

uint32 RenderToolsDX::CountAdapterOutputs(IDXGIAdapter* adapter)
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

void FormatD3DErrorString(HRESULT errorCode, StringBuilder& sb, HRESULT& removedReason)
{
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
    D3DERR(D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS);
    D3DERR(D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD);

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
        sb.AppendFormat(TEXT("0x{0:x}"), static_cast<unsigned int>(errorCode));
        break;
    }
#undef D3DERR

    if (errorCode == DXGI_ERROR_DEVICE_REMOVED || errorCode == DXGI_ERROR_DEVICE_RESET || errorCode == DXGI_ERROR_DRIVER_INTERNAL_ERROR)
    {
        GPUDevice* device = GPUDevice::Instance;
        const RendererType rendererType = device ? device->GetRendererType() : RendererType::Unknown;
        void* nativePtr = device ? device->GetNativePtr() : nullptr;
#if GRAPHICS_API_DIRECTX12
        if (rendererType == RendererType::DirectX12 && nativePtr)
        {
            removedReason = ((ID3D12Device*)nativePtr)->GetDeviceRemovedReason();
        }
#endif
#if GRAPHICS_API_DIRECTX11
        if ((rendererType == RendererType::DirectX11 ||
            rendererType == RendererType::DirectX10_1 ||
            rendererType == RendererType::DirectX10) && nativePtr)
        {
            removedReason = ((ID3D11Device*)nativePtr)->GetDeviceRemovedReason();
        }
#endif
        const Char* reasonStr = nullptr;
        switch (removedReason)
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
}

void RenderToolsDX::LogD3DResult(HRESULT result, const char* file, uint32 line, bool fatal)
{
    ASSERT_LOW_LAYER(FAILED(result));

    // Process error and format message
    StringBuilder sb;
    HRESULT removedReason = S_OK;
    sb.Append(TEXT("DirectX error: "));
    FormatD3DErrorString(result, sb, removedReason);
    if (file)
        sb.Append(TEXT(" at ")).Append(file).Append(':').Append(line);
    const StringView msg(sb.ToStringView());

    // Handle error
    FatalErrorType errorType = FatalErrorType::None;
    if (result == E_OUTOFMEMORY)
        errorType = FatalErrorType::GPUOutOfMemory;
    else if (removedReason != S_OK)
    {
        errorType = FatalErrorType::GPUCrash;
        if (removedReason == DXGI_ERROR_DEVICE_HUNG)
            errorType = FatalErrorType::GPUHang;
    }
    if (errorType != FatalErrorType::None)
        Platform::Fatal(msg, nullptr, errorType);
    else
        Log::Logger::Write(fatal ? LogType::Fatal : LogType::Error, msg);
}

LPCSTR RenderToolsDX::GetVertexInputSemantic(VertexElement::Types type, UINT& semanticIndex)
{
    static_assert((int32)VertexElement::Types::MAX == 16, "Update code below.");
    semanticIndex = 0;
    switch (type)
    {
    case VertexElement::Types::Position:
        return "POSITION";
    case VertexElement::Types::Color:
        return "COLOR";
    case VertexElement::Types::Normal:
        return "NORMAL";
    case VertexElement::Types::Tangent:
        return "TANGENT";
    case VertexElement::Types::BlendIndices:
        return "BLENDINDICES";
    case VertexElement::Types::BlendWeights:
        return "BLENDWEIGHTS";
    case VertexElement::Types::TexCoord0:
        return "TEXCOORD";
    case VertexElement::Types::TexCoord1:
        semanticIndex = 1;
        return "TEXCOORD";
    case VertexElement::Types::TexCoord2:
        semanticIndex = 2;
        return "TEXCOORD";
    case VertexElement::Types::TexCoord3:
        semanticIndex = 3;
        return "TEXCOORD";
    case VertexElement::Types::TexCoord4:
        semanticIndex = 4;
        return "TEXCOORD";
    case VertexElement::Types::TexCoord5:
        semanticIndex = 5;
        return "TEXCOORD";
    case VertexElement::Types::TexCoord6:
        semanticIndex = 6;
        return "TEXCOORD";
    case VertexElement::Types::TexCoord7:
        semanticIndex = 7;
        return "TEXCOORD";
    case VertexElement::Types::Attribute0:
        return "ATTRIBUTE";
    case VertexElement::Types::Attribute1:
        semanticIndex = 1;
        return "ATTRIBUTE";
    case VertexElement::Types::Attribute2:
        semanticIndex = 2;
        return "ATTRIBUTE";
    case VertexElement::Types::Attribute3:
        semanticIndex = 3;
        return "ATTRIBUTE";
    case VertexElement::Types::Lightmap:
        return "LIGHTMAP";
    default:
        LOG(Fatal, "Invalid vertex shader element semantic type");
        return "";
    }
}

void GPUAdapterDX::GetDriverVersion()
{
#if GPU_DRIVER_DETECTION_WIN32_REGISTRY
    {
        // Reference: https://github.com/GameTechDev/gpudetect/blob/master/GPUDetect.cpp

        // Fetch registry data
        HKEY dxKeyHandle = nullptr;
        DWORD numOfAdapters = 0;
        LSTATUS returnCode = RegOpenKeyExW(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\DirectX"), 0, KEY_READ, &dxKeyHandle);
        if (returnCode == S_OK)
        {
            // Find all sub keys
            DWORD subKeyMaxLength = 0;
            returnCode = RegQueryInfoKeyW(dxKeyHandle, 0, 0, 0, &numOfAdapters, &subKeyMaxLength, 0, 0, 0, 0, 0, 0);
            if (returnCode == S_OK && subKeyMaxLength < 100)
            {
                subKeyMaxLength += 1;
                uint64_t driverVersionRaw = 0;
                TCHAR subKeyName[100];
                for (DWORD i = 0; i < numOfAdapters; i++)
                {
                    DWORD subKeyLength = subKeyMaxLength;
                    returnCode = RegEnumKeyExW(dxKeyHandle, i, subKeyName, &subKeyLength, 0, 0, 0, 0);
                    if (returnCode == S_OK)
                    {
                        LUID adapterLUID = {};
                        DWORD qwordSize = sizeof(uint64_t);
                        returnCode = RegGetValueW(dxKeyHandle, subKeyName, TEXT("AdapterLuid"), RRF_RT_QWORD, 0, &adapterLUID, &qwordSize);
                        if (returnCode == S_OK && adapterLUID.HighPart == Description.AdapterLuid.HighPart && adapterLUID.LowPart == Description.AdapterLuid.LowPart)
                        {
                            // Get driver version
                            returnCode = RegGetValueW(dxKeyHandle, subKeyName, TEXT("DriverVersion"), RRF_RT_QWORD, 0, &driverVersionRaw, &qwordSize);
                            if (returnCode == S_OK)
                            {
                                Version driverVersion(
                                    (int32)((driverVersionRaw & 0xFFFF000000000000) >> 16 * 3),
                                    (int32)((driverVersionRaw & 0x0000FFFF00000000) >> 16 * 2),
                                    (int32)((driverVersionRaw & 0x00000000FFFF0000) >> 16 * 1),
                                    (int32)((driverVersionRaw & 0x000000000000FFFF)));
                                SetDriverVersion(driverVersion);
                            }
                            break;
                        }
                    }
                }
            }
            RegCloseKey(dxKeyHandle);
        }

        if (DriverVersion != Version(0, 0))
            return;
    }
#endif

#if GPU_DRIVER_DETECTION_WIN32_SETUPAPI
    {
        // Reference: https://gist.github.com/LxLasso/eccee4d71c2e49492f2cbf01a966fa73

        // Copied from devguid.h and devpkey.h
#define MAKE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) const GUID name = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
#define MAKE_DEVPROPKEY(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8, pid) const DEVPROPKEY name = { { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }, pid }
        MAKE_GUID(GUID_DEVCLASS_DISPLAY, 0x4d36e968L, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18);
        MAKE_DEVPROPKEY(DEVPKEY_Device_DriverDate, 0xa8b865dd, 0x2e3d, 0x4094, 0xad, 0x97, 0xe5, 0x93, 0xa7, 0xc, 0x75, 0xd6, 2);
        MAKE_DEVPROPKEY(DEVPKEY_Device_DriverVersion, 0xa8b865dd, 0x2e3d, 0x4094, 0xad, 0x97, 0xe5, 0x93, 0xa7, 0xc, 0x75, 0xd6, 3);
#undef MAKE_DEVPROPKEY
#undef MAKE_GUID

        HDEVINFO deviceInfoList = SetupDiGetClassDevs(&GUID_DEVCLASS_DISPLAY, NULL, NULL, DIGCF_PRESENT);
        if (deviceInfoList != INVALID_HANDLE_VALUE)
        {
            SP_DEVINFO_DATA deviceInfo;
            ZeroMemory(&deviceInfo, sizeof(deviceInfo));
            deviceInfo.cbSize = sizeof(SP_DEVINFO_DATA);

            wchar_t searchBuffer[128];
            swprintf(searchBuffer, sizeof(searchBuffer) / 2, L"PCI\\VEN_%04X&DEV_%04X&SUBSYS_%04X", Description.VendorId, Description.DeviceId, Description.SubSysId);
            size_t searchBufferLen = wcslen(searchBuffer);

            DWORD deviceIndex = 0;
            DEVPROPTYPE propertyType;
            wchar_t buffer[300];
            while (SetupDiEnumDeviceInfo(deviceInfoList, deviceIndex, &deviceInfo))
            {
                DWORD deviceIdSize;
                if (SetupDiGetDeviceInstanceId(deviceInfoList, &deviceInfo, buffer, sizeof(buffer), &deviceIdSize) &&
                    wcsncmp(buffer, searchBuffer, searchBufferLen) == 0)
                {
                    // Get driver version
                    if (SetupDiGetDeviceProperty(deviceInfoList, &deviceInfo, &DEVPKEY_Device_DriverVersion, &propertyType, (PBYTE)buffer, sizeof(buffer), NULL, 0) &&
                        propertyType == DEVPROP_TYPE_STRING)
                    {
                        //ParseDriverVersionBuffer(buffer);
                        Version driverVersion;
                        String bufferStr(buffer);
                        if (!Version::Parse(bufferStr, &driverVersion))
                            SetDriverVersion(driverVersion);
                    }

#if 0
                    // Get driver date
					DEVPROPTYPE propertyType;
					if (SetupDiGetDeviceProperty(deviceInfoList, &deviceInfo, &DEVPKEY_Device_DriverDate, &propertyType, (PBYTE)buffer, sizeof(FILETIME), NULL, 0) &&
                        propertyType == DEVPROP_TYPE_FILETIME)
					{
						SYSTEMTIME deviceDriverSystemTime;
						FileTimeToSystemTime((FILETIME*)buffer, &deviceDriverSystemTime);
						DriverDate = DateTime(deviceDriverSystemTime.wYear, deviceDriverSystemTime.wMonth, deviceDriverSystemTime.wDay, deviceDriverSystemTime.wHour, deviceDriverSystemTime.wMinute, deviceDriverSystemTime.wSecond, deviceDriverSystemTime.wMilliseconds);
					}
#endif
                }
                deviceIndex++;
            }

            SetupDiDestroyDeviceInfoList(deviceInfoList);
        }

        if (DriverVersion != Version(0, 0))
            return;
    }
#endif
}

void GPUAdapterDX::SetDriverVersion(Version& ver)
{
    if (IsNVIDIA() && ver.Build() > 0 && ver.Revision() > 99)
    {
        // Convert NVIDIA version from 32.0.15.7247 into 572.47
        ver = Version((ver.Build() % 10) * 100 + ver.Revision() / 100, ver.Revision() % 100);
    }
    DriverVersion = ver;
}

void GPUDeviceDX::UpdateOutputs(IDXGIAdapter* adapter)
{
#if PLATFORM_WINDOWS
    // Collect output devices
    uint32 outputIdx = 0;
    ComPtr<IDXGIOutput> output;
    DXGI_FORMAT defaultBackbufferFormat = RenderToolsDX::ToDxgiFormat(GPU_BACK_BUFFER_PIXEL_FORMAT);
    Array<DXGI_MODE_DESC> modeDesc;
    while (adapter->EnumOutputs(outputIdx, &output) != DXGI_ERROR_NOT_FOUND)
    {
        auto& outputDX11 = Outputs.AddOne();

        outputDX11.Output = output;
        output->GetDesc(&outputDX11.Desc);

        uint32 numModes = 0;
        HRESULT hr = output->GetDisplayModeList(defaultBackbufferFormat, 0, &numModes, nullptr);
        if (FAILED(hr))
        {
            LOG(Warning, "Error while enumerating adapter output video modes.");
            continue;
        }

        modeDesc.Resize(numModes, false);
        hr = output->GetDisplayModeList(defaultBackbufferFormat, 0, &numModes, modeDesc.Get());
        if (FAILED(hr))
        {
            LOG(Warning, "Error while enumerating adapter output video modes.");
            continue;
        }

        for (auto& mode : modeDesc)
        {
            bool foundVideoMode = false;
            for (auto& videoMode : outputDX11.VideoModes)
            {
                if (videoMode.Width == mode.Width &&
                    videoMode.Height == mode.Height &&
                    videoMode.RefreshRate.Numerator == mode.RefreshRate.Numerator &&
                    videoMode.RefreshRate.Denominator == mode.RefreshRate.Denominator)
                {
                    foundVideoMode = true;
                    break;
                }
            }

            if (!foundVideoMode)
            {
                outputDX11.VideoModes.Add(mode);

                // Collect only from the main monitor
                if (Outputs.Count() == 1)
                {
                    VideoOutputModes.Add({ mode.Width, mode.Height, (uint32)(mode.RefreshRate.Numerator / (float)mode.RefreshRate.Denominator) });
                }
            }
        }

        // Get desktop display mode
        HMONITOR hMonitor = outputDX11.Desc.Monitor;
        MONITORINFOEX monitorInfo;
        monitorInfo.cbSize = sizeof(MONITORINFOEX);
        GetMonitorInfo(hMonitor, &monitorInfo);

        Windows::DEVMODEW devMode;
        devMode.dmSize = sizeof(Windows::DEVMODEW);
        devMode.dmDriverExtra = 0;
        Windows::EnumDisplaySettingsW(monitorInfo.szDevice, ((DWORD)-1), &devMode);

        DXGI_MODE_DESC currentMode;
        currentMode.Width = devMode.dmPelsWidth;
        currentMode.Height = devMode.dmPelsHeight;
        bool useDefaultRefreshRate = 1 == devMode.dmDisplayFrequency || 0 == devMode.dmDisplayFrequency;
        currentMode.RefreshRate.Numerator = useDefaultRefreshRate ? 0 : devMode.dmDisplayFrequency;
        currentMode.RefreshRate.Denominator = useDefaultRefreshRate ? 0 : 1;
        currentMode.Format = defaultBackbufferFormat;
        currentMode.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        currentMode.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

        if (output->FindClosestMatchingMode(&currentMode, &outputDX11.DesktopViewMode, nullptr) != S_OK)
            outputDX11.DesktopViewMode = currentMode;

        float refreshRate = outputDX11.DesktopViewMode.RefreshRate.Numerator / (float)outputDX11.DesktopViewMode.RefreshRate.Denominator;
        LOG(Info, "Video output '{0}' {1}x{2} {3} Hz", outputDX11.Desc.DeviceName, devMode.dmPelsWidth, devMode.dmPelsHeight, refreshRate);
        outputIdx++;
    }
#endif
}

#endif
