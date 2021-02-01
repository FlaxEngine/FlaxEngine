// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "Animation.h"
#include "SkinnedModel.h"
#include "Engine/Core/Log.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Content/Factories/BinaryAssetFactory.h"
#include "Engine/Animations/CurveSerialization.h"
#include "Engine/Serialization/MemoryReadStream.h"
#if USE_EDITOR
#include "Engine/Serialization/MemoryWriteStream.h"
#endif

REGISTER_BINARY_ASSET(Animation, "FlaxEngine.Animation", nullptr, false);

Animation::Animation(const SpawnParams& params, const AssetInfo* info)
    : BinaryAsset(params, info)
{
}

Animation::InfoData Animation::GetInfo() const
{
    InfoData info;
    if (IsLoaded())
    {
        info.Length = Data.GetLength();
        info.FramesCount = (int32)Data.Duration;
        info.ChannelsCount = Data.Channels.Count();
        info.KeyframesCount = Data.GetKeyframesCount();
    }
    else
    {
        info.Length = 0.0f;
        info.FramesCount = 0;
        info.ChannelsCount = 0;
        info.KeyframesCount = 0;
    }
    return info;
}

void Animation::ClearCache()
{
    ScopeLock lock(Locker);

    // Unlink events
    for (auto i = MappingCache.Begin(); i.IsNotEnd(); ++i)
    {
        i->Key->OnUnloaded.Unbind<Animation, &Animation::OnSkinnedModelUnloaded>(this);
        i->Key->OnReloading.Unbind<Animation, &Animation::OnSkinnedModelUnloaded>(this);
    }

    // Free memory
    MappingCache.Clear();
    MappingCache.Cleanup();
}

const Animation::NodeToChannel* Animation::GetMapping(SkinnedModel* obj)
{
    ASSERT(obj && obj->IsLoaded() && IsLoaded());

    ScopeLock lock(Locker);

    // Try quick lookup
    NodeToChannel* result = MappingCache.TryGet(obj);
    if (result == nullptr)
    {
        PROFILE_CPU();

        // Add to cache
        NodeToChannel tmp;
        auto bucket = MappingCache.Add(obj, tmp);
        result = &bucket->Value;
        obj->OnUnloaded.Bind<Animation, &Animation::OnSkinnedModelUnloaded>(this);
        obj->OnReloading.Bind<Animation, &Animation::OnSkinnedModelUnloaded>(this);

        // Initialize the mapping
        const auto& skeleton = obj->Skeleton;
        const int32 nodesCount = skeleton.Nodes.Count();
        result->Resize(nodesCount, false);
        result->SetAll(-1);
        for (int32 i = 0; i < Data.Channels.Count(); i++)
        {
            auto& nodeAnim = Data.Channels[i];

            for (int32 j = 0; j < nodesCount; j++)
            {
                if (skeleton.Nodes[j].Name == nodeAnim.NodeName)
                {
                    result->At(j) = i;
                    break;
                }
            }
        }
    }

    return result;
}

#if USE_EDITOR

