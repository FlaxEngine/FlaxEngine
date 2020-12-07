// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "TextureOGL.h"

#if GRAPHICS_API_OPENGL

#include "RenderToolsOGL.h"
#include "GPUDeviceOGL.h"
#include "GPULimitsOGL.h"
#include "Engine/Graphics/PixelFormatExtensions.h"

void TextureOGL::initHandles()
{
	// Cache properties
	bool useSRV = IsShaderResource();
	bool useDSV = IsDepthStencil();
	bool useRTV = IsRenderTarget();
	bool useUAV = IsUnorderedAccess();
	int32 arraySize = ArraySize();
	int32 mipLevels = MipLevels();
	bool isArray = arraySize > 1;
	bool isCubeMap = IsCubeMap();
	bool isMsaa = IsMultiSample();
	bool isVolume = IsVolume();

	// Create resource views
	if (useUAV)
	{
		_uav.InitAsFull(this, Format());
	}
	if (isVolume)
	{
		// Create handle for whole 3d texture
		_handleVolume.InitAsFull(this);

		// Init per slice views
		_handlesPerSlice.Resize(Depth(), false);
		if (_desc.HasPerSliceViews() && useRTV)
		{
			for (int32 sliceIndex = 0; sliceIndex < Depth(); sliceIndex++)
			{
				_handlesPerSlice[sliceIndex].InitAsView(this, Format(), GPUTextureViewOGL::ViewType::Texture2D, mipLevels, sliceIndex, 1);
			}
		}
	}
	else if (isArray)
	{
		// Resize handles
		_handlesPerSlice.Resize(ArraySize(), false);

		// Create per array slice handles
		for (int32 arrayIndex = 0; arrayIndex < arraySize; arrayIndex++)
		{
			_handlesPerSlice[arrayIndex].InitAsView(this, Format(), GPUTextureViewOGL::ViewType::Texture2D, mipLevels, arrayIndex, 1);
		}

		// Create whole array handle
		{
			if (isCubeMap)
			{
				_handleArray.InitAsView(this, Format(), GPUTextureViewOGL::ViewType::Texture2D_Array, mipLevels, 0, arraySize);
			}
			else
			{
				_handleArray.InitAsView(this, Format(), GPUTextureViewOGL::ViewType::TextureCube_Array, mipLevels, 0, arraySize);
			}
		}
	}
	else
	{
		// Create single handle for the whole texture
		_handlesPerSlice.Resize(1, false);
		_handlesPerSlice[0].InitAsFull(this);
	}

	// Init per mip map handles
	if (HasPerMipViews())
	{
		// Init handles
		_handlesPerMip.Resize(arraySize, false);
		for (int32 arrayIndex = 0; arrayIndex < arraySize; arrayIndex++)
		{
			auto& slice = _handlesPerMip[arrayIndex];
			slice.Resize(mipLevels, false);

			for (int32 mipIndex = 0; mipIndex < mipLevels; mipIndex++)
			{
				slice[mipIndex].InitAsView(this, Format(), GPUTextureViewOGL::ViewType::Texture2D, mipLevels, arrayIndex, 1, mipIndex);
			}
		}
	}
}

#endif
