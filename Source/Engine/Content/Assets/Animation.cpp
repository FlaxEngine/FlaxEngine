// Copyright (c) Wojciech Figat. All rights reserved.

#include "Animation.h"
#include "SkinnedModel.h"
#include "Engine/Core/Log.h"
#include "Engine/Content/Factories/BinaryAssetFactory.h"
#include "Engine/Animations/CurveSerialization.h"
#include "Engine/Animations/AnimEvent.h"
#include "Engine/Animations/Animations.h"
#include "Engine/Animations/SceneAnimations/SceneAnimation.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Serialization/MemoryReadStream.h"
#if USE_EDITOR
#include "Engine/Serialization/MemoryWriteStream.h"
#include "Engine/Serialization/JsonWriters.h"
#include "Engine/Graphics/Models/ModelData.h"
#include "Engine/Content/JsonAsset.h"
#include "Engine/Level/Level.h"
#include "Engine/Debug/Exceptions/ArgumentOutOfRangeException.h"
#endif

REGISTER_BINARY_ASSET(Animation, "FlaxEngine.Animation", false);

Animation::Animation(const SpawnParams& params, const AssetInfo* info)
    : BinaryAsset(params, info)
{
}

#if USE_EDITOR

void Animation::OnScriptsReloadStart()
{
    for (auto& e : Events)
    {
        for (auto& k : e.Second.GetKeyframes())
            Level::ScriptsReloadRegisterObject((ScriptingObject*&)k.Value.Instance);
    }
}

#endif

Animation::InfoData Animation::GetInfo() const
{
    ScopeLock lock(Locker);
    InfoData info;
    info.MemoryUsage = sizeof(Animation);
    if (IsLoaded())
    {
        info.Length = Data.GetLength();
        info.FramesCount = (int32)Data.Duration;
        info.ChannelsCount = Data.Channels.Count();
        info.KeyframesCount = Data.GetKeyframesCount();
        info.MemoryUsage += Data.Channels.Capacity() * sizeof(NodeAnimationData);
        for (auto& e : Data.Channels)
        {
            info.MemoryUsage += (e.NodeName.Length() + 1) * sizeof(Char);
            info.MemoryUsage += e.Position.GetKeyframes().Capacity() * sizeof(LinearCurveKeyframe<Float3>);
            info.MemoryUsage += e.Rotation.GetKeyframes().Capacity() * sizeof(LinearCurveKeyframe<Quaternion>);
            info.MemoryUsage += e.Scale.GetKeyframes().Capacity() * sizeof(LinearCurveKeyframe<Float3>);
        }
    }
    else
    {
        info.Length = 0.0f;
        info.FramesCount = 0;
        info.ChannelsCount = 0;
        info.KeyframesCount = 0;
    }
    info.MemoryUsage += Events.Capacity() * sizeof(Pair<String, StepCurve<AnimEventData>>);
    info.MemoryUsage += NestedAnims.Capacity() * sizeof(Pair<String, NestedAnimData>);
    for (auto& e : Events)
        info.MemoryUsage += e.Second.GetKeyframes().Capacity() * sizeof(StepCurve<AnimEventData>);
    return info;
}

#if USE_EDITOR