void Animation::LoadTimeline(BytesContainer& result) const
{
    result.Release();
    if (!IsLoaded())
        return;
    MemoryWriteStream stream(4096);

    // Version
    stream.WriteInt32(3);

    // Meta
    float fps = (float)Data.FramesPerSecond;
    const float fpsInv = 1.0f / fps;
    stream.WriteFloat(fps);
    stream.WriteInt32((int32)Data.Duration);
    int32 tracksCount = Data.Channels.Count();
    for (auto& channel : Data.Channels)
        tracksCount +=
                (channel.Position.GetKeyframes().HasItems() ? 1 : 0) +
                (channel.Rotation.GetKeyframes().HasItems() ? 1 : 0) +
                (channel.Scale.GetKeyframes().HasItems() ? 1 : 0);
    stream.WriteInt32(tracksCount);

    // Tracks
    int32 trackIndex = 0;
    for (int32 i = 0; i < Data.Channels.Count(); i++)
    {
        auto& channel = Data.Channels[i];
        const int32 childrenCount =
                (channel.Position.GetKeyframes().HasItems() ? 1 : 0) +
                (channel.Rotation.GetKeyframes().HasItems() ? 1 : 0) +
                (channel.Scale.GetKeyframes().HasItems() ? 1 : 0);

        // Animation Channel track
        stream.WriteByte(17); // Track Type
        stream.WriteByte(0); // Track Flags
        stream.WriteInt32(-1); // Parent Index
        stream.WriteInt32(childrenCount); // Children Count
        stream.WriteString(channel.NodeName, -13); // Name
        stream.Write(&Color32::White); // Color
        const int32 parentIndex = trackIndex++;

        auto& position = channel.Position.GetKeyframes();
        if (position.HasItems())
        {
            // Animation Channel Data track (position)
            stream.WriteByte(18); // Track Type
            stream.WriteByte(0); // Track Flags
            stream.WriteInt32(parentIndex); // Parent Index
            stream.WriteInt32(0); // Children Count
            stream.WriteString(String::Format(TEXT("Track_{0}_Position"), i), -13); // Name
            stream.Write(&Color32::White); // Color
            stream.WriteByte(0); // Type
            stream.WriteInt32(position.Count()); // Keyframes Count
            for (auto& k : position)
            {
                stream.WriteFloat(k.Time * fpsInv);
                stream.Write(&k.Value);
            }
            trackIndex++;
        }

        auto& rotation = channel.Rotation.GetKeyframes();
        if (rotation.HasItems())
        {
            // Animation Channel Data track (rotation)
            stream.WriteByte(18); // Track Type
            stream.WriteByte(0); // Track Flags
            stream.WriteInt32(parentIndex); // Parent Index
            stream.WriteInt32(0); // Children Count
            stream.WriteString(String::Format(TEXT("Track_{0}_Rotation"), i), -13); // Name
            stream.Write(&Color32::White); // Color
            stream.WriteByte(1); // Type
            stream.WriteInt32(rotation.Count()); // Keyframes Count
            for (auto& k : rotation)
            {
                stream.WriteFloat(k.Time * fpsInv);
                stream.Write(&k.Value);
            }
            trackIndex++;
        }

        auto& scale = channel.Scale.GetKeyframes();
        if (scale.HasItems())
        {
            // Animation Channel Data track (scale)
            stream.WriteByte(18); // Track Type
            stream.WriteByte(0); // Track Flags
            stream.WriteInt32(parentIndex); // Parent Index
            stream.WriteInt32(0); // Children Count
            stream.WriteString(String::Format(TEXT("Track_{0}_Scale"), i), -13); // Name
            stream.Write(&Color32::White); // Color
            stream.WriteByte(2); // Type
            stream.WriteInt32(scale.Count()); // Keyframes Count
            for (auto& k : scale)
            {
                stream.WriteFloat(k.Time * fpsInv);
                stream.Write(&k.Value);
            }
            trackIndex++;
        }
    }

    result.Copy(stream.GetHandle(), stream.GetPosition());
}

bool Animation::SaveTimeline(BytesContainer& data)
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

    MemoryReadStream stream(data.Get(), data.Length());

    // Version
    int32 version;
    stream.ReadInt32(&version);
    if (version != 3)
    {
        LOG(Error, "Unknown timeline data version {0}.", version);
        return true;
    }

    // Meta
    float fps;
    stream.ReadFloat(&fps);
    Data.FramesPerSecond = static_cast<double>(fps);
    int32 duration;
    stream.ReadInt32(&duration);
    Data.Duration = static_cast<double>(duration);
    int32 tracksCount;
    stream.ReadInt32(&tracksCount);

    // Tracks
    Data.Channels.Clear();
    Dictionary<int32, int32> animationChannelTrackIndexToChannelIndex;
    animationChannelTrackIndexToChannelIndex.EnsureCapacity(tracksCount * 3);
    for (int32 trackIndex = 0; trackIndex < tracksCount; trackIndex++)
    {
        const byte trackType = stream.ReadByte();
        const byte trackFlags = stream.ReadByte();
        int32 parentIndex, childrenCount;
        stream.ReadInt32(&parentIndex);
        stream.ReadInt32(&childrenCount);
        String name;
        stream.ReadString(&name, -13);
        Color32 color;
        stream.Read(&color);
        switch (trackType)
        {
        case 17:
        {
            // Animation Channel track
            const int32 channelIndex = Data.Channels.Count();
            animationChannelTrackIndexToChannelIndex[trackIndex] = channelIndex;
            auto& channel = Data.Channels.AddOne();
            channel.NodeName = name;
            break;
        }
        case 18:
        {
            // Animation Channel Data track
            const byte type = stream.ReadByte();
            int32 keyframesCount;
            stream.ReadInt32(&keyframesCount);
            int32 channelIndex;
            if (!animationChannelTrackIndexToChannelIndex.TryGet(parentIndex, channelIndex))
            {
                LOG(Error, "Invalid animation channel data track parent linkage.");
                return true;
            }
            auto& channel = Data.Channels[channelIndex];
            switch (type)
            {
            case 0:
                channel.Position.Resize(keyframesCount);
                for (int32 i = 0; i < keyframesCount; i++)
                {
                    LinearCurveKeyframe<Vector3>& k = channel.Position.GetKeyframes()[i];
                    stream.ReadFloat(&k.Time);
                    k.Time *= fps;
                    stream.Read(&k.Value);
                }
                break;
            case 1:
                channel.Rotation.Resize(keyframesCount);
                for (int32 i = 0; i < keyframesCount; i++)
                {
                    LinearCurveKeyframe<Quaternion>& k = channel.Rotation.GetKeyframes()[i];
                    stream.ReadFloat(&k.Time);
                    k.Time *= fps;
                    stream.Read(&k.Value);
                }
                break;
            case 2:
                channel.Scale.Resize(keyframesCount);
                for (int32 i = 0; i < keyframesCount; i++)
                {
                    LinearCurveKeyframe<Vector3>& k = channel.Scale.GetKeyframes()[i];
                    stream.ReadFloat(&k.Time);
                    k.Time *= fps;
                    stream.Read(&k.Value);
                }
                break;
            }
            break;
        }
        default:
            LOG(Error, "Unsupported track type {0} for animation.", trackType);
            return true;
        }
    }
    if (stream.GetLength() != stream.GetPosition())
    {
        LOG(Warning, "Invalid animation timeline data length.");
    }

    return Save();
}

