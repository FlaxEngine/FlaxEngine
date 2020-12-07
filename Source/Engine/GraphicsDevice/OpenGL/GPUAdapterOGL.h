// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_OPENGL

#include "Engine/Graphics/Config.h"
#include "Engine/Core/Core.h"
#include "Engine/Graphics/GPUAdapter.h"
#include "Engine/Platform/Platform.h"
#include "IncludeOpenGLHeaders.h"

/// <summary>
/// Graphics Device adapter implementation for OpenGL backend.
/// </summary>
class GPUAdapterOGL : public GPUAdapter
{
public:

	AdapterOGL()
	{
	}

public:

	int32 Version = 0;
	int32 VersionMajor = 0;
	int32 VersionMinor = 0;

	String Vendor;
	String Renderer;
	
	uint32 VendorId = 0;
	bool AmdWorkaround = false;

	typedef StringAnsi Extension;
	Array<Extension> Extensions;

public:

	bool HasExtension(const char* str)
	{
		for (int32 i = 0; i < Extensions.Count(); i++)
		{
			if (Extensions[i] == str)
				return true;
		}
		return false;
	}

public:

	bool Init(HDC deviceContext)
	{
#define CHECK_NULL(obj) if (obj == nullptr) { return true; }

		// Get OpenGL version
		const char* pcVer;
		{
			pcVer = (const char*)glGetString(GL_VERSION);
			ASSERT(pcVer);
			StringAnsi version = pcVer;
			version = version.substr(0, version.find(' '));
			const auto split = version.Find('.');
			if (split == -1)
			{
				VersionMajor = atoi(version.c_str());
			}
			else
			{
				VersionMajor = atoi(version.substr(0, split).Get());
				VersionMinor = atoi(version.substr(split + 1).Get());
			}
		}
		Version = VersionMajor * 100 + VersionMinor * 10;

		// Get GPU info
		const char* pcVendor = (const char*)glGetString(GL_VENDOR);
		CHECK_NULL(pcVendor);
		Vendor = pcVendor;
		const char* pcRenderer = (const char*)glGetString(GL_RENDERER);
		CHECK_NULL(pcRenderer);
		Renderer = pcRenderer;

		// Get extensions list
#if PLATFORM_WINDOWS && false
/*
// Check for Win32 specific extensions probe function
auto _wglGetExtensionsString = (PFNWGLGETEXTENSIONSSTRINGARBPROC)GetGLFuncAddress("wglGetExtensionsString");
CHECK_NULL(_wglGetExtensionsString);
const char* wglExtensions = _wglGetExtensionsString(deviceContext);

// Parse them, and add them to the main list
Extensions.EnsureCapacity(512);
std::stringstream ext;
ext << wglExtensions;
std::string instr;
while (ext >> instr)
    Extensions.Add(instr);
*/
#else
		GLint numExtensions = 0;
		glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
		Extensions.Resize(numExtensions);
		for (GLint i = 0; i < numExtensions; i++)
		{
			Extensions[i] = (const char*)glGetStringi(GL_EXTENSIONS, i);
		}
#endif

// Pick a GPU vendor id
#if PLATFORM_IOS
		VendorId = 0x1010;
#else
		if (Vendor.Contains(TEXT("ATI ")))
		{
			VendorId = 0x1002;
#if PLATFORM_WINDOWS || PLATFORM_LINUX
			AmdWorkaround = true;
#endif
		}
#if PLATFORM_LINUX
		else if (Vendor.Contains(TEXT("X.Org")))
		{
			VendorId = 0x1002;
			bAmdWorkaround = true;
		}
#endif
		else if (Vendor.Contains(TEXT("Intel ")) || Vendor == TEXT("Intel"))
		{
			VendorId = 0x8086;
#if PLATFORM_WINDOWS || PLATFORM_LINUX
			AmdWorkaround = true;
#endif
		}
		else if (Vendor.Contains(TEXT("NVIDIA ")))
		{
			VendorId = 0x10DE;
		}
		else if (Vendor.Contains(TEXT("ImgTec")))
		{
			VendorId = 0x1010;
		}
		else if (Vendor.Contains(TEXT("ARM")))
		{
			VendorId = 0x13B5;
		}
		else if (Vendor.Contains(TEXT("Qualcomm")))
		{
			VendorId = 0x5143;
		}
		if (VendorId == 0)
		{
			// Fix for Mesa Radeon
			if (strstr(pcVer, "Mesa") && (strstr(pcRenderer, "AMD") || strstr(pcRenderer, "ATI")))
			{
				VendorId = 0x1002;
			}
		}
#endif

#undef CHECK_NULL

		return false;
	}

public:

	// [GPUAdapter]
	bool IsValid() const override
	{
		return true;
	}
	uint32 GetVendorId() const override
	{
		return VendorId;
	}
	const Char* GetDescription() const override
	{
		static String desc = Vendor + TEXT(" ") + Renderer;
		return *desc;
	}
};

#endif
