// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#if COMPILE_WITH_DX_SHADER_COMPILER || GRAPHICS_API_DIRECTX12

#include "../IncludeDirectXHeaders.h"

struct DxShaderHeader
{
	/// <summary>
	/// The SRV dimensions per-slot.
	/// </summary>
	byte SrDimensions[32];

	// .. rest is just a actual data array
};

#endif
