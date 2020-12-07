// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_OPENGL

#include "Engine/Graphics/Config.h"
#include "Engine/Graphics/GPULimits.h"
#include "Engine/Graphics/RenderTools.h"
#include "GPUDeviceOGL.h"
#if BUILD_DEBUG
#include "Engine/Core/Types/StringBuilder.h"
#endif

struct TextureFormatOGL
{
public:

	GLenum InternalFormat;
	GLenum Format;
	GLenum Type;
	bool IsCompressed;

public:

	TextureFormatOGL()
	{
		InternalFormat = GL_NONE;
		Format = GL_NONE;
		Type = GL_NONE;
		IsCompressed = false;
	}

	TextureFormatOGL(GLenum internalFormat, GLenum format, GLenum type, bool isCompressed = false, bool isBGRA = false)
	{
		InternalFormat = internalFormat;
		Format = format;
		Type = type;
		IsCompressed = isCompressed;
	}
};

/// <summary>
/// Implementation of GPU Limits for OpenGL
/// </summary>
/// <seealso cref="GPULimits" />
class GPULimitsOGL : public GPULimits
{
protected:

	GPUDeviceOGL * _device;

public:

	/// <summary>
	/// Initializes a new instance of the <see cref="GPULimitsOGL"/> class.
	/// </summary>
	/// <param name="device">The device.</param>
	GPULimitsOGL(GPUDeviceOGL* device)
		: _device(device)
	{
	}

public:

	bool SupportsTessellation;
	bool SupportsGPUMemoryInfo;
	bool SupportsComputeShaders;
	bool SupportsVertexAttribBinding;
	bool SupportsTextureView;
	bool SupportsVolumeTextureRendering;
	bool SupportsASTC;
	bool SupportsCopyImage;
	bool SupportsSeamlessCubemap;
	bool SupportsTextureFilterAnisotropic;
	bool SupportsDrawBuffersBlend;
	bool SupportsSeparateShaderObjects;
	bool SupportsClipControl;

	uint64 VideoMemorySize;
	int32 MaxTextureMipCount;
	int32 MaxTextureSize;
	int32 MaxCubeTextureSize;
	int32 MaxVolumeTextureSize;
	int32 MaxTextureArraySize;
	int32 MaxOpenGLDrawBuffers;
	int32 MaxTextureImageUnits;
	int32 MaxCombinedTextureImageUnits;
	int32 MaxVertexTextureImageUnits;
	int32 MaxGeometryTextureImageUnits;
	int32 MaxHullTextureImageUnits;
	int32 MaxDomainTextureImageUnits;
	int32 MaxVaryingVectors;
	int32 MaxVertexUniformComponents;
	int32 MaxPixelUniformComponents;
	int32 MaxGeometryUniformComponents;
	int32 MaxHullUniformComponents;
	int32 MaxDomainUniformComponents;
	int32 MaxComputeTextureImageUnits;
	int32 MaxComputeUniformComponents;

	TextureFormatOGL TextureFormats[static_cast<int32>(PixelFormat::Maximum)];

public:

	GLenum GetInternalTextureFormat(PixelFormat format)
	{
		return TextureFormats[(int32)format].InternalFormat;
	}

	GLenum GetInternalTextureFormat(PixelFormat format, GPUTextureFlags flags)
	{
		GLenum f = TextureFormats[(int32)format].InternalFormat;

		// Correct GL texure format
		if (flags & GPUTextureFlags::DepthStencil)
		{
			if (f == GL_R32F)
				f = GL_DEPTH_COMPONENT32F;
			else if (f == GL_R16)
				f = GL_DEPTH_COMPONENT16;
		}

		return f;
	}

private:

