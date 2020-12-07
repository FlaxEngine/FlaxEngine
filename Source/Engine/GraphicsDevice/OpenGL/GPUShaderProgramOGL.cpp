// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "GPUShaderProgramOGL.h"

#if GRAPHICS_API_OPENGL

#include "../RenderToolsOGL.h"
#include "../GPUDeviceOGL.h"
#include "../GPULimitsOGL.h"

GPUShaderProgramVSOGL::GPUShaderProgramVSOGL(GPUDeviceOGL* device, byte* shaderBytes, uint32 shaderBytesSize, ReadStream& stream, const StringAnsi& name)
	: GPUShaderProgramOGL(GL_VERTEX_SHADER, shaderBytes, shaderBytesSize, stream, name)
	, _device(device)
{
	// Temporary variables
	byte Type, Format, SemanticIndex, InputSlot, InputSlotClass;
	uint32 AlignedByteOffset, InstanceDataStepRate;

	// Load Input Layout (it may be empty)
	byte layoutSize;
	stream.ReadByte(&layoutSize);
	ASSERT(layoutSize <= VERTEX_SHADER_MAX_INPUT_ELEMENTS);
	Layout.SetSize(layoutSize);
	uint32 offset = 0;
	for (int32 a = 0; a < layoutSize; a++)
	{
		auto& e = Layout[a];

		// TODO: cleanup the old vertex input layout loading style to be more robust

		stream.ReadByte(&Type);
		stream.ReadByte(&SemanticIndex);
		stream.ReadByte(&Format);
		stream.ReadByte(&InputSlot);
		stream.ReadUint32(&AlignedByteOffset);
		stream.ReadByte(&InputSlotClass);
		stream.ReadUint32(&InstanceDataStepRate);

		e.BufferSlot = InputSlot;
		e.Format = (PixelFormat)Format;
		e.Size = PixelFormatExtensions::SizeInBytes(e.Format);
		e.InstanceDataStepRate = InstanceDataStepRate;
		e.InputIndex = SemanticIndex;
		e.TypeCount = PixelFormatExtensions::ComputeComponentsCount(e.Format);
		if (PixelFormatExtensions::IsBGRAOrder(e.Format))
			e.TypeCount = GL_BGRA;
		e.Normalized = PixelFormatExtensions::IsNormalized(e.Format) ? GL_TRUE : GL_FALSE;
		e.GlType = _device->GetLimits()->TextureFormats[(int32)e.Format].Type;
		e.IsInteger = e.GlType == GL_SHORT || e.GlType == GL_UNSIGNED_SHORT || e.GlType == GL_INT || e.GlType == GL_UNSIGNED_INT || e.GlType == GL_UNSIGNED_BYTE;

		// Items are always specified in a sequential order
		e.RelativeOffset = AlignedByteOffset == INPUT_LAYOUT_ELEMENT_ALIGN ? offset : AlignedByteOffset;
		offset = (e.RelativeOffset == 0 ? 0 : offset) + e.Size;
	}
}

#endif