void Animation::LoadTimeline(BytesContainer& result) const
{
    result.Release();
    if (!IsLoaded())
        return;
    MemoryWriteStream stream(4096);

    // Version
    stream.WriteInt32(4);

    // Meta
    float fps = (float)Data.FramesPerSecond;
    const float fpsInv = 1.0f / fps;
    stream.Write(fps);
    stream.Write((int32)Data.Duration);
    int32 tracksCount = Data.Channels.Count() + NestedAnims.Count() + Events.Count();
    for (auto& channel : Data.Channels)
    {
        tracksCount +=
                (channel.Position.GetKeyframes().HasItems() ? 1 : 0) +
                (channel.Rotation.GetKeyframes().HasItems() ? 1 : 0) +
                (channel.Scale.GetKeyframes().HasItems() ? 1 : 0);
    }
    stream.Write(tracksCount);

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
        stream.Write((byte)17); // Track Type
        stream.Write((byte)0); // Track Flags
        stream.Write(-1); // Parent Index
        stream.Write(childrenCount); // Children Count
        stream.Write(channel.NodeName, -13); // Name
        stream.Write(Color32::White); // Color
        const int32 parentIndex = trackIndex++;

        auto& position = channel.Position.GetKeyframes();
        if (position.HasItems())
        {
            // Animation Channel Data track (position)
            stream.Write((byte)18); // Track Type
            stream.Write((byte)0); // Track Flags
            stream.Write(parentIndex); // Parent Index
            stream.Write(0); // Children Count
            stream.Write(String::Format(TEXT("Track_{0}_Position"), i), -13); // Name
            stream.Write(Color32::White); // Color
            stream.Write((byte)0); // Type
            stream.Write(position.Count()); // Keyframes Count
            for (auto& k : position)
            {
                stream.Write(k.Time * fpsInv);
                stream.Write(k.Value);
            }
            trackIndex++;
        }

        auto& rotation = channel.Rotation.GetKeyframes();
        if (rotation.HasItems())
        {
            // Animation Channel Data track (rotation)
            stream.Write((byte)18); // Track Type
            stream.Write((byte)0); // Track Flags
            stream.Write(parentIndex); // Parent Index
            stream.Write(0); // Children Count
            stream.Write(String::Format(TEXT("Track_{0}_Rotation"), i), -13); // Name
            stream.Write(Color32::White); // Color
            stream.Write((byte)1); // Type
            stream.Write(rotation.Count()); // Keyframes Count
            for (auto& k : rotation)
            {
                stream.Write(k.Time * fpsInv);
                stream.Write(k.Value);
            }
            trackIndex++;
        }

        auto& scale = channel.Scale.GetKeyframes();
        if (scale.HasItems())
        {
            // Animation Channel Data track (scale)
            stream.Write((byte)18); // Track Type
            stream.Write((byte)0); // Track Flags
            stream.Write(parentIndex); // Parent Index
            stream.Write(0); // Children Count
            stream.Write(String::Format(TEXT("Track_{0}_Scale"), i), -13); // Name
            stream.Write(Color32::White); // Color
            stream.Write((byte)2); // Type
            stream.Write(scale.Count()); // Keyframes Count
            for (auto& k : scale)
            {
                stream.Write(k.Time * fpsInv);
                stream.Write(k.Value);
            }
            trackIndex++;
        }
    }
    for (auto& e : NestedAnims)
    {
        auto& nestedAnim = e.Second;
        byte flags = 0;
        if (!nestedAnim.Enabled)
            flags |= (byte)SceneAnimation::Track::Flags::Mute;
        if (nestedAnim.Loop)
            flags |= (byte)SceneAnimation::Track::Flags::Loop;
        Guid id = nestedAnim.Anim.GetID();

        // Nested Animation track
        stream.Write((byte)20); // Track Type
        stream.Write(flags); // Track Flags
        stream.Write(-1); // Parent Index
        stream.Write(0); // Children Count
        stream.Write(e.First, -13); // Name
        stream.Write(Color32::White); // Color
        stream.Write(id);
        stream.Write(nestedAnim.Time);
        stream.Write(nestedAnim.Duration);
        stream.Write(nestedAnim.Speed);
        stream.Write(nestedAnim.StartTime);
    }
    for (auto& e : Events)
    {
        // Animation Event track
        stream.Write((byte)19); // Track Type
        stream.Write((byte)0); // Track Flags
        stream.Write(-1); // Parent Index
        stream.Write(0); // Children Count
        stream.Write(e.First, -13); // Name
        stream.Write(Color32::White); // Color
        stream.Write(e.Second.GetKeyframes().Count()); // Events Count
        for (const auto& k : e.Second.GetKeyframes())
        {
            stream.Write(k.Time);
            stream.Write(k.Value.Duration);
            stream.Write(k.Value.TypeName, 13);
            stream.WriteJson(k.Value.Instance);
        }
    }

    result.Copy(ToSpan(stream));
}