	void InitFormats()
	{
		// References:
// http://www.opengl.org/wiki/Image_Format
// http://www.g-truc.net/post-0335.html
		// http://renderingpipeline.com/2012/07/texture-compression/

		TextureFormats[(int32)PixelFormat::Unknown] = TextureFormatOGL();

		TextureFormats[(int32)PixelFormat::R32G32B32A32_Typeless] = TextureFormatOGL(GL_RGBA32F, GL_RGBA, GL_FLOAT);
		TextureFormats[(int32)PixelFormat::R32G32B32A32_Float] = TextureFormatOGL(GL_RGBA32F, GL_RGBA, GL_FLOAT);
		TextureFormats[(int32)PixelFormat::R32G32B32A32_UInt] = TextureFormatOGL(GL_RGBA32UI, GL_RGBA_INTEGER, GL_UNSIGNED_INT);
		TextureFormats[(int32)PixelFormat::R32G32B32A32_SInt] = TextureFormatOGL(GL_RGBA32I, GL_RGBA_INTEGER, GL_INT);

		TextureFormats[(int32)PixelFormat::R32G32B32_Typeless] = TextureFormatOGL(GL_RGB32F, GL_RGB, GL_FLOAT);
		TextureFormats[(int32)PixelFormat::R32G32B32_Float] = TextureFormatOGL(GL_RGB32F, GL_RGB, GL_FLOAT);
		TextureFormats[(int32)PixelFormat::R32G32B32_UInt] = TextureFormatOGL(GL_RGB32UI, GL_RGB_INTEGER, GL_UNSIGNED_INT);
		TextureFormats[(int32)PixelFormat::R32G32B32_SInt] = TextureFormatOGL(GL_RGB32I, GL_RGB_INTEGER, GL_INT);

		TextureFormats[(int32)PixelFormat::R16G16B16A16_Typeless] = TextureFormatOGL(GL_RGBA16F, GL_RGBA, GL_HALF_FLOAT);
		TextureFormats[(int32)PixelFormat::R16G16B16A16_Float] = TextureFormatOGL(GL_RGBA16F, GL_RGBA, GL_HALF_FLOAT);
		TextureFormats[(int32)PixelFormat::R16G16B16A16_UNorm] = TextureFormatOGL(GL_RGBA16, GL_RGBA, GL_UNSIGNED_SHORT);
		TextureFormats[(int32)PixelFormat::R16G16B16A16_UInt] = TextureFormatOGL(GL_RGBA16UI, GL_RGBA_INTEGER, GL_UNSIGNED_SHORT);
		TextureFormats[(int32)PixelFormat::R16G16B16A16_SNorm] = TextureFormatOGL(GL_RGBA16_SNORM, GL_RGBA, GL_SHORT);
		TextureFormats[(int32)PixelFormat::R16G16B16A16_SInt] = TextureFormatOGL(GL_RGBA16I, GL_RGBA_INTEGER, GL_SHORT);

		TextureFormats[(int32)PixelFormat::R32G32_Typeless] = TextureFormatOGL(GL_RG32F, GL_RG, GL_FLOAT);
		TextureFormats[(int32)PixelFormat::R32G32_Float] = TextureFormatOGL(GL_RG32F, GL_RG, GL_FLOAT);
		TextureFormats[(int32)PixelFormat::R32G32_UInt] = TextureFormatOGL(GL_RG32UI, GL_RG_INTEGER, GL_UNSIGNED_INT);
		TextureFormats[(int32)PixelFormat::R32G32_SInt] = TextureFormatOGL(GL_RG32I, GL_RG_INTEGER, GL_INT);

		TextureFormats[(int32)PixelFormat::R32G8X24_Typeless] = TextureFormatOGL(GL_DEPTH32F_STENCIL8, GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV);
		TextureFormats[(int32)PixelFormat::D32_Float_S8X24_UInt] = TextureFormatOGL(GL_DEPTH32F_STENCIL8, GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV);
		TextureFormats[(int32)PixelFormat::R32_Float_X8X24_Typeless] = TextureFormatOGL(GL_DEPTH32F_STENCIL8, GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV);
		TextureFormats[(int32)PixelFormat::X32_Typeless_G8X24_UInt] = TextureFormatOGL(GL_DEPTH32F_STENCIL8, GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV);

		TextureFormats[(int32)PixelFormat::R10G10B10A2_Typeless] = TextureFormatOGL(GL_RGB10_A2, GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV);
		TextureFormats[(int32)PixelFormat::R10G10B10A2_UNorm] = TextureFormatOGL(GL_RGB10_A2, GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV);
		TextureFormats[(int32)PixelFormat::R10G10B10A2_UInt] = TextureFormatOGL(GL_RGB10_A2UI, GL_RGBA_INTEGER, GL_UNSIGNED_INT_2_10_10_10_REV);
		TextureFormats[(int32)PixelFormat::R11G11B10_Float] = TextureFormatOGL(GL_R11F_G11F_B10F, GL_RGB, GL_UNSIGNED_INT_10F_11F_11F_REV);

		TextureFormats[(int32)PixelFormat::R8G8B8A8_Typeless] = TextureFormatOGL(GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
		TextureFormats[(int32)PixelFormat::R8G8B8A8_UNorm] = TextureFormatOGL(GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
		TextureFormats[(int32)PixelFormat::R8G8B8A8_UNorm_sRGB] = TextureFormatOGL(GL_SRGB8_ALPHA8, GL_RGBA, GL_UNSIGNED_BYTE);
		TextureFormats[(int32)PixelFormat::R8G8B8A8_UInt] = TextureFormatOGL(GL_RGBA8UI, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE);
		TextureFormats[(int32)PixelFormat::R8G8B8A8_SNorm] = TextureFormatOGL(GL_RGBA8_SNORM, GL_RGBA, GL_BYTE);
		TextureFormats[(int32)PixelFormat::R8G8B8A8_SInt] = TextureFormatOGL(GL_RGBA8I, GL_RGBA_INTEGER, GL_BYTE);

		TextureFormats[(int32)PixelFormat::R16G16_Typeless] = TextureFormatOGL(GL_RG16F, GL_RG, GL_HALF_FLOAT);
		TextureFormats[(int32)PixelFormat::R16G16_Float] = TextureFormatOGL(GL_RG16F, GL_RG, GL_HALF_FLOAT);
		TextureFormats[(int32)PixelFormat::R16G16_UNorm] = TextureFormatOGL(GL_RG16, GL_RG, GL_UNSIGNED_SHORT);
		TextureFormats[(int32)PixelFormat::R16G16_UInt] = TextureFormatOGL(GL_RG16UI, GL_RG_INTEGER, GL_UNSIGNED_SHORT);
		TextureFormats[(int32)PixelFormat::R16G16_SNorm] = TextureFormatOGL(GL_RG16_SNORM, GL_RG, GL_SHORT);
		TextureFormats[(int32)PixelFormat::R16G16_SInt] = TextureFormatOGL(GL_RG16I, GL_RG_INTEGER, GL_SHORT);

		TextureFormats[(int32)PixelFormat::R32_Typeless] = TextureFormatOGL(GL_R32F, GL_RED, GL_FLOAT);
		TextureFormats[(int32)PixelFormat::D32_Float] = TextureFormatOGL(GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT);
		TextureFormats[(int32)PixelFormat::R32_Float] = TextureFormatOGL(GL_R32F, GL_RED, GL_FLOAT);
		TextureFormats[(int32)PixelFormat::R32_UInt] = TextureFormatOGL(GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT);
		TextureFormats[(int32)PixelFormat::R32_SInt] = TextureFormatOGL(GL_R32I, GL_RED_INTEGER, GL_INT);

		TextureFormats[(int32)PixelFormat::R24G8_Typeless] = TextureFormatOGL(GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8);
		TextureFormats[(int32)PixelFormat::D24_UNorm_S8_UInt] = TextureFormatOGL(GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8);
		TextureFormats[(int32)PixelFormat::R24_UNorm_X8_Typeless] = TextureFormatOGL(GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8);
		TextureFormats[(int32)PixelFormat::X24_Typeless_G8_UInt] = TextureFormatOGL(GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8);

		TextureFormats[(int32)PixelFormat::R8G8_Typeless] = TextureFormatOGL(GL_RG8, GL_RG, GL_UNSIGNED_BYTE);
		TextureFormats[(int32)PixelFormat::R8G8_UNorm] = TextureFormatOGL(GL_RG8, GL_RG, GL_UNSIGNED_BYTE);
		TextureFormats[(int32)PixelFormat::R8G8_UInt] = TextureFormatOGL(GL_RG8UI, GL_RG_INTEGER, GL_UNSIGNED_BYTE);
		TextureFormats[(int32)PixelFormat::R8G8_SNorm] = TextureFormatOGL(GL_RG8_SNORM, GL_RG, GL_BYTE);
		TextureFormats[(int32)PixelFormat::R8G8_SInt] = TextureFormatOGL(GL_RG8I, GL_RG_INTEGER, GL_BYTE);

		TextureFormats[(int32)PixelFormat::R16_Typeless] = TextureFormatOGL(GL_R16F, GL_RED, GL_HALF_FLOAT);
		TextureFormats[(int32)PixelFormat::R16_Float] = TextureFormatOGL(GL_R16F, GL_RED, GL_HALF_FLOAT);
		TextureFormats[(int32)PixelFormat::D16_UNorm] = TextureFormatOGL(GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT);
		TextureFormats[(int32)PixelFormat::R16_UNorm] = TextureFormatOGL(GL_R16, GL_RED, GL_UNSIGNED_SHORT);
		TextureFormats[(int32)PixelFormat::R16_UInt] = TextureFormatOGL(GL_R16UI, GL_RED_INTEGER, GL_UNSIGNED_SHORT);
		TextureFormats[(int32)PixelFormat::R16_SNorm] = TextureFormatOGL(GL_R16_SNORM, GL_RED, GL_SHORT);
		TextureFormats[(int32)PixelFormat::R16_SInt] = TextureFormatOGL(GL_R16I, GL_RED_INTEGER, GL_SHORT);

		TextureFormats[(int32)PixelFormat::R8_Typeless] = TextureFormatOGL(GL_R8, GL_RED, GL_UNSIGNED_BYTE);
		TextureFormats[(int32)PixelFormat::R8_UNorm] = TextureFormatOGL(GL_R8, GL_RED, GL_UNSIGNED_BYTE);
		TextureFormats[(int32)PixelFormat::R8_UInt] = TextureFormatOGL(GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE);
		TextureFormats[(int32)PixelFormat::R8_SNorm] = TextureFormatOGL(GL_R8_SNORM, GL_RED, GL_BYTE);
		TextureFormats[(int32)PixelFormat::R8_SInt] = TextureFormatOGL(GL_R8I, GL_RED_INTEGER, GL_BYTE);
		TextureFormats[(int32)PixelFormat::A8_UNorm] = TextureFormatOGL();

		TextureFormats[(int32)PixelFormat::R1_UNorm] = TextureFormatOGL();

		TextureFormats[(int32)PixelFormat::R9G9B9E5_SharedExp] = TextureFormatOGL(GL_RGB9_E5, GL_RGB, GL_UNSIGNED_INT_5_9_9_9_REV);
		TextureFormats[(int32)PixelFormat::R8G8_B8G8_UNorm] = TextureFormatOGL();
		TextureFormats[(int32)PixelFormat::G8R8_G8B8_UNorm] = TextureFormatOGL();

		TextureFormats[(int32)PixelFormat::BC1_Typeless] = TextureFormatOGL(GL_COMPRESSED_RGB_S3TC_DXT1_EXT, GL_RGB, GL_UNSIGNED_BYTE, true);
		TextureFormats[(int32)PixelFormat::BC1_UNorm] = TextureFormatOGL(GL_COMPRESSED_RGB_S3TC_DXT1_EXT, GL_RGB, GL_UNSIGNED_BYTE, true);
		TextureFormats[(int32)PixelFormat::BC1_UNorm_sRGB] = TextureFormatOGL(GL_COMPRESSED_SRGB_S3TC_DXT1_EXT, GL_RGB, GL_UNSIGNED_BYTE, true);
		TextureFormats[(int32)PixelFormat::BC2_Typeless] = TextureFormatOGL(GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, GL_RGBA, GL_UNSIGNED_BYTE, true);
		TextureFormats[(int32)PixelFormat::BC2_UNorm] = TextureFormatOGL(GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, GL_RGBA, GL_UNSIGNED_BYTE, true);
		TextureFormats[(int32)PixelFormat::BC2_UNorm_sRGB] = TextureFormatOGL(GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT, GL_RGBA, GL_UNSIGNED_BYTE, true);
		TextureFormats[(int32)PixelFormat::BC3_Typeless] = TextureFormatOGL(GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, GL_RGBA, GL_UNSIGNED_BYTE, true);
		TextureFormats[(int32)PixelFormat::BC3_UNorm] = TextureFormatOGL(GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, GL_RGBA, GL_UNSIGNED_BYTE, true);
		TextureFormats[(int32)PixelFormat::BC3_UNorm_sRGB] = TextureFormatOGL(GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT, GL_RGBA, GL_UNSIGNED_BYTE, true);
		TextureFormats[(int32)PixelFormat::BC4_Typeless] = TextureFormatOGL(GL_COMPRESSED_RED_RGTC1, GL_RED, GL_UNSIGNED_BYTE, true);
		TextureFormats[(int32)PixelFormat::BC4_UNorm] = TextureFormatOGL(GL_COMPRESSED_RED_RGTC1, GL_RED, GL_UNSIGNED_BYTE, true);
		TextureFormats[(int32)PixelFormat::BC4_SNorm] = TextureFormatOGL(GL_COMPRESSED_SIGNED_RED_RGTC1, GL_RED, GL_UNSIGNED_BYTE, true);
		TextureFormats[(int32)PixelFormat::BC5_Typeless] = TextureFormatOGL(GL_COMPRESSED_RG_RGTC2, GL_RG, GL_UNSIGNED_BYTE, true);
		TextureFormats[(int32)PixelFormat::BC5_UNorm] = TextureFormatOGL(GL_COMPRESSED_RG_RGTC2, GL_RG, GL_UNSIGNED_BYTE, true);
		TextureFormats[(int32)PixelFormat::BC5_SNorm] = TextureFormatOGL(GL_COMPRESSED_SIGNED_RG_RGTC2, GL_RG, GL_UNSIGNED_BYTE, true);
		TextureFormats[(int32)PixelFormat::B5G6R5_UNorm] = TextureFormatOGL(GL_RGB565, GL_RGB, GL_UNSIGNED_SHORT_5_6_5_REV);
		TextureFormats[(int32)PixelFormat::B5G5R5A1_UNorm] = TextureFormatOGL(GL_RGB5_A1, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV);
		TextureFormats[(int32)PixelFormat::B8G8R8A8_UNorm] = TextureFormatOGL();
		TextureFormats[(int32)PixelFormat::B8G8R8X8_UNorm] = TextureFormatOGL();
		TextureFormats[(int32)PixelFormat::R10G10B10_Xr_Bias_A2_UNorm] = TextureFormatOGL();
		TextureFormats[(int32)PixelFormat::B8G8R8A8_Typeless] = TextureFormatOGL(GL_RGBA8, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV);
		TextureFormats[(int32)PixelFormat::B8G8R8A8_UNorm_sRGB] = TextureFormatOGL(GL_SRGB8_ALPHA8, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV);
		TextureFormats[(int32)PixelFormat::B8G8R8X8_Typeless] = TextureFormatOGL(GL_RGBA8, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV);
		TextureFormats[(int32)PixelFormat::B8G8R8X8_UNorm_sRGB] = TextureFormatOGL(GL_SRGB8_ALPHA8, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV);
		TextureFormats[(int32)PixelFormat::BC6H_Typeless] = TextureFormatOGL(GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT, GL_RGB, GL_UNSIGNED_BYTE, true);
		TextureFormats[(int32)PixelFormat::BC6H_Uf16] = TextureFormatOGL(GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT, GL_RGB, GL_UNSIGNED_BYTE, true);
		TextureFormats[(int32)PixelFormat::BC6H_Sf16] = TextureFormatOGL(GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT, GL_RGB, GL_UNSIGNED_BYTE, true);
		TextureFormats[(int32)PixelFormat::BC7_Typeless] = TextureFormatOGL(GL_COMPRESSED_RGBA_BPTC_UNORM, GL_RGB, GL_UNSIGNED_BYTE, true);
		TextureFormats[(int32)PixelFormat::BC7_UNorm] = TextureFormatOGL(GL_COMPRESSED_RGBA_BPTC_UNORM, GL_RGB, GL_UNSIGNED_BYTE, true);
		TextureFormats[(int32)PixelFormat::BC7_UNorm_sRGB] = TextureFormatOGL(GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM, GL_RGB, GL_UNSIGNED_BYTE, true);

		// Check features for each PixelFormat
		for (int32 i = 0; i < static_cast<int32>(PixelFormat::Maximum); i++)
		{
			// TODO: try to detect MSAA or some feature levels like on D3D11
			auto format = static_cast<PixelFormat>(i);
			auto& info = TextureFormats[i];
			FormatSupport support = FormatSupport::None;
			if (info.Format != 0)
			{
				support = FormatSupport::Texture1D | FormatSupport::Texture2D | FormatSupport::Texture3D | FormatSupport::DepthStencil | FormatSupport::Buffer;
			}
			_featuresPerFormat[i] = FeaturesPerFormat(format, MSAALevel::None, support);
		}
	}

#if BUILD_DEBUG

	void PrintStats()
	{
#define PRINT_STAT(name) sb.AppendFormat(TEXT("{0} = {1}\n"), TEXT(#name), name)

		StringBuilder sb;
		sb.AppendLine();
		sb.AppendLine();

		auto adapter = (AdapterOGL*)_device->GetAdapter();
		sb.AppendFormat(TEXT("OpenGL {0}.{1}\n"), adapter->VersionMajor, adapter->VersionMinor);

		sb.AppendLine();
		PRINT_STAT(SupportsTessellation);
		PRINT_STAT(SupportsGPUMemoryInfo);
		PRINT_STAT(SupportsComputeShaders);
		PRINT_STAT(SupportsVertexAttribBinding);
		PRINT_STAT(SupportsTextureView);
		PRINT_STAT(SupportsVolumeTextureRendering);
		PRINT_STAT(SupportsASTC);
		PRINT_STAT(SupportsCopyImage);
		PRINT_STAT(SupportsSeamlessCubemap);
		PRINT_STAT(SupportsTextureFilterAnisotropic);
		PRINT_STAT(SupportsDrawBuffersBlend);
		PRINT_STAT(SupportsSeparateShaderObjects);
		PRINT_STAT(SupportsClipControl);

		sb.AppendLine();
		PRINT_STAT(VideoMemorySize);
		PRINT_STAT(MaxTextureMipCount);
		PRINT_STAT(MaxTextureSize);
		PRINT_STAT(MaxCubeTextureSize);
		PRINT_STAT(MaxVolumeTextureSize);
		PRINT_STAT(MaxTextureArraySize);
		PRINT_STAT(MaxOpenGLDrawBuffers);
		PRINT_STAT(MaxTextureImageUnits);
		PRINT_STAT(MaxCombinedTextureImageUnits);
		PRINT_STAT(MaxVertexTextureImageUnits);
		PRINT_STAT(MaxGeometryTextureImageUnits);
		PRINT_STAT(MaxHullTextureImageUnits);
		PRINT_STAT(MaxDomainTextureImageUnits);
		PRINT_STAT(MaxVaryingVectors);
		PRINT_STAT(MaxVertexUniformComponents);
		PRINT_STAT(MaxPixelUniformComponents);
		PRINT_STAT(MaxGeometryUniformComponents);
		PRINT_STAT(MaxHullUniformComponents);
		PRINT_STAT(MaxDomainUniformComponents);
		PRINT_STAT(MaxComputeTextureImageUnits);
		PRINT_STAT(MaxComputeUniformComponents);

		sb.AppendLine();

		LOG_STR(Info, sb.ToString());

#undef PRINT_STAT
	}

#endif

public:

	// [GPULimits]
	bool Init() override
	{
		auto adapter = (AdapterOGL*)_device->GetAdapter();
		auto VersionMajor = adapter->VersionMajor;
		auto VersionMinor = adapter->VersionMinor;

		// Test graphics pipeline features support
		SupportsTessellation = (VersionMajor >= 4) || adapter->HasExtension("GL_ARB_tessellation_shader");
		SupportsGPUMemoryInfo = adapter->HasExtension("GL_NVX_gpu_memory_info");
		SupportsComputeShaders = (VersionMajor == 4 && VersionMinor >= 3) || (VersionMajor > 4) || adapter->HasExtension("GL_ARB_compute_shader");
		SupportsVertexAttribBinding = (VersionMajor == 4 && VersionMinor >= 3) || (VersionMajor > 4) || adapter->HasExtension("GL_ARB_vertex_attrib_binding");
		SupportsTextureView = (VersionMajor == 4 && VersionMinor >= 3) || (VersionMajor > 4) || adapter->HasExtension("GL_ARB_texture_view");
		SupportsASTC = adapter->HasExtension("GL_KHR_texture_compression_astc_ldr");
		SupportsCopyImage = adapter->HasExtension("GL_ARB_copy_image");
		SupportsSeamlessCubemap = adapter->HasExtension("GL_ARB_seamless_cube_map");
		SupportsTextureFilterAnisotropic = adapter->HasExtension("GL_EXT_texture_filter_anisotropic");
		SupportsDrawBuffersBlend = adapter->HasExtension("GL_ARB_draw_buffers_blend");
		SupportsClipControl = adapter->HasExtension("GL_ARB_clip_control");
#if GRAPHICS_API_OPENGL_ES
		SupportsSeparateShaderObjects = false;
#else
		SupportsSeparateShaderObjects = (VersionMajor == 4 && VersionMinor >= 4) || adapter->HasExtension("GL_ARB_separate_shader_objects");
#endif

		// Get video memory size (in bytes)
		VideoMemorySize = 0;
		if (SupportsGPUMemoryInfo)
		{
			GLint VMSizeKB = 0;
			glGetIntegerv(GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX, &VMSizeKB);
			VideoMemorySize = VMSizeKB * 1024ll;
		}

		// Test whether the GPU can support volume-texture rendering.
		// There is no API to query this - you just have to test whether a 3D texture is framebuffer-complete.
		{
			GLuint frameBuffer;
			glGenFramebuffers(1, &frameBuffer);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frameBuffer);
			GLuint volumeTexture;
			glGenTextures(1, &volumeTexture);
			glBindTexture(GL_TEXTURE_3D, volumeTexture);
			glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8, 256, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
			glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, volumeTexture, 0);

			SupportsVolumeTextureRendering = (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

			glDeleteTextures(1, &volumeTexture);
			glDeleteFramebuffers(1, &frameBuffer);
		}

		// Get the device limits with OpenGL API
#define LOG_AND_GET_GL_INT_TEMP(IntEnum, Default) GLint Value_##IntEnum = Default; if (IntEnum) {glGetIntegerv(IntEnum, &Value_##IntEnum); glGetError();} else {Value_##IntEnum = Default;}
		LOG_AND_GET_GL_INT_TEMP(GL_MAX_TEXTURE_SIZE, 0);
		LOG_AND_GET_GL_INT_TEMP(GL_MAX_CUBE_MAP_TEXTURE_SIZE, 0);
		LOG_AND_GET_GL_INT_TEMP(GL_MAX_ARRAY_TEXTURE_LAYERS, 0);
		LOG_AND_GET_GL_INT_TEMP(GL_MAX_3D_TEXTURE_SIZE, 0);
		LOG_AND_GET_GL_INT_TEMP(GL_MAX_RENDERBUFFER_SIZE, 0);
		LOG_AND_GET_GL_INT_TEMP(GL_MAX_TEXTURE_IMAGE_UNITS, 0);
		LOG_AND_GET_GL_INT_TEMP(GL_MAX_DRAW_BUFFERS, 1);
		LOG_AND_GET_GL_INT_TEMP(GL_MAX_COLOR_ATTACHMENTS, 1);
		LOG_AND_GET_GL_INT_TEMP(GL_MAX_SAMPLES, 1);
		LOG_AND_GET_GL_INT_TEMP(GL_MAX_COLOR_TEXTURE_SAMPLES, 1);
		LOG_AND_GET_GL_INT_TEMP(GL_MAX_DEPTH_TEXTURE_SAMPLES, 1);
		LOG_AND_GET_GL_INT_TEMP(GL_MAX_INTEGER_SAMPLES, 1);
		LOG_AND_GET_GL_INT_TEMP(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, 0);
		LOG_AND_GET_GL_INT_TEMP(GL_MAX_VERTEX_ATTRIBS, 0);
		LOG_AND_GET_GL_INT_TEMP(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, 0);
		LOG_AND_GET_GL_INT_TEMP(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, 0);
		LOG_AND_GET_GL_INT_TEMP(GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS, 0);
		LOG_AND_GET_GL_INT_TEMP(GL_MAX_TESS_CONTROL_TEXTURE_IMAGE_UNITS, 0);
		LOG_AND_GET_GL_INT_TEMP(GL_MAX_TESS_EVALUATION_TEXTURE_IMAGE_UNITS, 0);
		LOG_AND_GET_GL_INT_TEMP(GL_MAX_VERTEX_UNIFORM_COMPONENTS, 0);
		LOG_AND_GET_GL_INT_TEMP(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, 0);
		LOG_AND_GET_GL_INT_TEMP(GL_MAX_GEOMETRY_UNIFORM_COMPONENTS, 0);
		LOG_AND_GET_GL_INT_TEMP(GL_MAX_VARYING_VECTORS, 0);

		// Setup the actual limits
		MaxTextureSize = Value_GL_MAX_TEXTURE_SIZE;
		MaxTextureMipCount = Math::Min<int32>(GPU_MAX_TEXTURE_MIP_LEVELS, MipLevelsCount(MaxTextureSize));
		MaxCubeTextureSize = Value_GL_MAX_CUBE_MAP_TEXTURE_SIZE;
		MaxTextureArraySize = Value_GL_MAX_ARRAY_TEXTURE_LAYERS;
		MaxVolumeTextureSize = MaxCubeTextureSize;
		MaxOpenGLDrawBuffers = Value_GL_MAX_DRAW_BUFFERS;
		MaxTextureImageUnits = Value_GL_MAX_TEXTURE_IMAGE_UNITS;
		MaxCombinedTextureImageUnits = Value_GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS;
		MaxVertexTextureImageUnits = Value_GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS;
		MaxGeometryTextureImageUnits = Value_GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS;
		MaxHullTextureImageUnits = Value_GL_MAX_TESS_CONTROL_TEXTURE_IMAGE_UNITS;
		MaxDomainTextureImageUnits = Value_GL_MAX_TESS_EVALUATION_TEXTURE_IMAGE_UNITS;
		MaxVaryingVectors = Value_GL_MAX_VARYING_VECTORS;
		MaxVertexUniformComponents = Value_GL_MAX_VERTEX_UNIFORM_COMPONENTS;
		MaxPixelUniformComponents = Value_GL_MAX_FRAGMENT_UNIFORM_COMPONENTS;
		MaxGeometryUniformComponents = Value_GL_MAX_GEOMETRY_UNIFORM_COMPONENTS;
		if (SupportsTessellation)
		{
			LOG_AND_GET_GL_INT_TEMP(GL_MAX_TESS_CONTROL_UNIFORM_COMPONENTS, 0);
			LOG_AND_GET_GL_INT_TEMP(GL_MAX_TESS_EVALUATION_UNIFORM_COMPONENTS, 0);
			MaxHullUniformComponents = Value_GL_MAX_TESS_CONTROL_UNIFORM_COMPONENTS;
			MaxDomainUniformComponents = Value_GL_MAX_TESS_EVALUATION_UNIFORM_COMPONENTS;
		}
		else
		{
			MaxHullUniformComponents = 0;
			MaxDomainUniformComponents = 0;
		}
		if (SupportsComputeShaders)
		{
			LOG_AND_GET_GL_INT_TEMP(GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS, 0);
			LOG_AND_GET_GL_INT_TEMP(GL_MAX_COMPUTE_UNIFORM_COMPONENTS, 0);
			MaxComputeTextureImageUnits = Value_GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS;
			MaxComputeUniformComponents = Value_GL_MAX_COMPUTE_UNIFORM_COMPONENTS;
		}
		else
		{
			MaxComputeTextureImageUnits = 0;
			MaxComputeUniformComponents = 0;
		}

		// For now, just allocate additional units if available and advertise no tessellation units for HW that can't handle more
		if (MaxCombinedTextureImageUnits < 48)
		{
			// To work around AMD driver limitation of 32 GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS,
// Going to hard code this for now (16 units in PS, 8 units in VS, 8 units in GS).
			// This is going to be a problem for tessellation.
			MaxTextureImageUnits = MaxTextureImageUnits > 16 ? 16 : MaxTextureImageUnits;
			MaxVertexTextureImageUnits = MaxVertexTextureImageUnits > 8 ? 8 : MaxVertexTextureImageUnits;
			MaxGeometryTextureImageUnits = MaxGeometryTextureImageUnits > 8 ? 8 : MaxGeometryTextureImageUnits;
			MaxHullTextureImageUnits = 0;
			MaxDomainTextureImageUnits = 0;
			MaxCombinedTextureImageUnits = MaxCombinedTextureImageUnits > 32 ? 32 : MaxCombinedTextureImageUnits;
		}
		else
		{
			// Clamp things to the levels that the other path is going, but allow additional units for tessellation
			MaxTextureImageUnits = MaxTextureImageUnits > 16 ? 16 : MaxTextureImageUnits;
			MaxVertexTextureImageUnits = MaxVertexTextureImageUnits > 8 ? 8 : MaxVertexTextureImageUnits;
			MaxGeometryTextureImageUnits = MaxGeometryTextureImageUnits > 8 ? 8 : MaxGeometryTextureImageUnits;
			MaxHullTextureImageUnits = MaxHullTextureImageUnits > 8 ? 8 : MaxHullTextureImageUnits;
			MaxDomainTextureImageUnits = MaxDomainTextureImageUnits > 8 ? 8 : MaxDomainTextureImageUnits;
			MaxCombinedTextureImageUnits = MaxCombinedTextureImageUnits > 48 ? 48 : MaxCombinedTextureImageUnits;
		}

		InitFormats();

#if BUILD_DEBUG
		PrintStats();
#endif

		// Validate minimum specs for the engine to start
		if (!SupportsTextureView)
		{
			LOG(Error, "The GPU does not meet minimal requirements.");
			return true;
		}

		return false;
	}
	bool HasCompute() const override
	{
		return SupportsComputeShaders;
	}
	bool HasTessellation() const override
	{
		return SupportsTessellation;
	}
	bool HasGeometryShaders() const override
	{
#if GRAPHICS_API_OPENGLES
		return false;
#else
		return true;
#endif
	}
	bool HasVolumeTextureRendering() const override
	{
		return SupportsVolumeTextureRendering;
	}
	bool HasDrawIndirect() const override
	{
		return false;
	}
	bool HasAppendConsumeBuffers() const override
	{
		return false;
	}
	bool HasSeparateRenderTargetBlendState() const override
	{
		return false;
	}
	bool HasDepthAsSRV() const override
	{
		return true;
	}
	bool HasMultisampleDepthAsSRV() const override
	{
		return true;
	}
	int32 MaximumMipLevelsCount() const override
	{
		return MaxTextureMipCount;
	}
	int32 MaximumTexture1DSize() const override
	{
		return MaxTextureSize;
	}
	int32 MaximumTexture1DArraySize() const override
	{
		return MaxTextureArraySize;
	}
	int32 MaximumTexture2DSize() const override
	{
		return MaxTextureSize;
	}
	int32 MaximumTexture2DArraySize() const override
	{
		return MaxTextureArraySize;
	}
	int32 MaximumTexture3DSize() const override
	{
		return MaxVolumeTextureSize;
	}
	int32 MaximumTextureCubeSize() const override
	{
		return MaxCubeTextureSize;
	}
};

#endif
