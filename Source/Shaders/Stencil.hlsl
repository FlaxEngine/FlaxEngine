// Copyright (c) Wojciech Figat. All rights reserved.

#ifndef __STENCIL__
#define __STENCIL__

#include "./Flax/Common.hlsl"

// Stencil bits usage (must match RenderBuffers.h)
// [0] | Object Layer
// [1] | Object Layer
// [2] | Object Layer
// [3] | Object Layer
// [4] | Object Layer
// [5] | <unsued>
// [6] | <unsued>
// [7] | <unsued>
#define STENCIL_BUFFER_OBJECT_LAYER(stencil) uint(stencil & 0x1f)
#ifndef STENCIL_BUFFER_SWIZZLE
#define STENCIL_BUFFER_SWIZZLE .g
#endif
#define STENCIL_BUFFER_LOAD(rt, pos) rt.Load(int3(pos, 0)) STENCIL_BUFFER_SWIZZLE

#endif
