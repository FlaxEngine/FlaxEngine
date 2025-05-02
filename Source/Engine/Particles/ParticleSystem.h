// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Content/BinaryAsset.h"
#include "Engine/Core/Math/Color32.h"
#include "Engine/Core/Types/Pair.h"
#include "ParticleEmitter.h"

/// <summary>
/// Particle system contains a composition of particle emitters and playback information.
/// </summary>
API_CLASS(NoSpawn) class FLAXENGINE_API ParticleSystem : public BinaryAsset
{
    DECLARE_BINARY_ASSET_HEADER(ParticleSystem, 1);

public:
    /// <summary>
    /// The particle system timeline track data.
    /// </summary>
    struct Track
    {
        enum class Types
        {
            Emitter = 0,
            Folder = 1,
        };

        enum class Flags
        {
            None = 0,
            Mute = 1,
        };

        /// <summary>
        /// The type of the track.
        /// </summary>
        Types Type;

        /// <summary>
        /// The flags of the track.
        /// </summary>
        Flags Flag;

        /// <summary>
        /// The parent track index or -1 for root tracks.
        /// </summary>
        int32 ParentIndex;

        /// <summary>
        /// The amount of child tracks (stored in the sequence after this track).
        /// </summary>
        int32 ChildrenCount;

        /// <summary>
        /// The name of the track.
        /// </summary>
        String Name;

        /// <summary>
        /// True if track is disabled, otherwise false (cached on load based on the flags and parent flags).
        /// </summary>
        bool Disabled;

        /// <summary>
        /// The track color.
        /// </summary>
        Color32 Color;

        union
        {
            struct
            {
                /// <summary>
                /// The index of the emitter (from particle system emitters collection).
                /// </summary>
                int32 Index;

                /// <summary>
                /// The start frame of the emitter play begin.
                /// </summary>
                int32 StartFrame;

                /// <summary>
                /// The total duration of the emitter playback in the timeline sequence frames amount.
                /// </summary>
                int32 DurationFrames;
            } AsEmitter;

            struct
            {
            } AsFolder;
        };

        Track()
        {
        }
    };

    typedef Pair<int32, Guid> EmitterParameterOverrideKey;

private:
#if !BUILD_RELEASE
    String _debugName;
#endif

public:
    /// <summary>
    /// The asset data version number. Used to sync the  data with the instances state. Incremented each time asset gets loaded.
    /// </summary>
    uint32 Version = 0;

    /// <summary>
    /// The frames amount per second of the timeline animation.
    /// </summary>
    API_FIELD(ReadOnly) float FramesPerSecond;

    /// <summary>
    /// The animation duration (in frames).
    /// </summary>
    API_FIELD(ReadOnly) int32 DurationFrames;

    /// <summary>
    /// The animation duration (in seconds).
    /// </summary>
    API_PROPERTY() float GetDuration() const
    {
        return static_cast<float>(DurationFrames) / FramesPerSecond;
    }

    /// <summary>
    /// The emitters used by this system.
    /// </summary>
    Array<AssetReference<ParticleEmitter>> Emitters;

    /// <summary>
    /// The overriden values for the emitters parameters. Key is pair of emitter index and parameter ID, value is the custom value.
    /// </summary>
    Dictionary<EmitterParameterOverrideKey, Variant> EmittersParametersOverrides;

    /// <summary>
    /// The tracks on the system timeline.
    /// </summary>
    Array<Track> Tracks;

public:
    /// <summary>
    /// Initializes the particle system that plays a single particles emitter. This can be used only for virtual assets.
    /// </summary>
    /// <param name="emitter">The emitter to playback.</param>
    /// <param name="duration">The timeline animation duration in seconds.</param>
    /// <param name="fps">The amount of frames per second of the timeline animation.</param>
    API_FUNCTION() void Init(ParticleEmitter* emitter, float duration, float fps = 60.0f);

    /// <summary>
    /// Loads the serialized timeline data.
    /// </summary>
    /// <returns>The output surface data, or empty if failed to load.</returns>
    API_FUNCTION() BytesContainer LoadTimeline() const;

#if USE_EDITOR
    /// <summary>
    /// Saves the serialized timeline data to the asset.
    /// </summary>
    /// <param name="data">The timeline data container.</param>
    /// <returns><c>true</c> failed to save data; otherwise, <c>false</c>.</returns>
    API_FUNCTION() bool SaveTimeline(const BytesContainer& data) const;
#endif

public:
    /// <summary>
    /// Spawns the particles at the given location.
    /// </summary>
    /// <param name="position">The spawn position.</param>
    /// <param name="autoDestroy">If set to <c>true</c> effect be auto-destroyed after duration.</param>
    /// <returns>The spawned effect.</returns>
    API_FUNCTION() ParticleEffect* Spawn(const Vector3& position, bool autoDestroy = false)
    {
        return Spawn(nullptr, Transform(position), autoDestroy);
    }

    /// <summary>
    /// Spawns the particles at the given location.
    /// </summary>
    /// <param name="position">The spawn position.</param>
    /// <param name="rotation">The spawn rotation.</param>
    /// <param name="autoDestroy">If set to <c>true</c> effect be auto-destroyed after duration.</param>
    /// <returns>The spawned effect.</returns>
    API_FUNCTION() ParticleEffect* Spawn(const Vector3& position, const Quaternion& rotation, bool autoDestroy = false)
    {
        return Spawn(nullptr, Transform(position, rotation), autoDestroy);
    }

    /// <summary>
    /// Spawns the particles at the given location.
    /// </summary>
    /// <param name="transform">The spawn transform.</param>
    /// <param name="autoDestroy">If set to <c>true</c> effect be auto-destroyed after duration.</param>
    /// <returns>The spawned effect.</returns>
    API_FUNCTION() ParticleEffect* Spawn(const Transform& transform, bool autoDestroy = false)
    {
        return Spawn(nullptr, transform, autoDestroy);
    }

    /// <summary>
    /// Spawns the particles at the given location.
    /// </summary>
    /// <param name="parent">The parent actor (can be null to link it to the first loaded scene).</param>
    /// <param name="position">The spawn position.</param>
    /// <param name="autoDestroy">If set to <c>true</c> effect be auto-destroyed after duration.</param>
    /// <returns>The spawned effect.</returns>
    API_FUNCTION() ParticleEffect* Spawn(Actor* parent, const Vector3& position, bool autoDestroy = false)
    {
        return Spawn(parent, Transform(position), autoDestroy);
    }

    /// <summary>
    /// Spawns the particles at the given location.
    /// </summary>
    /// <param name="parent">The parent actor (can be null to link it to the first loaded scene).</param>
    /// <param name="position">The spawn position.</param>
    /// <param name="rotation">The spawn rotation.</param>
    /// <param name="autoDestroy">If set to <c>true</c> effect be auto-destroyed after duration.</param>
    /// <returns>The spawned effect.</returns>
    API_FUNCTION() ParticleEffect* Spawn(Actor* parent, Vector3 position, Quaternion rotation, bool autoDestroy = false)
    {
        return Spawn(parent, Transform(position, rotation), autoDestroy);
    }

    /// <summary>
    /// Spawns the particles at the given location.
    /// </summary>
    /// <param name="parent">The parent actor (can be null to link it to the first loaded scene).</param>
    /// <param name="transform">The spawn transform.</param>
    /// <param name="autoDestroy">If set to <c>true</c> effect be auto-destroyed after duration.</param>
    /// <returns>The spawned effect.</returns>
    API_FUNCTION() ParticleEffect* Spawn(Actor* parent, const Transform& transform, bool autoDestroy = false);

public:
    // [BinaryAsset]
    void InitAsVirtual() override;
#if USE_EDITOR
    void GetReferences(Array<Guid>& assets, Array<String>& files) const override;
    bool Save(const StringView& path = StringView::Empty) override;
#endif

protected:
    // [ParticleSystemBase]
    LoadResult load() override;
    void unload(bool isReloading) override;
    AssetChunksFlag getChunksToPreload() const override;
};
