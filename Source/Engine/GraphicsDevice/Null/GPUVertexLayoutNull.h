// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_NULL

#include "Engine/Graphics/Shaders/GPUVertexLayout.h"

/// <summary>
/// Vertex layout for Null backend.
/// </summary>
class GPUVertexLayoutNull : public GPUVertexLayout
{
public:
	GPUVertexLayoutNull(const Elements& elements)
		: GPUVertexLayout()
	{
        SetElements(elements, {});
	}
};

#endif
