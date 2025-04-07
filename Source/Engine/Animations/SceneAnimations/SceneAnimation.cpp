// Copyright (c) Wojciech Figat. All rights reserved.

#include "SceneAnimation.h"
#include "Engine/Core/Log.h"
#include "Engine/Content/Factories/BinaryAssetFactory.h"
#include "Engine/Content/Assets/MaterialBase.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/Deprecated.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Audio/AudioClip.h"
#include "Engine/Graphics/PostProcessSettings.h"
#if USE_EDITOR
#include "Engine/Threading/Threading.h"
#endif

REGISTER_BINARY_ASSET(SceneAnimation, "FlaxEngine.SceneAnimation", false);

SceneAnimation::SceneAnimation(const SpawnParams& params, const AssetInfo* info)
    : BinaryAsset(params, info)
    , FramesPerSecond(1)
    , DurationFrames(0)
    , TrackStatesCount(0)
{
}

float SceneAnimation::GetDuration() const
{
    return (float)DurationFrames / FramesPerSecond;
}

const BytesContainer& SceneAnimation::LoadTimeline()
{
    WaitForLoaded();
    return _data;
}

#if USE_EDITOR

void SceneAnimation::SaveData(MemoryWriteStream& stream) const
{
    // Save properties
    stream.Write(4);
    stream.Write(FramesPerSecond);
    stream.Write(DurationFrames);

    // Save tracks
    stream.Write(Tracks.Count());
    for (const auto& track : Tracks)
    {
        stream.Write((byte)track.Type);
        stream.Write((byte)track.Flag);
        stream.Write((int32)track.ParentIndex);
        stream.Write((int32)track.ChildrenCount);
        stream.Write(track.Name, -13);
        stream.Write(track.Color);
        switch (track.Type)
        {
        case Track::Types::Folder:
            break;
        case Track::Types::PostProcessMaterial:
        {
            auto trackData = stream.Move<PostProcessMaterialTrack::Data>();
            trackData->AssetID = track.Asset.GetID();
            const auto trackRuntime = track.GetRuntimeData<PostProcessMaterialTrack::Runtime>();
            stream.Write((int32)trackRuntime->Count);
            stream.WriteBytes(trackRuntime->Media, sizeof(Media) * trackRuntime->Count);
            break;
        }
        case Track::Types::NestedSceneAnimation:
        {
            auto trackData = stream.Move<NestedSceneAnimationTrack::Data>();
            *trackData = *track.GetData<NestedSceneAnimationTrack::Data>();
            trackData->AssetID = track.Asset.GetID();
            break;
        }
        case Track::Types::ScreenFade:
        {
            auto trackData = stream.Move<ScreenFadeTrack::Data>();
            *trackData = *track.GetData<ScreenFadeTrack::Data>();
            const auto trackRuntime = track.GetRuntimeData<ScreenFadeTrack::Runtime>();
            stream.WriteBytes(trackRuntime->GradientStops, sizeof(ScreenFadeTrack::GradientStop) * trackData->GradientStopsCount);
            break;
        }
        case Track::Types::Audio:
        {
            auto trackData = stream.Move<AudioTrack::Data>();
            trackData->AssetID = track.Asset.GetID();
            const auto trackRuntime = track.GetRuntimeData<AudioTrack::Runtime>();
            stream.Write((int32)trackRuntime->Count);
            stream.WriteBytes(trackRuntime->Media, sizeof(AudioTrack::Media) * trackRuntime->Count);
            break;
        }
        case Track::Types::AudioVolume:
        {
            auto trackData = stream.Move<AudioVolumeTrack::Data>();
            *trackData = *track.GetData<AudioVolumeTrack::Data>();
            const auto trackRuntime = track.GetRuntimeData<AudioVolumeTrack::Runtime>();
            stream.WriteBytes(trackRuntime->Keyframes, sizeof(BezierCurveKeyframe<float>) * trackRuntime->KeyframesCount);
            break;
        }
        case Track::Types::Actor:
        {
            auto trackData = stream.Move<ActorTrack::Data>();
            *trackData = *track.GetData<ActorTrack::Data>();
            break;
        }
        case Track::Types::Script:
        {
            auto trackData = stream.Move<ScriptTrack::Data>();
            *trackData = *track.GetData<ScriptTrack::Data>();
            break;
        }
        case Track::Types::KeyframesProperty:
        case Track::Types::ObjectReferenceProperty:
        {
            auto trackData = stream.Move<KeyframesPropertyTrack::Data>();
            *trackData = *track.GetData<KeyframesPropertyTrack::Data>();
            const auto trackRuntime = track.GetRuntimeData<KeyframesPropertyTrack::Runtime>();
            stream.WriteBytes(trackRuntime->PropertyName, trackData->PropertyNameLength + 1);
            stream.WriteBytes(trackRuntime->PropertyTypeName, trackData->PropertyTypeNameLength + 1);
            stream.WriteBytes(trackRuntime->Keyframes, trackRuntime->KeyframesSize);
            break;
        }
        case Track::Types::CurveProperty:
        {
            auto trackData = stream.Move<CurvePropertyTrack::Data>();
            *trackData = *track.GetData<CurvePropertyTrack::Data>();
            const auto trackRuntime = track.GetRuntimeData<CurvePropertyTrack::Runtime>();
            stream.WriteBytes(trackRuntime->PropertyName, trackData->PropertyNameLength + 1);
            stream.WriteBytes(trackRuntime->PropertyTypeName, trackData->PropertyTypeNameLength + 1);
            const int32 keyframesDataSize = trackData->KeyframesCount * (sizeof(float) + trackData->ValueSize * 3);
            stream.WriteBytes(trackRuntime->Keyframes, keyframesDataSize);
            break;
        }
        case Track::Types::StringProperty:
        {
            const auto trackData = stream.Move<StringPropertyTrack::Data>();
            *trackData = *track.GetData<StringPropertyTrack::Data>();
            const auto trackRuntime = track.GetRuntimeData<StringPropertyTrack::Runtime>();
            stream.WriteBytes(trackRuntime->PropertyName, trackData->PropertyNameLength + 1);
            stream.WriteBytes(trackRuntime->PropertyTypeName, trackData->PropertyTypeNameLength + 1);
            const auto keyframesTimes = (float*)((byte*)trackRuntime + sizeof(StringPropertyTrack::Runtime));
            const auto keyframesLengths = (int32*)((byte*)keyframesTimes + sizeof(float) * trackRuntime->KeyframesCount);
            const auto keyframesValues = (Char**)((byte*)keyframesLengths + sizeof(int32) * trackRuntime->KeyframesCount);
            for (int32 j = 0; j < trackRuntime->KeyframesCount; j++)
            {
                stream.Write((float)keyframesTimes[j]);
                stream.Write((int32)keyframesLengths[j]);
                stream.WriteBytes(keyframesValues[j], keyframesLengths[j] * sizeof(Char));
            }
            break;
        }
        case Track::Types::StructProperty:
        case Track::Types::ObjectProperty:
        {
            auto trackData = stream.Move<StructPropertyTrack::Data>();
            *trackData = *track.GetData<StructPropertyTrack::Data>();
            const auto trackRuntime = track.GetRuntimeData<StructPropertyTrack::Runtime>();
            stream.WriteBytes(trackRuntime->PropertyName, trackData->PropertyNameLength + 1);
            stream.WriteBytes(trackRuntime->PropertyTypeName, trackData->PropertyTypeNameLength + 1);
            break;
        }
        case Track::Types::Event:
        {
            const auto trackRuntime = track.GetRuntimeData<EventTrack::Runtime>();
            int32 tmp = StringUtils::Length(trackRuntime->EventName);
            stream.Write((int32)trackRuntime->EventParamsCount);
            stream.Write((int32)trackRuntime->EventsCount);
            stream.Write((int32)tmp);
            stream.WriteBytes(trackRuntime->EventName, tmp + 1);
            for (int j = 0; j < trackRuntime->EventParamsCount; j++)
            {
                stream.Write((int32)trackRuntime->EventParamSizes[j]);
                stream.Write((int32)tmp);
                stream.WriteBytes(trackRuntime->EventParamTypes[j], tmp + 1);
            }
            stream.WriteBytes(trackRuntime->DataBegin, trackRuntime->EventsCount * (sizeof(float) + trackRuntime->EventParamsSize));
            break;
        }
        case Track::Types::CameraCut:
        {
            auto trackData = stream.Move<CameraCutTrack::Data>();
            *trackData = *track.GetData<CameraCutTrack::Data>();
            const auto trackRuntime = track.GetRuntimeData<CameraCutTrack::Runtime>();
            stream.Write((int32)trackRuntime->Count);
            stream.WriteBytes(trackRuntime->Media, sizeof(Media) * trackRuntime->Count);
            break;
        }
        default:
            break;
        }
    }
}