bool Animation::Save(const StringView& path)
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

    // Serialize animation data to the stream
    {
        MemoryWriteStream stream(4096);

        // Info
        stream.WriteInt32(100);
        stream.WriteDouble(Data.Duration);
        stream.WriteDouble(Data.FramesPerSecond);
        stream.WriteBool(Data.EnableRootMotion);
        stream.WriteString(Data.RootNodeName, 13);

        // Animation channels
        stream.WriteInt32(Data.Channels.Count());
        for (int32 i = 0; i < Data.Channels.Count(); i++)
        {
            auto& anim = Data.Channels[i];
            stream.WriteString(anim.NodeName, 172);
            Serialization::Serialize(stream, anim.Position);
            Serialization::Serialize(stream, anim.Rotation);
            Serialization::Serialize(stream, anim.Scale);
        }

        // Set data to the chunk asset
        auto chunk0 = GetOrCreateChunk(0);
        ASSERT(chunk0 != nullptr);
        chunk0->Data.Copy(stream.GetHandle(), stream.GetPosition());
    }

    // Save
    AssetInitData data;
    data.SerializedVersion = SerializedVersion;
    const bool saveResult = path.HasChars() ? SaveAsset(path, data) : SaveAsset(data, true);
    if (saveResult)
    {
        LOG(Error, "Cannot save \'{0}\'", ToString());
        return true;
    }

    return false;
}

#endif

void Animation::OnSkinnedModelUnloaded(Asset* obj)
{
    ScopeLock lock(Locker);

    const auto key = static_cast<SkinnedModel*>(obj);
    auto i = MappingCache.Find(key);
    ASSERT(i != MappingCache.End());

    // Unlink event
    key->OnUnloaded.Unbind<Animation, &Animation::OnSkinnedModelUnloaded>(this);
    key->OnReloading.Unbind<Animation, &Animation::OnSkinnedModelUnloaded>(this);

    // Clear cache
    i->Value.Resize(0, false);
    MappingCache.Remove(i);
}

Asset::LoadResult Animation::load()
{
    // Get stream with animations data
    const auto dataChunk = GetChunk(0);
    if (dataChunk == nullptr)
        return LoadResult::MissingDataChunk;
    MemoryReadStream stream(dataChunk->Get(), dataChunk->Size());

    // Info
    int32 headerVersion = *(int32*)stream.GetPositionHandle();
    switch (headerVersion)
    {
    case 100:
    {
        stream.ReadInt32(&headerVersion);
        stream.ReadDouble(&Data.Duration);
        stream.ReadDouble(&Data.FramesPerSecond);
        Data.EnableRootMotion = stream.ReadBool();
        stream.ReadString(&Data.RootNodeName, 13);
        break;
    }
    default:
        stream.ReadDouble(&Data.Duration);
        stream.ReadDouble(&Data.FramesPerSecond);
        break;
    }
    if (Data.Duration < ZeroTolerance || Data.FramesPerSecond < ZeroTolerance)
    {
        LOG(Warning, "Invalid animation info");
        return LoadResult::Failed;
    }

    // Animation channels
    int32 animationsCount;
    stream.ReadInt32(&animationsCount);
    Data.Channels.Resize(animationsCount, false);
    for (int32 i = 0; i < animationsCount; i++)
    {
        auto& anim = Data.Channels[i];

        stream.ReadString(&anim.NodeName, 172);
        bool failed = Serialization::Deserialize(stream, anim.Position);
        failed |= Serialization::Deserialize(stream, anim.Rotation);
        failed |= Serialization::Deserialize(stream, anim.Scale);

        if (failed)
        {
            LOG(Warning, "Failed to deserialize the animation curve data.");
            return LoadResult::Failed;
        }
    }

    return LoadResult::Ok;
}

void Animation::unload(bool isReloading)
{
    ClearCache();
    Data.Dispose();
}

AssetChunksFlag Animation::getChunksToPreload() const
{
    return GET_CHUNK_FLAG(0);
}
