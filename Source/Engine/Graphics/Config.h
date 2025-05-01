// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Platform/Defines.h"

// Maximum amount of binded render targets at the same time
#define GPU_MAX_RT_BINDED 6

// Maximum amount of binded shader resources at the same time
#define GPU_MAX_SR_BINDED 32

// Maximum amount of binded constant buffers at the same time
#define GPU_MAX_CB_BINDED 4

// Maximum amount of binded unordered access resources at the same time
#define GPU_MAX_UA_BINDED 4

// Maximum amount of binded texture sampler resources at the same time
#define GPU_MAX_SAMPLER_BINDED 16

// Amount of initial slots used for global samplers (static, 4 common samplers + 2 comparision samplers)
#define GPU_STATIC_SAMPLERS_COUNT 6

// Maximum amount of binded vertex buffers at the same time
#define GPU_MAX_VB_BINDED 4

// Maximum amount of vertex shader input elements in a layout
#define GPU_MAX_VS_ELEMENTS 16

// Maximum amount of thread groups per dimension for compute dispatch
#define GPU_MAX_CS_DISPATCH_THREAD_GROUPS 65535

// Alignment of the shader data structures (16-byte boundaries) to improve memory copies efficiency.
#define GPU_SHADER_DATA_ALIGNMENT 16

// Enable/disable assertion for graphics layers
#define GPU_ENABLE_ASSERTION 1
#define GPU_ENABLE_ASSERTION_LOW_LAYERS (!BUILD_RELEASE)

// Enable/disable dynamic textures quality streaming
#define GPU_ENABLE_TEXTURES_STREAMING 1

// Enable/disable creating Shader Resource View for window backbuffer surface
#define GPU_USE_WINDOW_SRV 1

// True if allow graphics profile events and markers
#define GPU_ALLOW_PROFILE_EVENTS (!BUILD_RELEASE)

// True if allow hardware tessellation shaders (Hull and Domain shaders)
#ifndef GPU_ALLOW_TESSELLATION_SHADERS
#define GPU_ALLOW_TESSELLATION_SHADERS 1
#endif

// True if allow geometry shaders
#ifndef GPU_ALLOW_GEOMETRY_SHADERS
#define GPU_ALLOW_GEOMETRY_SHADERS 1
#endif

// Enable/disable creating GPU resources on separate threads (otherwise only the main thread can be used)
#define GPU_ENABLE_ASYNC_RESOURCES_CREATION 1

// Enable/disable force shaders recompilation
#define GPU_FORCE_RECOMPILE_SHADERS 0

// Define default back buffer(s) format
#ifndef GPU_BACK_BUFFER_PIXEL_FORMAT
#define GPU_BACK_BUFFER_PIXEL_FORMAT PixelFormat::B8G8R8A8_UNorm
#endif

// Default depth buffer pixel format
#ifndef GPU_DEPTH_BUFFER_PIXEL_FORMAT
#define GPU_DEPTH_BUFFER_PIXEL_FORMAT PixelFormat::D32_Float
#endif

// Enable/disable gpu resources naming
#define GPU_ENABLE_RESOURCE_NAMING (!BUILD_RELEASE)

// True if use debug tools and flow for shaders
#define GPU_USE_SHADERS_DEBUG_LAYER (BUILD_DEBUG)

// Maximum size of the texture that is supported by the engine (specific platforms can have lower limit)
#define GPU_MAX_TEXTURE_SIZE 16384
#define GPU_MAX_TEXTURE_MIP_LEVELS 15
#define GPU_MAX_TEXTURE_ARRAY_SIZE 1024

// Validate configuration
#if !ENABLE_ASSERTION
#undef GPU_ENABLE_ASSERTION
#define GPU_ENABLE_ASSERTION 0
#endif

// Helper macro for defining shader structures wrappers in C++ that match HLSL constant buffers
#define GPU_CB_STRUCT(_declaration) ALIGN_BEGIN(GPU_SHADER_DATA_ALIGNMENT) PACK_BEGIN() struct _declaration PACK_END() ALIGN_END(GPU_SHADER_DATA_ALIGNMENT)
