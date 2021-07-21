// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "SceneAnimation.h"
#include "Engine/Core/Log.h"
#include "Engine/Content/Factories/BinaryAssetFactory.h"
#include "Engine/Content/Assets/MaterialBase.h"
#include "Engine/Content/Content.h"
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

const BytesContainer& SceneAnimation::LoadTimeline()
{
    WaitForLoaded();
    return _data;
}

#if USE_EDITOR

bool SceneAnimation::SaveTimeline(BytesContainer& data)
{
    // Wait for asset to be loaded or don't if last load failed (eg. by shader source compilation error)
    if (LastLoadFailed())
    {
        LOG(Warning, "Saving asset that failed to load.");
    }
    else if (WaitForLoaded())
    {
        LOG(Error, "Asset loading failed. Cannot save it.");
        return true;
    }

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

#endif

#if USE_EDITOR

void SceneAnimation::GetReferences(Array<Guid>& output) const
{
    // Base
    BinaryAsset::GetReferences(output);

    for (int32 i = 0; i < Tracks.Count(); i++)
    {
        const auto& track = Tracks[i];
        if (track.Asset)
            output.Add(track.Asset->GetID());
    }
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
    stream.ReadInt32(&version);
    switch (version)
    {
    case 2:
    case 3:
    {
        stream.ReadFloat(&FramesPerSecond);
        stream.ReadInt32(&DurationFrames);

        // Load tracks
        int32 tracksCount;
        stream.ReadInt32(&tracksCount);
        Tracks.Resize(tracksCount, false);
        for (int32 i = 0; i < tracksCount; i++)
        {
            auto& track = Tracks[i];

            track.Type = (Track::Types)stream.ReadByte();
            // [Deprecated on 13.07.2019 expires on 13.11.2019]
            if (version == 6184 || version == 6183)
                track.Flag = Track::Flags::None;
            else
                track.Flag = (Track::Flags)stream.ReadByte();
            stream.ReadInt32(&track.ParentIndex);
            stream.ReadInt32(&track.ChildrenCount);
            stream.ReadString(&track.Name, -13);
            stream.Read(&track.Color);
            track.Disabled = (int32)track.Flag & (int32)Track::Flags::Mute || (track.ParentIndex != -1 && Tracks[track.ParentIndex].Disabled);
            track.TrackStateIndex = -1;
            track.Data = nullptr;
            track.RuntimeData = nullptr;

            bool needsParent = false;
            switch (track.Type)
            {
            case Track::Types::Folder:
            {
                break;
            }
            case Track::Types::PostProcessMaterial:
            {
                const auto trackData = stream.Read<PostProcessMaterialTrack::Data>();
                track.Data = trackData;
                track.Asset = Content::LoadAsync<MaterialBase>(trackData->AssetID);
                if (trackData->AssetID.IsValid() && !track.Asset)
                {
                    LOG(Warning, "Missing material for track {0} in {1}.", track.Name, ToString());
                    track.Disabled = true;
                }
                break;
            }
            case Track::Types::NestedSceneAnimation:
            {
                const auto trackData = stream.Read<NestedSceneAnimationTrack::Data>();
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
                const auto trackData = stream.Read<ScreenFadeTrack::Data>();
                track.Data = trackData;
                if (trackData->GradientStopsCount < 0)
                {
                    LOG(Warning, "Negative amount of gradient stops.");
                    return LoadResult::InvalidData;
                }
                const auto trackRuntime = _runtimeData.Move<ScreenFadeTrack::Runtime>();
                track.RuntimeData = trackRuntime;
                trackRuntime->GradientStops = stream.Read<ScreenFadeTrack::GradientStop>(trackData->GradientStopsCount);
                break;
            }
            case Track::Types::Audio:
            {
                const auto trackData = stream.Read<AudioTrack::Data>();
                track.Data = trackData;
                track.Asset = Content::LoadAsync<AudioClip>(trackData->AssetID);
                const auto trackRuntime = _runtimeData.Move<AudioTrack::Runtime>();
                track.RuntimeData = trackRuntime;
                track.TrackStateIndex = TrackStatesCount++;
                trackRuntime->VolumeTrackIndex = -1;
                break;
            }
            case Track::Types::AudioVolume:
            {
                const auto trackData = stream.Read<AudioVolumeTrack::Data>();
                track.Data = trackData;
                const auto trackRuntime = _runtimeData.Move<AudioVolumeTrack::Runtime>();
                track.RuntimeData = trackRuntime;
                trackRuntime->KeyframesCount = trackData->KeyframesCount;
                trackRuntime->Keyframes = stream.Read<BezierCurveKeyframe<float>>(trackData->KeyframesCount);
                if (track.ParentIndex != -1)
                {
                    if (Tracks[track.ParentIndex].Type == Track::Types::Audio)
                    {
                        ((AudioTrack::Runtime*)((byte*)_runtimeData.GetHandle() + (intptr)Tracks[track.ParentIndex].RuntimeData))->VolumeTrackIndex = i;
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
                const auto trackData = stream.Read<ActorTrack::Data>();
                track.Data = trackData;
                const auto trackRuntime = _runtimeData.Move<ActorTrack::Runtime>();
                track.RuntimeData = trackRuntime;
                track.TrackStateIndex = TrackStatesCount++;
                break;
            }
            case Track::Types::Script:
            {
                const auto trackData = stream.Read<ScriptTrack::Data>();
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
                const auto trackData = stream.Read<KeyframesPropertyTrack::Data>();
                track.Data = trackData;
                const auto trackRuntime = _runtimeData.Move<KeyframesPropertyTrack::Runtime>();
                track.RuntimeData = trackRuntime;
                track.TrackStateIndex = TrackStatesCount++;
                trackRuntime->PropertyName = stream.Read<char>(trackData->PropertyNameLength + 1);
                trackRuntime->PropertyTypeName = stream.Read<char>(trackData->PropertyTypeNameLength + 1);
                const int32 keyframesDataSize = trackData->KeyframesCount * (sizeof(float) + trackData->ValueSize);
                trackRuntime->ValueSize = trackData->ValueSize;
                trackRuntime->KeyframesCount = trackData->KeyframesCount;
                trackRuntime->Keyframes = stream.Read(keyframesDataSize);
                needsParent = true;
                break;
            }
            case Track::Types::CurveProperty:
            {
                const auto trackData = stream.Read<CurvePropertyTrack::Data>();
                track.Data = trackData;
                const auto trackRuntime = _runtimeData.Move<CurvePropertyTrack::Runtime>();
                track.RuntimeData = trackRuntime;
                track.TrackStateIndex = TrackStatesCount++;
                trackRuntime->PropertyName = stream.Read<char>(trackData->PropertyNameLength + 1);
                trackRuntime->PropertyTypeName = stream.Read<char>(trackData->PropertyTypeNameLength + 1);
                const int32 keyframesDataSize = trackData->KeyframesCount * (sizeof(float) + trackData->ValueSize * 3);
                trackRuntime->ValueSize = trackData->ValueSize;
                trackRuntime->KeyframesCount = trackData->KeyframesCount;
                trackRuntime->Keyframes = stream.Read(keyframesDataSize);
                trackRuntime->DataType = CurvePropertyTrack::DataTypes::Unknown;
                if (StringUtils::Compare(trackRuntime->PropertyTypeName, "System.Single") == 0)
                    trackRuntime->DataType = CurvePropertyTrack::DataTypes::Float;
                else if (StringUtils::Compare(trackRuntime->PropertyTypeName, "System.Double") == 0)
                    trackRuntime->DataType = CurvePropertyTrack::DataTypes::Double;
                else if (StringUtils::Compare(trackRuntime->PropertyTypeName, "FlaxEngine.Vector2") == 0)
                    trackRuntime->DataType = CurvePropertyTrack::DataTypes::Vector2;
                else if (StringUtils::Compare(trackRuntime->PropertyTypeName, "FlaxEngine.Vector3") == 0)
                    trackRuntime->DataType = CurvePropertyTrack::DataTypes::Vector3;
                else if (StringUtils::Compare(trackRuntime->PropertyTypeName, "FlaxEngine.Vector4") == 0)
                    trackRuntime->DataType = CurvePropertyTrack::DataTypes::Vector4;
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
                needsParent = true;
                break;
            }
            case Track::Types::StringProperty:
            {
                const auto trackData = stream.Read<StringPropertyTrack::Data>();
                track.Data = trackData;
                const auto trackRuntime = (StringPropertyTrack::Runtime*)_runtimeData.Move(sizeof(StringPropertyTrack::Runtime) + sizeof(void*) * 2 * trackData->KeyframesCount);
                track.RuntimeData = trackRuntime;
                track.TrackStateIndex = TrackStatesCount++;
                trackRuntime->PropertyName = stream.Read<char>(trackData->PropertyNameLength + 1);
                trackRuntime->PropertyTypeName = stream.Read<char>(trackData->PropertyTypeNameLength + 1);
                trackRuntime->ValueSize = trackData->ValueSize;
                trackRuntime->KeyframesCount = trackData->KeyframesCount;
                const auto keyframesTimes = (float*)((byte*)trackRuntime + sizeof(StringPropertyTrack::Runtime));
                const auto keyframesLengths = (int32*)((byte*)keyframesTimes + sizeof(float) * trackData->KeyframesCount);
                const auto keyframesValues = (Char**)((byte*)keyframesLengths + sizeof(int32) * trackData->KeyframesCount);
                for (int32 j = 0; j < trackData->KeyframesCount; j++)
                {
                    stream.ReadFloat(&keyframesTimes[j]);
                    stream.ReadInt32(&keyframesLengths[j]);
                    keyframesValues[j] = stream.Read<Char>(keyframesLengths[j]);
                }
                needsParent = true;
                break;
            }
            case Track::Types::StructProperty:
            case Track::Types::ObjectProperty:
            {
                const auto trackData = stream.Read<StructPropertyTrack::Data>();
                track.Data = trackData;
                const auto trackRuntime = _runtimeData.Move<StructPropertyTrack::Runtime>();
                track.RuntimeData = trackRuntime;
                track.TrackStateIndex = TrackStatesCount++;
                trackRuntime->PropertyName = stream.Read<char>(trackData->PropertyNameLength + 1);
                trackRuntime->PropertyTypeName = stream.Read<char>(trackData->PropertyTypeNameLength + 1);
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
                trackRuntime->EventName = stream.Read<char>(tmp + 1);
                trackRuntime->EventParamsSize = 0;
                for (int j = 0; j < trackRuntime->EventParamsCount; j++)
                {
                    stream.ReadInt32(&trackRuntime->EventParamSizes[j]);
                    trackRuntime->EventParamsSize += trackRuntime->EventParamSizes[j];
                    stream.ReadInt32(&tmp);
                    trackRuntime->EventParamTypes[j] = stream.Read<char>(tmp + 1);
                }
                trackRuntime->DataBegin = stream.Read<byte>(trackRuntime->EventsCount * (sizeof(float) + trackRuntime->EventParamsSize));
                needsParent = true;
                break;
            }
            case Track::Types::CameraCut:
            {
                const auto trackData = stream.Read<CameraCutTrack::Data>();
                track.Data = trackData;
                const auto trackRuntime = _runtimeData.Move<CameraCutTrack::Runtime>();
                track.RuntimeData = trackRuntime;
                track.TrackStateIndex = TrackStatesCount++;
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
