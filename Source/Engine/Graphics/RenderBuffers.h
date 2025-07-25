// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/Viewport.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/Graphics/Textures/GPUTexture.h"

// GBuffer render targets formats
#define GBUFFER0_FORMAT PixelFormat::R8G8B8A8_UNorm
#define GBUFFER1_FORMAT PixelFormat::R10G10B10A2_UNorm
#define GBUFFER2_FORMAT PixelFormat::R8G8B8A8_UNorm
#define GBUFFER3_FORMAT PixelFormat::R8G8B8A8_UNorm

/// <summary>
/// The scene rendering buffers container.
/// </summary>
API_CLASS() class FLAXENGINE_API RenderBuffers : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE(RenderBuffers);

    /// <summary>
    /// The custom rendering state.
    /// </summary>
    class FLAXENGINE_API CustomBuffer : public Object
    {
    public:
        String Name;
        uint64 LastFrameUsed = 0;

        String ToString() const override;
    };

protected:
    int32 _width = 0;
    int32 _height = 0;
    float _aspectRatio = 0.0f;
    bool _useAlpha = false;
    Viewport _viewport;
    Array<GPUTexture*, FixedAllocation<32>> _resources;

public:
    union
    {
        struct
        {
            /// <summary>Gets the GBuffer texture 0. RGB: Color, A: AO</summary>
            API_FIELD(ReadOnly) GPUTexture* GBuffer0;
            /// <summary>Gets the GBuffer texture 1. RGB: Normal, A: ShadingModel</summary>
            API_FIELD(ReadOnly) GPUTexture* GBuffer1;
            /// <summary>Gets the GBuffer texture 2. R: Roughness, G: Metalness, B:Specular</summary>
            API_FIELD(ReadOnly) GPUTexture* GBuffer2;
            /// <summary>Gets the GBuffer texture 3. RGBA: Custom Data</summary>
            API_FIELD(ReadOnly) GPUTexture* GBuffer3;
        };

        GPUTexture* GBuffer[4];
    };

    // Helper target for the eye adaptation
    float LastEyeAdaptationTime = 0.0f;
    GPUTexture* LuminanceMap = nullptr;
    uint64 LastFrameLuminanceMap = 0;

    // Helper target for volumetric fog rendering (used for the temporal filter and apply via fog shader)
    GPUTexture* VolumetricFogHistory = nullptr;
    GPUTexture* VolumetricFog = nullptr;
    GPUTexture* LocalShadowedLightScattering = nullptr;
    uint64 LastFrameVolumetricFog = 0;

    struct
    {
        float MaxDistance;
    } VolumetricFogData;

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

    // Maps the custom buffer type into the object that holds the state.
    Array<CustomBuffer*, HeapAllocation> CustomBuffers;

public:
    /// <summary>
    /// Finalizes an instance of the <see cref="RenderBuffers"/> class.
    /// </summary>
    ~RenderBuffers();

public:
    /// <summary>
    /// Frees unused buffers to reduce memory usage for certain drawing effects that are state-dependant but unused for multiple frames.
    /// </summary>
    void ReleaseUnusedMemory();

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
    API_PROPERTY() FORCE_INLINE Float2 GetSize() const
    {
        return Float2((float)_width, (float)_height);
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
    /// Gets the output buffers format (R11G11B10 or R16G16B16A16 depending on UseAlpha property).
    /// </summary>
    API_PROPERTY() PixelFormat GetOutputFormat() const;

    /// <summary>
    /// True if support alpha output in the rendering buffers and pass-though alpha mask of the scene during rendering (at cost of reduced performance).
    /// </summary>
    API_PROPERTY() bool GetUseAlpha() const;

    /// <summary>
    /// True if support alpha output in the rendering buffers and pass-though alpha mask of the scene during rendering (at cost of reduced performance).
    /// </summary>
    API_PROPERTY() void SetUseAlpha(bool value);

    const CustomBuffer* FindCustomBuffer(const StringView& name, bool withLinked = true) const;

    template<class T>
    const T* FindCustomBuffer(const StringView& name, bool withLinked = true) const
    {
        return (const T*)FindCustomBuffer(name, withLinked);
    }

    template<class T>
    const T* FindLinkedBuffer(const StringView& name) const
    {
        return LinkedCustomBuffers ? (const T*)LinkedCustomBuffers->FindCustomBuffer(name, true) : nullptr;
    }

    template<class T>
    T* GetCustomBuffer(const StringView& name, bool withLinked = true)
    {
        if (LinkedCustomBuffers && withLinked)
            return LinkedCustomBuffers->GetCustomBuffer<T>(name, withLinked);
        CustomBuffer* result = (CustomBuffer*)FindCustomBuffer(name, withLinked);
        if (!result)
        {
            result = New<T>();
            result->Name = name;
            CustomBuffers.Add(result);
        }
        return (T*)result;
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

    /// <summary>
    /// External Render Buffers used to redirect FindCustomBuffer/GetCustomBuffer calls. Can be linked to other rendering task (eg. main game viewport) to reuse graphics effect state from it (eg. use GI from main game view in in-game camera renderer).
    /// </summary>
    API_FIELD() RenderBuffers* LinkedCustomBuffers = nullptr;

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
};
