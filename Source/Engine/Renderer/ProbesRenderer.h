// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Graphics/PixelFormat.h"
#include "Engine/Scripting/ScriptingObjectReference.h"
#include "Engine/Level/Actor.h"

// Amount of frames to wait for data from probe update job
#define PROBES_RENDERER_LATENCY_FRAMES 1

class EnvironmentProbe;
class SkyLight;
class RenderTask;

/// <summary>
/// Probes rendering service
/// </summary>
class ProbesRenderer
{
public:
    enum class EntryType
    {
        Invalid = 0,
        EnvProbe = 1,
        SkyLight = 2,
    };

    struct Entry
    {
        EntryType Type = EntryType::Invalid;
        ScriptingObjectReference<Actor> Actor;
        float Timeout = 0.0f;

        bool UseTextureData() const;
        int32 GetResolution() const;
        PixelFormat GetFormat() const;
    };

public:
    /// <summary>
    /// Minimum amount of time between two updated of probes
    /// </summary>
    static TimeSpan ProbesUpdatedBreak;

    /// <summary>
    ///  Time after last probe update when probes updating content will be released
    /// </summary>
    static TimeSpan ProbesReleaseDataTime;

    int32 GetBakeQueueSize();

    static Delegate<const Entry&> OnRegisterBake;

    static Delegate<const Entry&> OnFinishBake;

public:
    /// <summary>
    /// Checks if resources are ready to render probes (shaders or textures may be during loading).
    /// </summary>
    /// <returns>True if is ready, otherwise false.</returns>
    static bool HasReadyResources();

    /// <summary>
    /// Init probes content
    /// </summary>
    /// <returns>True if cannot init service</returns>
    static bool Init();

    /// <summary>
    /// Release probes content
    /// </summary>
    static void Release();

public:
    /// <summary>
    /// Register probe to baking service.
    /// </summary>
    /// <param name="probe">Probe to bake</param>
    /// <param name="timeout">Timeout in seconds left to bake it.</param>
    static void Bake(EnvironmentProbe* probe, float timeout = 0);

    /// <summary>
    /// Register probe to baking service.
    /// </summary>
    /// <param name="probe">Probe to bake</param>
    /// <param name="timeout">Timeout in seconds left to bake it.</param>
    static void Bake(SkyLight* probe, float timeout = 0);

private:
    static void OnRender(RenderTask* task, GPUContext* context);
};