bool Animation::SaveTimeline(BytesContainer& data)
{
    if (OnCheckSave())
        return true;
    ScopeLock lock(Locker);
    MemoryReadStream stream(data.Get(), data.Length());

    // Version
    int32 version;
    stream.Read(version);
    switch (version)
    {
    case 3: // [Deprecated on 03.09.2021 expires on 03.09.2023]
    case 4:
    {
        // Meta
        float fps;
        stream.Read(fps);
        Data.FramesPerSecond = static_cast<double>(fps);
        int32 duration;
        stream.Read(duration);
        Data.Duration = static_cast<double>(duration);
        int32 tracksCount;
        stream.Read(tracksCount);

        // Tracks
        Data.Channels.Clear();
        Events.Clear();
        NestedAnims.Clear();
        Dictionary<int32, int32> animationChannelTrackIndexToChannelIndex;
        animationChannelTrackIndexToChannelIndex.EnsureCapacity(tracksCount);
        for (int32 trackIndex = 0; trackIndex < tracksCount; trackIndex++)
        {
            const byte trackType = stream.ReadByte();
            const byte trackFlags = stream.ReadByte();
            int32 parentIndex, childrenCount;
            stream.Read(parentIndex);
            stream.Read(childrenCount);
            String name;
            stream.Read(name, -13);
            Color32 color;
            stream.Read(color);
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
                stream.Read(keyframesCount);
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
                        LinearCurveKeyframe<Float3>& k = channel.Position.GetKeyframes()[i];
                        stream.Read(k.Time);
                        k.Time *= fps;
                        stream.Read(k.Value);
                    }
                    break;
                case 1:
                    channel.Rotation.Resize(keyframesCount);
                    for (int32 i = 0; i < keyframesCount; i++)
                    {
                        LinearCurveKeyframe<Quaternion>& k = channel.Rotation.GetKeyframes()[i];
                        stream.Read(k.Time);
                        k.Time *= fps;
                        stream.Read(k.Value);
                    }
                    break;
                case 2:
                    channel.Scale.Resize(keyframesCount);
                    for (int32 i = 0; i < keyframesCount; i++)
                    {
                        LinearCurveKeyframe<Float3>& k = channel.Scale.GetKeyframes()[i];
                        stream.Read(k.Time);
                        k.Time *= fps;
                        stream.Read(k.Value);
                    }
                    break;
                }
                break;
            }
            case 19:
            {
                // Animation Event
                int32 count;
                stream.ReadInt32(&count);
                auto& eventTrack = Events.AddOne();
                eventTrack.First = name;
                eventTrack.Second.Resize(count);
                for (int32 i = 0; i < count; i++)
                {
                    auto& k = eventTrack.Second.GetKeyframes()[i];
                    stream.Read(k.Time);
                    stream.Read(k.Value.Duration);
                    stream.Read(k.Value.TypeName, 13);
                    const ScriptingTypeHandle typeHandle = Scripting::FindScriptingType(k.Value.TypeName);
                    k.Value.Instance = NewObject<AnimEvent>(typeHandle);
                    stream.ReadJson(k.Value.Instance);
                    if (!k.Value.Instance)
                    {
                        LOG(Error, "Failed to spawn object of type {0}.", String(k.Value.TypeName));
                        continue;
                    }
                    if (!_registeredForScriptingReload)
                    {
                        _registeredForScriptingReload = true;
                        Level::ScriptsReloadStart.Bind<Animation, &Animation::OnScriptsReloadStart>(this);
                    }
                }
                break;
            }
            case 20:
            {
                // Nested Animation
                auto& nestedTrack = NestedAnims.AddOne();
                nestedTrack.First = name;
                auto& nestedAnim = nestedTrack.Second;
                Guid id;
                stream.Read(id);
                stream.Read(nestedAnim.Time);
                stream.Read(nestedAnim.Duration);
                stream.Read(nestedAnim.Speed);
                stream.Read(nestedAnim.StartTime);
                nestedAnim.Anim = id;
                nestedAnim.Enabled = (trackFlags & (byte)SceneAnimation::Track::Flags::Mute) == 0;
                nestedAnim.Loop = (trackFlags & (byte)SceneAnimation::Track::Flags::Loop) != 0;
                break;
            }
            default:
                LOG(Error, "Unsupported track type {0} for animation.", trackType);
                return true;
            }
        }
        break;
    }
    default:
        LOG(Warning, "Unknown timeline version {0}.", version);
        return true;
    }
    if (stream.GetLength() != stream.GetPosition())
    {
        LOG(Warning, "Invalid animation timeline data length.");
    }

    return Save();
}

