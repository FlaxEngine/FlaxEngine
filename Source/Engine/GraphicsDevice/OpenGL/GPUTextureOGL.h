// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Graphics/Textures/GPUTexture.h"
#include "GPUDeviceOGL.h"
#include "GPUResourceOGL.h"
#include "GPUTextureViewOGL.h"

#if GRAPHICS_API_OPENGL

/// <summary>
/// Texture object for OpenGL
/// </summary>
class GPUTextureOGL : public GPUResourceOGL<Texture>
{
private:

	GPUTextureViewOGL _uav;
	GPUTextureViewOGL _handleArray;
	GPUTextureViewOGL _handleVolume;
	Array<GPUTextureViewOGL> _handlesPerSlice; // [slice]
	Array<Array<GPUTextureViewOGL>> _handlesPerMip; // [slice][mip]

public:

	/// <summary>
	/// Initializes a new instance of the <see cref="GPUTextureOGL"/> class.
	/// </summary>
	/// <param name="device">The device.</param>
	/// <param name="name">The name.</param>
	GPUTextureOGL(GPUDeviceOGL* device, const String& name);

	/// <summary>
	/// Finalizes an instance of the <see cref="GPUTextureOGL"/> class.
	/// </summary>
	~GPUTextureOGL()
	{
	}

public:

	GLuint TextureID = 0;
	GLenum Target = 0;
	GLenum FormatGL = 0;

	GPUTextureViewOGL* HandleUAV() const
	{
		ASSERT((GetDescription().Flags & GPUTextureFlags::UnorderedAccess) != 0);
		return (GPUTextureViewOGL*)&_uav;
	}

private:

	void initHandles();

public:

	// [GPUTexture]
	GPUTextureView* Handle(int32 arrayOrDepthIndex) const override
	{
		return (GPUTextureView*)&_handlesPerSlice[arrayOrDepthIndex];
	}
	GPUTextureView* Handle(int32 arrayOrDepthIndex, int32 mipMapIndex) const override
	{
		return (GPUTextureView*)&_handlesPerMip[arrayOrDepthIndex][mipMapIndex];
	}
	GPUTextureView* ViewArray() const override
	{
		ASSERT(ArraySize() > 1);
		return (GPUTextureView*)&_handleArray;
	}
	GPUTextureView* ViewVolume() const override
	{
		ASSERT(IsVolume());
		return (GPUTextureView*)&_handleVolume;
	}
	bool GetData(int32 arrayOrDepthSliceIndex, int32 mipMapIndex, TextureData::MipData& data, uint32 mipRowPitch) override;

protected:

	// [GPUTexture]
	bool OnInit() override;
	void onResidentMipsChanged() override;
	void OnReleaseGPU() override;
};

#endif
