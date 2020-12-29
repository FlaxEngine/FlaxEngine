// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Content/BinaryAsset.h"
#include "Engine/Content/AssetReference.h"
#include "Engine/Core/Math/Color32.h"
#include "Engine/Animations/Curve.h"
#include "Engine/Serialization/MemoryWriteStream.h"

/// <summary>
/// Scene animation timeline for animating objects and playing cut-scenes.
/// </summary>
API_CLASS(NoSpawn) class FLAXENGINE_API SceneAnimation final : public BinaryAsset
{
DECLARE_BINARY_ASSET_HEADER(SceneAnimation, 1);
public:

    /// <summary>
    /// The animation timeline track data.
    /// </summary>
    struct Track
    {
        enum class Types
        {
            //Emitter = 0,
            Folder = 1,
            PostProcessMaterial = 2,
            NestedSceneAnimation = 3,
            ScreenFade = 4,
            Audio = 5,
            AudioVolume = 6,
            Actor = 7,
            Script = 8,
            KeyframesProperty = 9,
            CurveProperty = 10,
            StringProperty = 11,
            ObjectReferenceProperty = 12,
            StructProperty = 13,
            ObjectProperty = 14,
            Event = 15,
            CameraCut = 16,
            //AnimationChannel = 17,
            //AnimationChannelData = 18,
        };

        enum class Flags
        {
            None = 0,
            Mute = 1,
            Loop = 2,
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

        /// <summary>
        /// The referenced asset.
        /// </summary>
        AssetReference<Asset> Asset;

        /// <summary>
        /// The track state index.
        /// </summary>
        int32 TrackStateIndex;

        /// <summary>
        /// The track data (from the asset storage).
        /// </summary>
        void* Data;

        /// <summary>
        /// The track dependent data (from the shared runtime allocation).
        /// </summary>
        void* RuntimeData;

        Track()
        {
        }

        template<typename T>
        FORCE_INLINE const T* GetData() const
        {
            return static_cast<T*>(Data);
        }

        template<typename T>
        FORCE_INLINE T* GetRuntimeData()
        {
            return static_cast<T*>(RuntimeData);
        }

        template<typename T>
        FORCE_INLINE const T* GetRuntimeData() const
        {
            return static_cast<T*>(RuntimeData);
        }
    };

    struct PostProcessMaterialTrack
    {
        struct Data
        {
            /// <summary>
            /// The PostFx material asset ID.
            /// </summary>
            Guid AssetID;

            /// <summary>
            /// The start frame of the media play begin.
            /// </summary>
            int32 StartFrame;

            /// <summary>
            /// The total duration of the media playback in the timeline sequence frames amount.
            /// </summary>
            int32 DurationFrames;
        };
    };

    struct NestedSceneAnimationTrack
    {
        struct Data
        {
            /// <summary>
            /// The scene animation asset ID.
            /// </summary>
            Guid AssetID;

            /// <summary>
            /// The start frame of the media play begin.
            /// </summary>
            int32 StartFrame;

            /// <summary>
            /// The total duration of the media playback in the timeline sequence frames amount.
            /// </summary>
            int32 DurationFrames;
        };

        struct Runtime
        {
        };
    };

    struct ScreenFadeTrack
    {
        struct GradientStop
        {
            int32 Frame;
            Color Value;
        };

        struct Data
        {
            /// <summary>
            /// The start frame of the media play begin.
            /// </summary>
            int32 StartFrame;

            /// <summary>
            /// The total duration of the media playback in the timeline sequence frames amount.
            /// </summary>
            int32 DurationFrames;

            /// <summary>
            /// The gradient stops count.
            /// </summary>
            int32 GradientStopsCount;
        };

        struct Runtime
        {
            /// <summary>
            /// The pointer to the gradient stops array (items count is GradientStopsCount).
            /// </summary>
            GradientStop* GradientStops;
        };
    };

    struct AudioTrack
    {
        struct Data
        {
            /// <summary>
            /// The audio clip asset ID.
            /// </summary>
            Guid AssetID;

            /// <summary>
            /// The start frame of the media play begin.
            /// </summary>
            int32 StartFrame;

            /// <summary>
            /// The total duration of the media playback in the timeline sequence frames amount.
            /// </summary>
            int32 DurationFrames;
        };

        struct Runtime
        {
            /// <summary>
            /// The index of the volume track. If not used then value is -1. Assigned on load.
            /// </summary>
            int32 VolumeTrackIndex;
        };
    };

    struct AudioVolumeTrack
    {
        typedef CurveBase<float, BezierCurveKeyframe<float>> CurveType;

        struct Data
        {
            /// <summary>
            /// The keyframes count.
            /// </summary>
            int32 KeyframesCount;
        };

        struct Runtime
        {
            /// <summary>
            /// The keyframes count.
            /// </summary>
            int32 KeyframesCount;

            /// <summary>
            /// The keyframes array (items count is KeyframesCount).
            /// </summary>
            BezierCurveKeyframe<float>* Keyframes;
        };
    };

    struct ObjectTrack
    {
        struct Data
        {
            /// <summary>
            /// The object ID.
            /// </summary>
            Guid ID;
        };

        struct Runtime
        {
        };
    };

    struct ActorTrack : ObjectTrack
    {
        struct Data : ObjectTrack::Data
        {
        };

        struct Runtime : ObjectTrack::Runtime
        {
        };
    };

    struct ScriptTrack : ObjectTrack
    {
        struct Data : ObjectTrack::Data
        {
        };

        struct Runtime : ObjectTrack::Runtime
        {
        };
    };

    struct PropertyTrack
    {
        struct Data
        {
            /// <summary>
            /// The property value data size (in bytes).
            /// </summary>
            int32 ValueSize;

            /// <summary>
            /// The name length.
            /// </summary>
            int32 PropertyNameLength;

            /// <summary>
            /// The typename length.
            /// </summary>
            int32 PropertyTypeNameLength;
        };

        struct Runtime
        {
            /// <summary>
            /// The property value data size (in bytes).
            /// </summary>
            int32 ValueSize;

            /// <summary>
            /// The name of the property (just a member name).
            /// </summary>
            char* PropertyName;

            /// <summary>
            /// The typename of the property value (fullname including namespace but not assembly).
            /// </summary>
            char* PropertyTypeName;
        };
    };

    struct KeyframesPropertyTrack : PropertyTrack
    {
        struct Data : PropertyTrack::Data
        {
            /// <summary>
            /// The keyframes count.
            /// </summary>
            int32 KeyframesCount;
        };

        struct Runtime : PropertyTrack::Runtime
        {
            /// <summary>
            /// The keyframes count.
            /// </summary>
            int32 KeyframesCount;

            /// <summary>
            /// The keyframes array (items count is KeyframesCount). Each keyframe is represented by pair of time (of type float) and the value data (of size ValueSize).
            /// </summary>
            void* Keyframes;
        };
    };

    struct CurvePropertyTrack : PropertyTrack
    {
        enum class DataTypes
        {
            Unknown,
            Float,
            Double,
            Vector2,
            Vector3,
            Vector4,
            Quaternion,
            Color,
            Color32,
        };

        struct Data : PropertyTrack::Data
        {
            /// <summary>
            /// The keyframes count.
            /// </summary>
            int32 KeyframesCount;
        };

        struct Runtime : PropertyTrack::Runtime
        {
            /// <summary>
            /// The cached curve data type.
            /// </summary>
            DataTypes DataType;

            /// <summary>
            /// The keyframes count.
            /// </summary>
            int32 KeyframesCount;

            /// <summary>
            /// The keyframes array (items count is KeyframesCount). Each keyframe is represented by: time (of type float), value data (of size ValueSize) and two values for bezier tangents.
            /// </summary>
            void* Keyframes;
        };
    };

    struct StringPropertyTrack : PropertyTrack
    {
        struct Data : PropertyTrack::Data
        {
            /// <summary>
            /// The keyframes count.
            /// </summary>
            int32 KeyframesCount;
        };

        struct Runtime : PropertyTrack::Runtime
        {
            /// <summary>
            /// The keyframes count.
            /// </summary>
            int32 KeyframesCount;

            // ..followed by the keyframes times and the values arrays (separate)
        };
    };

    typedef KeyframesPropertyTrack ObjectReferencePropertyTrack;
    typedef PropertyTrack StructPropertyTrack;
    typedef PropertyTrack ObjectPropertyTrack;

    struct EventTrack
    {
        enum
        {
            MaxParams = 8,
        };

        struct Data
        {
        };

        struct Runtime
        {
            /// <summary>
            /// The total amount of the event parameters.
            /// </summary>
            int32 EventParamsCount;

            /// <summary>
            /// The event invocations count.
            /// </summary>
            int32 EventsCount;

            /// <summary>
            /// The total size of the event parameters data in bytes).
            /// </summary>
            int32 EventParamsSize;

            /// <summary>
            /// The name of the event (just a member name).
            /// </summary>
            char* EventName;

            /// <summary>
            /// The name of the event parameter type (per parameter).
            /// </summary>
            char* EventParamTypes[MaxParams];

            /// <summary>
            /// The size (in bytes) of the event parameter type data (per parameter).
            /// </summary>
            int32 EventParamSizes[MaxParams];

            /// <summary>
            /// The events data begin.
            /// </summary>
            const byte* DataBegin;
        };
    };

    struct CameraCutTrack : ObjectTrack
    {
        struct Data : ObjectTrack::Data
        {
            /// <summary>
            /// The start frame of the media play begin.
            /// </summary>
            int32 StartFrame;

            /// <summary>
            /// The total duration of the media playback in the timeline sequence frames amount.
            /// </summary>
            int32 DurationFrames;
        };

        struct Runtime : ObjectTrack::Runtime
        {
        };
    };

private:

    BytesContainer _data;
    MemoryWriteStream _runtimeData;

public:

    /// <summary>
    /// The frames amount per second of the timeline animation.
    /// </summary>
    API_FIELD(ReadOnly) float FramesPerSecond;

    /// <summary>
    /// The animation duration (in frames).
    /// </summary>
    API_FIELD(ReadOnly) int32 DurationFrames;

    /// <summary>
    /// The tracks on the system timeline.
    /// </summary>
    Array<Track> Tracks;

    /// <summary>
    /// The amount of per-track state information required to allocate for this animation (including nested tracks).
    /// </summary>
    int32 TrackStatesCount;

public:

    /// <summary>
    /// Gets the animation duration (in seconds).
    /// </summary>
    /// <returns>The animation duration (in seconds).</returns>
    API_PROPERTY() FORCE_INLINE float GetDuration() const
    {
        return DurationFrames / FramesPerSecond;
    }

public:

    /// <summary>
    /// Gets the serialized timeline data.
    /// </summary>
    /// <returns>The output timeline data container. Empty if failed to load.</returns>
    API_FUNCTION() const BytesContainer& LoadTimeline();

#if USE_EDITOR

    /// <summary>
    /// Saves the serialized timeline data to the asset.
    /// </summary>
    /// <remarks>The cannot be used by virtual assets.</remarks>
    /// <param name="data">The timeline data container.</param>
    /// <returns><c>true</c> failed to save data; otherwise, <c>false</c>.</returns>
    API_FUNCTION() bool SaveTimeline(BytesContainer& data);

#endif

public:

    // [BinaryAsset]
#if USE_EDITOR
    void GetReferences(Array<Guid>& output) const override;
#endif

protected:

    // [SceneAnimationBase]
    LoadResult load() override;
    void unload(bool isReloading) override;
    AssetChunksFlag getChunksToPreload() const override;
};
