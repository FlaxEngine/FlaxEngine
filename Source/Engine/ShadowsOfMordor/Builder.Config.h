// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Enums.h"
#include "Engine/Graphics/PixelFormat.h"
#include "Engine/Graphics/Textures/TextureData.h"
#include "Types.h"

namespace ShadowsOfMordor
{
    DECLARE_ENUM_9(BuildProgressStep,
                   Initialize,
                   CacheEntries,
                   GenerateLightmapCharts,
                   PackLightmapCharts,
                   UpdateLightmapsCollection,
                   UpdateEntries,
                   GenerateHemispheresCache,
                   RenderHemispheres,
                   Cleanup);
    extern float BuildProgressStepProgress[BuildProgressStep_Count];
    extern PixelFormat HemispheresFormatToPixelFormat[2];
    const float LightmapTexelsPerWorldUnit = 1.0f / 4.0f;
    const int32 LightmapMinChartSize = 1;

    struct GenerateHemispheresData
    {
        TextureData PositionsData;
        TextureData NormalsData;
    };
};

#define HEMISPHERES_FORMAT_R32G32B32A32 0
#define HEMISPHERES_FORMAT_R16G16B16A16 1

// Adjustable configuration
#define LIGHTMAP_SCALE_MAX 1000000.0f
#define HEMISPHERES_RENDERING_TARGET_FPS 24
#define HEMISPHERES_PER_JOB_MIN 10
#define HEMISPHERES_PER_JOB_MAX 1000
#define HEMISPHERES_PER_GPU_FLUSH 15
#define HEMISPHERES_FOV 120.0f
#define HEMISPHERES_NEAR_PLANE 0.1f
#define HEMISPHERES_FAR_PLANE 10000.0f
#define HEMISPHERES_IRRADIANCE_FORMAT HEMISPHERES_FORMAT_R16G16B16A16
#define HEMISPHERES_BAKE_STATE_SAVE 1
#define HEMISPHERES_BAKE_STATE_SAVE_DELAY 300
#define CACHE_ENTRIES_PER_JOB 10
#define CACHE_POSITIONS_FORMAT HEMISPHERES_FORMAT_R32G32B32A32
#define CACHE_NORMALS_FORMAT HEMISPHERES_FORMAT_R16G16B16A16

// Debugging tools settings
// Note: debug images will be exported to the temporary folder ('<project-root>\Cache\ShadowsOfMordor_Debug')
#define DEBUG_EXPORT_LIGHTMAPS_PREVIEW 0
#define DEBUG_EXPORT_CACHE_PREVIEW 0
#define DEBUG_EXPORT_HEMISPHERES_PREVIEW 0

// Constants
#define HEMISPHERES_RESOLUTION 64
#define NUM_SH_TARGETS 3