bool SceneAnimation::SaveTimeline(const BytesContainer& data) const
{
    if (OnCheckSave())
        return true;
    ScopeLock lock(Locker);

    // Release all chunks
    for (int32 i = 0; i < ASSET_FILE_DATA_CHUNKS; i++)
        ReleaseChunk(i);

    // Set timeline data
    auto chunk0 = GetOrCreateChunk(0);
    ASSERT(chunk0 != nullptr);
    chunk0->Data.Copy(data);

    // Save
    AssetInitData initData;
    initData.SerializedVersion = SerializedVersion;
    if (SaveAsset(initData))
    {
        LOG(Error, "Cannot save \'{0}\'", ToString());
        return true;
    }

    return false;
}

void SceneAnimation::GetReferences(Array<Guid>& assets, Array<String>& files) const
{
    // Base
    BinaryAsset::GetReferences(assets, files);

    for (int32 i = 0; i < Tracks.Count(); i++)
    {
        const auto& track = Tracks[i];
        if (track.Asset)
            assets.Add(track.Asset->GetID());
    }
}

bool SceneAnimation::Save(const StringView& path)
{
    if (OnCheckSave(path))
        return true;
    ScopeLock lock(Locker);
    MemoryWriteStream stream;
    SaveData(stream);
    BytesContainer data;
    data.Link(ToSpan(stream));
    return SaveTimeline(data);
}

