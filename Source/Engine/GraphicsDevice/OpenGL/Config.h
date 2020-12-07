// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Graphics/Config.h"

// Flax uses DirectX versioning to switch between graphics features.
// Version mapping:
// DirectX 10 = OpenGL ES 3.0 = OpenGL 4.1
// DirectX 11 = OpenGl ES 3.1 = OpenGL 4.4

#define SHADER_DATA_FORMAT_RAW 0
#define SHADER_DATA_FORMAT_LZ4 1

// Enables force OpenGL shaders verification
#define GPU_OGL_DEBUG_SHADERS 1

// True if use OpenGL Debug Layer
#define GPU_OGL_USE_DEBUG_LAYER GPU_ENABLE_DIAGNOSTICS

// True if don't release the OpenGL shaders source code, can be used to debug rendering
#define GPU_OGL_KEEP_SHADER_SRC GPU_ENABLE_DIAGNOSTICS
