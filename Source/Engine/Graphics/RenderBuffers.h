// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/Viewport.h"
#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/Graphics/Textures/GPUTexture.h"

// Default motion vectors buffer pixel format (can fallback to other format if not supported)
#define MOTION_VECTORS_PIXEL_FORMAT PixelFormat::R16G16_Float

// GBuffer render targets formats
#define GBUFFER0_FORMAT PixelFormat::R8G8B8A8_UNorm
#define GBUFFER1_FORMAT PixelFormat::R10G10B10A2_UNorm
#define GBUFFER2_FORMAT PixelFormat::R8G8B8A8_UNorm
#define GBUFFER3_FORMAT PixelFormat::R8G8B8A8_UNorm

/// <summary>
/// The scene rendering buffers container.
/// </summary>
API_CLASS() class FLAXENGINE_API RenderBuffers : public PersistentScriptingObject
{
DECLARE_SCRIPTING_TYPE(RenderBuffers);
protected:

    int32 _width = 0;
    int32 _height = 0;
    float _aspectRatio = 0.0f;
    Viewport _viewport;

    Array<GPUTexture*, FixedAllocation<32>> _resources;

public:

    union
    {
        struct
        {
            GPUTexture* GBuffer0;
            GPUTexture* GBuffer1;
            GPUTexture* GBuffer2;
            GPUTexture* GBuffer3;
        };

        GPUTexture* GBuffer[4];
    };

    GPUTexture* RT1_FloatRGB;
    GPUTexture* RT2_FloatRGB;

    // Helper target for the eye adaptation
    float LastEyeAdaptationTime = 0.0f;
    GPUTexture* LuminanceMap = nullptr;
    uint64 LastFrameLuminanceMap = 0;

    // Helper target for volumetric fog rendering (used for the temporal filter and apply via fog shader)
    GPUTexture* VolumetricFogHistory = nullptr;
    GPUTexture* VolumetricFog = nullptr;
    GPUTexture* LocalShadowedLightScattering = nullptr;
    uint64 LastFrameVolumetricFog = 0;

    struct VolumetricFogData
    {
        float MaxDistance;
    };

    VolumetricFogData VolumetricFogData;

    // Helper buffer with half-resolution depth buffer shared by effects (eg. SSR, Motion Blur). Valid only during frame rendering and on request (see RequestHalfResDepth).
    // Should be released if not used for a few frames.
    GPUTexture* HalfResDepth = nullptr;
    uint64 LastFrameHalfResDepth = 0;

    // Helper target for the temporal SSR.
    // Should be released if not used for a few frames.
    GPUTexture* TemporalSSR = nullptr;
    uint64 LastFrameTemporalSSR = 0;

    // Helper target for the temporal AA.
    // Should be released if not used for a few frames.
    GPUTexture* TemporalAA = nullptr;
    uint64 LastFrameTemporalAA = 0;

public:

    /// <summary>
    /// Finalizes an instance of the <see cref="RenderBuffers"/> class.
    /// </summary>
    ~RenderBuffers();

public:

    /// <summary>
    /// Prepares buffers for rendering a scene. Called before rendering so other parts can reuse calculated value.
    /// </summary>
    void Prepare();

    /// <summary>
    /// Requests the half-resolution depth to be prepared for the current frame.
    /// </summary>
    /// <param name="context">The context.</param>
    /// <returns>The half-res depth buffer.</returns>
    GPUTexture* RequestHalfResDepth(GPUContext* context);

public:

    /// <summary>
    /// Gets the buffers width (in pixels).
    /// </summary>
    API_PROPERTY() FORCE_INLINE int32 GetWidth() const
    {
        return _width;
    }

    /// <summary>
    /// Gets the buffers height (in pixels).
    /// </summary>
    API_PROPERTY() FORCE_INLINE int32 GetHeight() const
    {
        return _height;
    }

    /// <summary>
    /// Gets the buffers width and height (in pixels).
    /// </summary>
    API_PROPERTY() FORCE_INLINE Vector2 GetSize() const
    {
        return Vector2((float)_width, (float)_height);
    }

    /// <summary>
    /// Gets the buffers aspect ratio.
    /// </summary>
    API_PROPERTY() FORCE_INLINE float GetAspectRatio() const
    {
        return _aspectRatio;
    }

    /// <summary>
    /// Gets the buffers rendering viewport.
    /// </summary>
    API_PROPERTY() FORCE_INLINE Viewport GetViewport() const
    {
        return _viewport;
    }

    /// <summary>
    /// Gets the current GPU memory usage by all the buffers (in bytes).
    /// </summary>
    uint64 GetMemoryUsage() const;

    /// <summary>
    /// Gets the depth buffer render target allocated within this render buffers collection (read only).
    /// </summary>
    API_FIELD(ReadOnly) GPUTexture* DepthBuffer;

    /// <summary>
    /// Gets the motion vectors render target allocated within this render buffers collection (read only).
    /// </summary>
    /// <remarks>
    /// Texture ca be null or not initialized if motion blur is disabled or not yet rendered.
    /// </remarks>
    API_FIELD(ReadOnly) GPUTexture* MotionVectors;

public:

    /// <summary>
    /// Allocates the buffers.
    /// </summary>
    /// <param name="width">The surface width (in pixels).</param>
    /// <param name="height">The surface height (in pixels).</param>
    /// <returns>True if cannot allocate buffers, otherwise false.</returns>
    API_FUNCTION() bool Init(int32 width, int32 height);

    /// <summary>
    /// Release the buffers data.
    /// </summary>
    API_FUNCTION() void Release();

public:

    // [PersistentScriptingObject]
    String ToString() const override;
};
