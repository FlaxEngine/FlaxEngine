// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

// Maximum amount of binded render targets at the same time
#define GPU_MAX_RT_BINDED 6

// Maximum amount of binded shader resources at the same time
#define GPU_MAX_SR_BINDED 32

// Maximum amount of binded constant buffers at the same time
#define GPU_MAX_CB_BINDED 4

// Maximum amount of binded unordered access resources at the same time
#define GPU_MAX_UA_BINDED 2

// Maximum amount of binded texture sampler resources at the same time
#define GPU_MAX_SAMPLER_BINDED 16

// Amount of initial slots used for global samplers (static, 4 common samplers + 2 comparision samplers)
#define GPU_STATIC_SAMPLERS_COUNT 6

// Maximum amount of binded vertex buffers at the same time
#define GPU_MAX_VB_BINDED 4

// Maximum amount of thread groups per dimension for compute dispatch
#define GPU_MAX_CS_DISPATCH_THREAD_GROUPS 65535

// Enable/disable assertion for graphics layers
#define GPU_ENABLE_ASSERTION 1

// Enable/disable dynamic textures quality streaming
#define GPU_ENABLE_TEXTURES_STREAMING 1

// Enable/disable creating Shader Resource View for window backbuffer surface
#define GPU_USE_WINDOW_SRV 1

// True if allow graphics profile events and markers
#define GPU_ALLOW_PROFILE_EVENTS (!BUILD_RELEASE)

// Enable/disable creating GPU resources on separate threads (otherwise only the main thread can be used)
#define GPU_ENABLE_ASYNC_RESOURCES_CREATION 1

// Enable/disable force shaders recompilation
#define GPU_FORCE_RECOMPILE_SHADERS 0

// True if use BGRA back buffer format
#define GPU_USE_BGRA_BACK_BUFFER 1

// Default back buffer pixel format
#define GPU_DEPTH_BUFFER_PIXEL_FORMAT PixelFormat::D32_Float

// Enable/disable gpu resources naming
#define GPU_ENABLE_RESOURCE_NAMING (!BUILD_RELEASE)

// True if use debug tools and flow for shaders
#define GPU_USE_SHADERS_DEBUG_LAYER (BUILD_DEBUG)

// Maximum size of the texture that is supported by the engine (specific platforms can have lower limit)
#define GPU_MAX_TEXTURE_SIZE 8192
#define GPU_MAX_TEXTURE_MIP_LEVELS 14
#define GPU_MAX_TEXTURE_ARRAY_SIZE 512

// Define default back buffer(s) format
#if GPU_USE_BGRA_BACK_BUFFER
#define GPU_BACK_BUFFER_PIXEL_FORMAT PixelFormat::B8G8R8A8_UNorm
#else
#define GPU_BACK_BUFFER_PIXEL_FORMAT PixelFormat::R8G8B8A8_UNorm
#endif

// Validate configuration
#if !ENABLE_ASSERTION
#undef GPU_ENABLE_ASSERTION
#define GPU_ENABLE_ASSERTION 0
#endif
