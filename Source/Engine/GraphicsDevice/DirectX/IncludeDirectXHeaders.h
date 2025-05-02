// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

// Common
#if GRAPHICS_API_DIRECTX11 || GRAPHICS_API_DIRECTX12 || COMPILE_WITH_DX_SHADER_COMPILER

#include "Engine/Platform/Win32/IncludeWindowsHeaders.h"
#include "Engine/Platform/Windows/ComPtr.h"

// Helper define to dispose the COM object with remaining references counter checking
#define DX_SAFE_RELEASE_CHECK(x, refs) if(x) { auto res = (x)->Release(); (x) = nullptr; CHECK(res == refs); }

#endif

// D3D11 and D3D12
#if GRAPHICS_API_DIRECTX11 || GRAPHICS_API_DIRECTX12

#if PLATFORM_XBOX_SCARLETT || PLATFORM_XBOX_ONE

#if  PLATFORM_XBOX_SCARLETT
#include <Scarlett/d3d12_xs.h>
#else
#include <XboxOne/d3d12_x.h>
#endif
typedef IGraphicsUnknown IDXGIFactory4;
typedef IGraphicsUnknown IDXGISwapChain3;
#define	D3D12_TEXTURE_DATA_PITCH_ALIGNMENT 256
#define IID_PPV_ARGS(ppType) IID_GRAPHICS_PPV_ARGS(ppType)

#else

#if GRAPHICS_API_DIRECTX12
#include <D3D12.h>
#endif
#include <d3dcommon.h>
#if GRAPHICS_API_DIRECTX11
#define D3D11_NO_HELPERS
#include <D3D11.h>
#include <d3d11_1.h>
#include <dxgi1_3.h>
#include <dxgi1_5.h>
#include <dxgi1_6.h>
#endif
#if GRAPHICS_API_DIRECTX12
#include <dxgi1_5.h>
#include <dxgi1_6.h>
#endif

#pragma comment(lib, "DXGI.lib")
typedef IUnknown IGraphicsUnknown;

#endif

#endif