bool Animation::Save(const StringView& path)
{
    if (OnCheckSave(path))
        return true;
    ScopeLock lock(Locker);

    // Serialize animation data to the stream
    {
        MemoryWriteStream stream(4096);

        // Info
        stream.Write(103);
        stream.Write(Data.Duration);
        stream.Write(Data.FramesPerSecond);
        stream.Write((byte)Data.RootMotionFlags);
        stream.Write(Data.RootNodeName, 13);

        // Animation channels
        stream.WriteInt32(Data.Channels.Count());
        for (int32 i = 0; i < Data.Channels.Count(); i++)
        {
            auto& anim = Data.Channels[i];
            stream.Write(anim.NodeName, 172);
            Serialization::Serialize(stream, anim.Position);
            Serialization::Serialize(stream, anim.Rotation);
            Serialization::Serialize(stream, anim.Scale);
        }

        // Animation events
        stream.WriteInt32(Events.Count());
        for (int32 i = 0; i < Events.Count(); i++)
        {
            auto& e = Events[i];
            stream.Write(e.First, 172);
            stream.Write(e.Second.GetKeyframes().Count());
            for (const auto& k : e.Second.GetKeyframes())
            {
                stream.Write(k.Time);
                stream.Write(k.Value.Duration);
                stream.Write(k.Value.TypeName, 17);
                stream.WriteJson(k.Value.Instance);
            }
        }

        // Nested animations
        stream.WriteInt32(NestedAnims.Count());
        for (int32 i = 0; i < NestedAnims.Count(); i++)
        {
            auto& e = NestedAnims[i];
            stream.Write(e.First, 172);
            auto& nestedAnim = e.Second;
            Guid id = nestedAnim.Anim.GetID();
            byte flags = 0;
            if (nestedAnim.Enabled)
                flags |= 1;
            if (nestedAnim.Loop)
                flags |= 2;
            stream.Write(flags);
            stream.Write(id);
            stream.Write(nestedAnim.Time);
            stream.Write(nestedAnim.Duration);
            stream.Write(nestedAnim.Speed);
            stream.Write(nestedAnim.StartTime);
        }

        // Set data to the chunk asset
        auto chunk0 = GetOrCreateChunk(0);
        ASSERT(chunk0 != nullptr);
        chunk0->Data.Copy(ToSpan(stream));
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

bool Animation::SaveHeader(const ModelData& modelData, WriteStream& stream, int32 animIndex)
{
    // Validate input
    if (animIndex < 0 || animIndex >= modelData.Animations.Count())
    {
        Log::ArgumentOutOfRangeException(TEXT("animIndex"));
        return true;
    }
    auto& anim = modelData.Animations.Get()[animIndex];
    if (anim.Duration <= ZeroTolerance || anim.FramesPerSecond <= ZeroTolerance)
    {
        Log::ArgumentOutOfRangeException(TEXT("Invalid animation duration."));
        return true;
    }
    if (anim.Channels.IsEmpty())
    {
        Log::ArgumentOutOfRangeException(TEXT("Channels"), TEXT("Animation channels collection cannot be empty."));
        return true;
    }

    // Info
    stream.Write(103); // Header version (for fast version upgrades without serialization format change)
    stream.Write(anim.Duration);
    stream.Write(anim.FramesPerSecond);
    stream.Write((byte)anim.RootMotionFlags);
    stream.Write(anim.RootNodeName, 13);

    // Animation channels
    stream.WriteInt32(anim.Channels.Count());
    for (const auto& channel : anim.Channels)
    {
        stream.Write(channel.NodeName, 172);
        Serialization::Serialize(stream, channel.Position);
        Serialization::Serialize(stream, channel.Rotation);
        Serialization::Serialize(stream, channel.Scale);
    }

    // Animation events
    stream.WriteInt32(anim.Events.Count());
    for (auto& e : anim.Events)
    {
        stream.Write(e.First, 172);
        stream.Write(e.Second.GetKeyframes().Count());
        for (const auto& k : e.Second.GetKeyframes())
        {
            stream.Write(k.Time);
            stream.Write(k.Value.Duration);
            stream.Write(k.Value.TypeName, 17);
            stream.WriteJson(k.Value.JsonData);
        }
    }

    // Nested animations
    stream.WriteInt32(0); // Empty list

    return stream.HasError();
}

void Animation::GetReferences(Array<Guid>& assets, Array<String>& files) const
{
    BinaryAsset::GetReferences(assets, files);

    for (const auto& e : Events)
    {
        for (const auto& k : e.Second.GetKeyframes())
        {
            if (k.Value.Instance)
            {
                // Collect refs from Anim Event data (as Json)
                rapidjson_flax::StringBuffer buffer;
                CompactJsonWriter writer(buffer);
                writer.StartObject();
                k.Value.Instance->Serialize(writer, nullptr);
                writer.EndObject();
                JsonAssetBase::GetReferences(StringAnsiView((const char*)buffer.GetString(), (int32)buffer.GetSize()), assets);
            }
        }
    }

    // Add nested animations
    for (const auto& e : NestedAnims)
    {
        assets.Add(e.Second.Anim.GetID());
    }
}

#endif

uint64 Animation::GetMemoryUsage() const
{
    Locker.Lock();
    uint64 result = BinaryAsset::GetMemoryUsage();
    result += sizeof(Animation) - sizeof(BinaryAsset);
    result += Data.GetMemoryUsage();
    result += Events.Capacity() * sizeof(Pair<String, StepCurve<AnimEventData>>);
    for (const auto& e : Events)
        result += e.First.Length() * sizeof(Char) + e.Second.GetMemoryUsage();
    result += NestedAnims.Capacity() * sizeof(Pair<String, NestedAnimData>);
    Locker.Unlock();
    return result;
}

void Animation::OnScriptingDispose()
{
    // Dispose any events to prevent crashes (scripting is released before content)
    for (auto& e : Events)
    {
        for (auto& k : e.Second.GetKeyframes())
        {
            if (k.Value.Instance)
            {
                Delete(k.Value.Instance);
                k.Value.Instance = nullptr;
            }
        }
    }

    BinaryAsset::OnScriptingDispose();
}

Asset::LoadResult Animation::load()
{
    ConcurrentSystemLocker::WriteScope systemScope(Animations::SystemLocker);

    // Get stream with animations data
    const auto dataChunk = GetChunk(0);
    if (dataChunk == nullptr)
        return LoadResult::MissingDataChunk;
    MemoryReadStream stream(dataChunk->Get(), dataChunk->Size());

    // Info
    int32 headerVersion = *(int32*)stream.GetPositionHandle();
    switch (headerVersion)
    {
    case 103:
        stream.Read(headerVersion);
        stream.Read(Data.Duration);
        stream.Read(Data.FramesPerSecond);
        stream.Read((byte&)Data.RootMotionFlags);
        stream.Read(Data.RootNodeName, 13);
        break;
    case 100:
    case 101:
    case 102:
        stream.Read(headerVersion);
        stream.Read(Data.Duration);
        stream.Read(Data.FramesPerSecond);
        Data.RootMotionFlags = stream.ReadBool() ? AnimationRootMotionFlags::RootPositionXZ : AnimationRootMotionFlags::None;
        stream.Read(Data.RootNodeName, 13);
        break;
    default:
        stream.Read(Data.Duration);
        stream.Read(Data.FramesPerSecond);
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

        stream.Read(anim.NodeName, 172);
        bool failed = Serialization::Deserialize(stream, anim.Position);
        failed |= Serialization::Deserialize(stream, anim.Rotation);
        failed |= Serialization::Deserialize(stream, anim.Scale);

        if (failed)
        {
            LOG(Warning, "Failed to deserialize the animation curve data.");
            return LoadResult::Failed;
        }
    }

    // Animation events
    if (headerVersion >= 101)
    {
        int32 eventTracksCount;
        stream.Read(eventTracksCount);
        Events.Resize(eventTracksCount, false);
#if !USE_EDITOR
        StringAnsi typeName;
#endif
        for (int32 i = 0; i < eventTracksCount; i++)
        {
            auto& e = Events[i];
            stream.Read(e.First, 172);
            int32 eventsCount;
            stream.Read(eventsCount);
            e.Second.GetKeyframes().Resize(eventsCount);
            for (auto& k : e.Second.GetKeyframes())
            {
                stream.Read(k.Time);
                stream.Read(k.Value.Duration);
#if USE_EDITOR
                StringAnsi& typeName = k.Value.TypeName;
#endif
                stream.Read(typeName, 17);
                const ScriptingTypeHandle typeHandle = Scripting::FindScriptingType(typeName);
                k.Value.Instance = NewObject<AnimEvent>(typeHandle);
                stream.ReadJson(k.Value.Instance);
                if (!k.Value.Instance)
                {
                    LOG(Error, "Failed to spawn object of type {0}.", String(typeName));
                    continue;
                }
#if USE_EDITOR
                if (!_registeredForScriptingReload)
                {
                    _registeredForScriptingReload = true;
                    Level::ScriptsReloadStart.Bind<Animation, &Animation::OnScriptsReloadStart>(this);
                }
#endif
            }
        }
    }

    // Nested animations
    if (headerVersion >= 102)
    {
        int32 nestedAnimationsCount;
        stream.Read(nestedAnimationsCount);
        NestedAnims.Resize(nestedAnimationsCount, false);
        for (int32 i = 0; i < nestedAnimationsCount; i++)
        {
            auto& e = NestedAnims[i];
            stream.Read(e.First, 172);
            auto& nestedAnim = e.Second;
            byte flags;
            stream.Read(flags);
            nestedAnim.Enabled = flags & 1;
            nestedAnim.Loop = flags & 2;
            Guid id;
            stream.Read(id);
            nestedAnim.Anim = id;
            stream.Read(nestedAnim.Time);
            stream.Read(nestedAnim.Duration);
            stream.Read(nestedAnim.Speed);
            stream.Read(nestedAnim.StartTime);
        }
    }

    return LoadResult::Ok;
}

void Animation::unload(bool isReloading)
{
    ConcurrentSystemLocker::WriteScope systemScope(Animations::SystemLocker);
#if USE_EDITOR
    if (_registeredForScriptingReload)
    {
        _registeredForScriptingReload = false;
        Level::ScriptsReloadStart.Unbind<Animation, &Animation::OnScriptsReloadStart>(this);
    }
#endif
    Data.Release();
    for (const auto& e : Events)
    {
        for (const auto& k : e.Second.GetKeyframes())
        {
            if (k.Value.Instance)
                Delete(k.Value.Instance);
        }
    }
    Events.Clear();
    NestedAnims.Clear();
}

AssetChunksFlag Animation::getChunksToPreload() const
{
    return GET_CHUNK_FLAG(0);
}