#endif

Asset::LoadResult SceneAnimation::load()
{
    TrackStatesCount = 0;

    // Get the data chunk
    if (LoadChunk(0))
        return LoadResult::CannotLoadData;
    const auto chunk0 = GetChunk(0);
    if (chunk0 == nullptr || chunk0->IsMissing())
        return LoadResult::MissingDataChunk;
    _data.Swap(chunk0->Data);
    MemoryReadStream stream(_data.Get(), _data.Length());
    _runtimeData.SetPosition(0);

    // Tracks data documentation:
    // - load the whole timeline data from asset chunk 0
    // - use runtime data container (as memory stream) to reference the data and prepare it for usage at runtime
    // - data itself is read-only here but runtime data can be adjusted
    // - also read tracks data to create cached tracks array to iterate over

    // Load properties
    int32 version;
    stream.Read(version);
    switch (version)
    {
    case 2: // [Deprecated in 2020 expires on 03.09.2023]
    case 3: // [Deprecated on 03.09.2021 expires on 03.09.2023]
        MARK_CONTENT_DEPRECATED();
    case 4:
    {
        stream.Read(FramesPerSecond);
        stream.Read(DurationFrames);

        // Load tracks
        int32 tracksCount;
        stream.Read(tracksCount);
        Tracks.Resize(tracksCount, false);
        for (int32 i = 0; i < tracksCount; i++)
        {
            auto& track = Tracks[i];

            track.Type = (Track::Types)stream.ReadByte();
            track.Flag = (Track::Flags)stream.ReadByte();
            stream.ReadInt32(&track.ParentIndex);
            stream.ReadInt32(&track.ChildrenCount);
            stream.Read(track.Name, -13);
            stream.Read(track.Color);
            track.Disabled = (int32)track.Flag & (int32)Track::Flags::Mute || (track.ParentIndex != -1 && Tracks[track.ParentIndex].Disabled);
            track.TrackStateIndex = -1;
            track.Data = nullptr;
            track.RuntimeData = nullptr;

            bool needsParent = false;
            switch (track.Type)
            {
            case Track::Types::Folder:
                break;
            case Track::Types::PostProcessMaterial:
            {
                const auto trackData = stream.Move<PostProcessMaterialTrack::Data>();
                track.Data = trackData;
                track.Asset = Content::LoadAsync<MaterialBase>(trackData->AssetID);
                const auto trackRuntime = _runtimeData.Move<PostProcessMaterialTrack::Runtime>();
                track.RuntimeData = trackRuntime;
                if (version <= 3)
                {
                    // [Deprecated on 03.09.2021 expires on 03.09.2023]
                    trackRuntime->Count = 1;
                    trackRuntime->Media = stream.Move<Media>();
                }
                else
                {
                    stream.ReadInt32(&trackRuntime->Count);
                    trackRuntime->Media = stream.Move<Media>(trackRuntime->Count);
                }
                if (trackData->AssetID.IsValid() && !track.Asset)
                {
                    LOG(Warning, "Missing material for track {0} in {1}.", track.Name, ToString());
                    track.Disabled = true;
                }
                break;
            }
            case Track::Types::NestedSceneAnimation:
            {
                const auto trackData = stream.Move<NestedSceneAnimationTrack::Data>();
                track.Data = trackData;
                track.Asset = Content::LoadAsync<SceneAnimation>(trackData->AssetID);
                const auto trackRuntime = _runtimeData.Move<NestedSceneAnimationTrack::Runtime>();
                track.RuntimeData = trackRuntime;
                track.TrackStateIndex = TrackStatesCount;
                if (track.Asset)
                {
                    // Count the sub-tracks
                    track.Asset->WaitForLoaded();
                    TrackStatesCount += track.Asset.As<SceneAnimation>()->TrackStatesCount;
                }
                break;
            }
            case Track::Types::ScreenFade:
            {
                const auto trackData = stream.Move<ScreenFadeTrack::Data>();
                track.Data = trackData;
                if (trackData->GradientStopsCount < 0)
                {
                    LOG(Warning, "Negative amount of gradient stops.");
                    return LoadResult::InvalidData;
                }
                const auto trackRuntime = _runtimeData.Move<ScreenFadeTrack::Runtime>();
                track.RuntimeData = trackRuntime;
                trackRuntime->GradientStops = stream.Move<ScreenFadeTrack::GradientStop>(trackData->GradientStopsCount);
                break;
            }
            case Track::Types::Audio:
            {
                const auto trackData = stream.Move<AudioTrack::Data>();
                track.Data = trackData;
                track.Asset = Content::LoadAsync<AudioClip>(trackData->AssetID);
                const auto trackRuntime = _runtimeData.Move<AudioTrack::Runtime>();
                track.RuntimeData = trackRuntime;
                track.TrackStateIndex = TrackStatesCount++;
                trackRuntime->VolumeTrackIndex = -1;
                if (version <= 3)
                {
                    // [Deprecated on 03.09.2021 expires on 03.09.2023]
                    trackRuntime->Count = 1;
                    trackRuntime->Media = _runtimeData.Move<AudioTrack::Media>();
                    stream.ReadInt32(&trackRuntime->Media->StartFrame);
                    stream.ReadInt32(&trackRuntime->Media->DurationFrames);
                    trackRuntime->Media->Offset = 0.0f;
                }
                else
                {
                    stream.ReadInt32(&trackRuntime->Count);
                    trackRuntime->Media = stream.Move<AudioTrack::Media>(trackRuntime->Count);
                }
                break;
            }
            case Track::Types::AudioVolume:
            {
                const auto trackData = stream.Move<AudioVolumeTrack::Data>();
                track.Data = trackData;
                const auto trackRuntime = _runtimeData.Move<AudioVolumeTrack::Runtime>();
                track.RuntimeData = trackRuntime;
                trackRuntime->KeyframesCount = trackData->KeyframesCount;
                trackRuntime->Keyframes = stream.Move<BezierCurveKeyframe<float>>(trackData->KeyframesCount);
                if (track.ParentIndex != -1)
                {
                    if (Tracks[track.ParentIndex].Type == Track::Types::Audio)
                    {
                        ((AudioTrack::Runtime*)(_runtimeData.GetHandle() + (intptr)Tracks[track.ParentIndex].RuntimeData))->VolumeTrackIndex = i;
                    }
                    else
                    {
                        LOG(Warning, "Invalid type of the parent track for the track {1}, type {0}.", (int32)track.Type, track.Name);
                        track.Disabled = true;
                    }
                }
                break;
            }
            case Track::Types::Actor:
            {
                const auto trackData = stream.Move<ActorTrack::Data>();
                track.Data = trackData;
                const auto trackRuntime = _runtimeData.Move<ActorTrack::Runtime>();
                track.RuntimeData = trackRuntime;
                track.TrackStateIndex = TrackStatesCount++;
                break;
            }
            case Track::Types::Script:
            {
                const auto trackData = stream.Move<ScriptTrack::Data>();
                track.Data = trackData;
                const auto trackRuntime = _runtimeData.Move<ScriptTrack::Runtime>();
                track.RuntimeData = trackRuntime;
                track.TrackStateIndex = TrackStatesCount++;
                if (track.ParentIndex != -1 && Tracks[track.ParentIndex].Type != Track::Types::Actor && Tracks[track.ParentIndex].Type != Track::Types::CameraCut)
                {
                    LOG(Warning, "Invalid type of the parent track for the track {1}, type {0}.", (int32)track.Type, track.Name);
                    track.Disabled = true;
                }
                break;
            }
            case Track::Types::KeyframesProperty:
            case Track::Types::ObjectReferenceProperty:
            {
                const auto trackData = stream.Move<KeyframesPropertyTrack::Data>();
                track.Data = trackData;
                const auto trackRuntime = _runtimeData.Move<KeyframesPropertyTrack::Runtime>();
                track.RuntimeData = trackRuntime;
                track.TrackStateIndex = TrackStatesCount++;
                trackRuntime->PropertyName = stream.Move<char>(trackData->PropertyNameLength + 1);
                trackRuntime->PropertyTypeName = stream.Move<char>(trackData->PropertyTypeNameLength + 1);
                int32 keyframesDataSize = trackData->KeyframesCount * (sizeof(float) + trackData->ValueSize);
                if (trackData->ValueSize == 0)
                {
                    // When using json data (from non-POD types) read the sum of all keyframes data
                    const int32 keyframesDataStart = stream.GetPosition();
                    for (int32 j = 0; j < trackData->KeyframesCount; j++)
                    {
                        stream.Move<float>(); // Time
                        int32 jsonLen;
                        stream.ReadInt32(&jsonLen);
                        stream.Move(jsonLen);
                    }
                    const int32 keyframesDataEnd = stream.GetPosition();
                    stream.SetPosition(keyframesDataStart);
                    keyframesDataSize = keyframesDataEnd - keyframesDataStart;
                }
                trackRuntime->ValueSize = trackData->ValueSize;
                trackRuntime->KeyframesCount = trackData->KeyframesCount;
                trackRuntime->Keyframes = stream.Move(keyframesDataSize);
                trackRuntime->KeyframesSize = keyframesDataSize;
                needsParent = true;
                break;
            }
            case Track::Types::CurveProperty:
            {
                const auto trackData = stream.Move<CurvePropertyTrack::Data>();
                track.Data = trackData;
                const auto trackRuntime = _runtimeData.Move<CurvePropertyTrack::Runtime>();
                track.RuntimeData = trackRuntime;
                track.TrackStateIndex = TrackStatesCount++;
                trackRuntime->PropertyName = stream.Move<char>(trackData->PropertyNameLength + 1);
                trackRuntime->PropertyTypeName = stream.Move<char>(trackData->PropertyTypeNameLength + 1);
                const int32 keyframesDataSize = trackData->KeyframesCount * (sizeof(float) + trackData->ValueSize * 3);
                ASSERT(trackData->ValueSize > 0);
                trackRuntime->ValueSize = trackData->ValueSize;
                trackRuntime->KeyframesCount = trackData->KeyframesCount;
                trackRuntime->Keyframes = stream.Move(keyframesDataSize);
                trackRuntime->DataType = CurvePropertyTrack::DataTypes::Unknown;
                trackRuntime->ValueType = CurvePropertyTrack::DataTypes::Unknown;
                if (StringUtils::Compare(trackRuntime->PropertyTypeName, "System.Single") == 0)
                    trackRuntime->DataType = CurvePropertyTrack::DataTypes::Float;
                else if (StringUtils::Compare(trackRuntime->PropertyTypeName, "System.Double") == 0)
                    trackRuntime->DataType = CurvePropertyTrack::DataTypes::Double;
                else if (StringUtils::Compare(trackRuntime->PropertyTypeName, "FlaxEngine.Vector2") == 0)
                {
                    // Read Float or Double depending on serialized data but use value depending on the current build configuration
                    trackRuntime->DataType = trackData->ValueSize == sizeof(Float2) ? CurvePropertyTrack::DataTypes::Float2 : CurvePropertyTrack::DataTypes::Double2;
#if USE_LARGE_WORLDS
                    trackRuntime->ValueType = CurvePropertyTrack::DataTypes::Double2;
#else
                    trackRuntime->ValueType = CurvePropertyTrack::DataTypes::Float2;
#endif
                }
                else if (StringUtils::Compare(trackRuntime->PropertyTypeName, "FlaxEngine.Vector3") == 0)
                {
                    // Read Float or Double depending on serialized data but use value depending on the current build configuration
                    trackRuntime->DataType = trackData->ValueSize == sizeof(Float3) ? CurvePropertyTrack::DataTypes::Float3 : CurvePropertyTrack::DataTypes::Double3;
#if USE_LARGE_WORLDS
                    trackRuntime->ValueType = CurvePropertyTrack::DataTypes::Double3;
#else
                    trackRuntime->ValueType = CurvePropertyTrack::DataTypes::Float3;
#endif
                }
                else if (StringUtils::Compare(trackRuntime->PropertyTypeName, "FlaxEngine.Vector4") == 0)
                {
                    // Read Float or Double depending on serialized data but use value depending on the current build configuration
                    trackRuntime->DataType = trackData->ValueSize == sizeof(Float4) ? CurvePropertyTrack::DataTypes::Float4 : CurvePropertyTrack::DataTypes::Double4;
#if USE_LARGE_WORLDS
                    trackRuntime->ValueType = CurvePropertyTrack::DataTypes::Double4;
#else
                    trackRuntime->ValueType = CurvePropertyTrack::DataTypes::Float4;
#endif
                }
                else if (StringUtils::Compare(trackRuntime->PropertyTypeName, "FlaxEngine.Float2") == 0)
                    trackRuntime->DataType = CurvePropertyTrack::DataTypes::Float2;
                else if (StringUtils::Compare(trackRuntime->PropertyTypeName, "FlaxEngine.Float3") == 0)
                    trackRuntime->DataType = CurvePropertyTrack::DataTypes::Float3;
                else if (StringUtils::Compare(trackRuntime->PropertyTypeName, "FlaxEngine.Float4") == 0)
                    trackRuntime->DataType = CurvePropertyTrack::DataTypes::Float4;
                else if (StringUtils::Compare(trackRuntime->PropertyTypeName, "FlaxEngine.Double2") == 0)
                    trackRuntime->DataType = CurvePropertyTrack::DataTypes::Double2;
                else if (StringUtils::Compare(trackRuntime->PropertyTypeName, "FlaxEngine.Double3") == 0)
                    trackRuntime->DataType = CurvePropertyTrack::DataTypes::Double3;
                else if (StringUtils::Compare(trackRuntime->PropertyTypeName, "FlaxEngine.Double4") == 0)
                    trackRuntime->DataType = CurvePropertyTrack::DataTypes::Double4;
                else if (StringUtils::Compare(trackRuntime->PropertyTypeName, "FlaxEngine.Quaternion") == 0)
                    trackRuntime->DataType = CurvePropertyTrack::DataTypes::Quaternion;
                else if (StringUtils::Compare(trackRuntime->PropertyTypeName, "FlaxEngine.Color") == 0)
                    trackRuntime->DataType = CurvePropertyTrack::DataTypes::Color;
                else if (StringUtils::Compare(trackRuntime->PropertyTypeName, "FlaxEngine.Color32") == 0)
                    trackRuntime->DataType = CurvePropertyTrack::DataTypes::Color32;
                if (trackRuntime->DataType == CurvePropertyTrack::DataTypes::Unknown)
                {
                    LOG(Warning, "Unknown curve animation property type {2} for the track {1}, type {0}.", (int32)track.Type, track.Name, String(trackRuntime->PropertyTypeName));
                    track.Disabled = true;
                }
                if (trackRuntime->ValueType == CurvePropertyTrack::DataTypes::Unknown)
                    trackRuntime->ValueType = trackRuntime->DataType;
                needsParent = true;
                break;
            }
            case Track::Types::StringProperty:
            {
                const auto trackData = stream.Move<StringPropertyTrack::Data>();
                track.Data = trackData;
                const auto trackRuntime = (StringPropertyTrack::Runtime*)_runtimeData.Move(sizeof(StringPropertyTrack::Runtime) + sizeof(void*) * 2 * trackData->KeyframesCount);
                track.RuntimeData = trackRuntime;
                track.TrackStateIndex = TrackStatesCount++;
                trackRuntime->PropertyName = stream.Move<char>(trackData->PropertyNameLength + 1);
                trackRuntime->PropertyTypeName = stream.Move<char>(trackData->PropertyTypeNameLength + 1);
                trackRuntime->ValueSize = trackData->ValueSize;
                ASSERT(trackData->ValueSize > 0);
                trackRuntime->KeyframesCount = trackData->KeyframesCount;
                const auto keyframesTimes = (float*)((byte*)trackRuntime + sizeof(StringPropertyTrack::Runtime));
                const auto keyframesLengths = (int32*)((byte*)keyframesTimes + sizeof(float) * trackData->KeyframesCount);
                const auto keyframesValues = (Char**)((byte*)keyframesLengths + sizeof(int32) * trackData->KeyframesCount);
                for (int32 j = 0; j < trackData->KeyframesCount; j++)
                {
                    stream.ReadFloat(&keyframesTimes[j]);
                    stream.ReadInt32(&keyframesLengths[j]);
                    keyframesValues[j] = stream.Move<Char>(keyframesLengths[j]);
                }
                needsParent = true;
                break;
            }
            case Track::Types::StructProperty:
            case Track::Types::ObjectProperty:
            {
                const auto trackData = stream.Move<StructPropertyTrack::Data>();
                track.Data = trackData;
                const auto trackRuntime = _runtimeData.Move<StructPropertyTrack::Runtime>();
                track.RuntimeData = trackRuntime;
                track.TrackStateIndex = TrackStatesCount++;
                trackRuntime->PropertyName = stream.Move<char>(trackData->PropertyNameLength + 1);
                trackRuntime->PropertyTypeName = stream.Move<char>(trackData->PropertyTypeNameLength + 1);
                trackRuntime->ValueSize = trackData->ValueSize;
                needsParent = true;
                break;
            }
            case Track::Types::Event:
            {
                const auto trackRuntime = _runtimeData.Move<EventTrack::Runtime>();
                track.RuntimeData = trackRuntime;
                track.TrackStateIndex = TrackStatesCount++;
                int32 tmp;
                stream.ReadInt32(&trackRuntime->EventParamsCount);
                stream.ReadInt32(&trackRuntime->EventsCount);
                stream.ReadInt32(&tmp);
                trackRuntime->EventName = stream.Move<char>(tmp + 1);
                trackRuntime->EventParamsSize = 0;
                for (int j = 0; j < trackRuntime->EventParamsCount; j++)
                {
                    stream.ReadInt32(&trackRuntime->EventParamSizes[j]);
                    trackRuntime->EventParamsSize += trackRuntime->EventParamSizes[j];
                    stream.ReadInt32(&tmp);
                    trackRuntime->EventParamTypes[j] = stream.Move<char>(tmp + 1);
                }
                trackRuntime->DataBegin = stream.Move<byte>(trackRuntime->EventsCount * (sizeof(float) + trackRuntime->EventParamsSize));
                needsParent = true;
                break;
            }
            case Track::Types::CameraCut:
            {
                const auto trackData = stream.Move<CameraCutTrack::Data>();
                track.Data = trackData;
                const auto trackRuntime = _runtimeData.Move<CameraCutTrack::Runtime>();
                track.RuntimeData = trackRuntime;
                track.TrackStateIndex = TrackStatesCount++;
                if (version <= 3)
                {
                    // [Deprecated on 03.09.2021 expires on 03.09.2023]
                    trackRuntime->Count = 1;
                    trackRuntime->Media = stream.Move<Media>();
                }
                else
                {
                    stream.ReadInt32(&trackRuntime->Count);
                    trackRuntime->Media = stream.Move<Media>(trackRuntime->Count);
                }
                break;
            }
            default:
                LOG(Warning, "Unsupported scene animation track type.");
                track.Disabled = true;
                return LoadResult::InvalidData;
            }

            if (needsParent && track.ParentIndex == -1)
            {
                LOG(Warning, "Missing parent track for the track {1}, type {0}.", (int32)track.Type, track.Name);
                track.Disabled = true;
            }

            // Offset track runtime data to be offset from allocation start
            if (track.RuntimeData)
                track.RuntimeData = (void*)((intptr)track.RuntimeData - (intptr)_runtimeData.GetHandle());
        }
        break;
    }
    default:
        LOG(Warning, "Unknown timeline version {0}.", version);
        return LoadResult::InvalidData;
    }

    // Offset back the tracks runtime data to be offset from allocation start
    for (int32 i = 0; i < Tracks.Count(); i++)
    {
        Track& track = Tracks[i];
        track.RuntimeData = (void*)((intptr)track.RuntimeData + (intptr)_runtimeData.GetHandle());
    }

    // Wait for all referenced assets (scene animation cannot be used if  data is not loaded)
    // Note: this loop might trigger loading referenced assets on this thread
    for (int32 i = 0; i < Tracks.Count(); i++)
    {
        auto& track = Tracks[i];
        if (track.Asset)
            track.Asset->WaitForLoaded();
    }

    return LoadResult::Ok;
}

void SceneAnimation::unload(bool isReloading)
{
    FramesPerSecond = 0.0f;
    DurationFrames = 0;
    Tracks.Resize(0);
    _runtimeData = MemoryWriteStream();
    _data.Release();
}

AssetChunksFlag SceneAnimation::getChunksToPreload() const
{
    return GET_CHUNK_FLAG(0);
}
