// Copyright (c) Wojciech Figat. All rights reserved.

#include "ParticleSystem.h"
#include "ParticleEffect.h"
#include "Engine/Core/Types/CommonValue.h"
#include "Engine/Level/Level.h"
#include "Engine/Content/Deprecated.h"
#include "Engine/Content/Factories/BinaryAssetFactory.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Serialization/MemoryWriteStream.h"
#include "Engine/Threading/Threading.h"

REGISTER_BINARY_ASSET(ParticleSystem, "FlaxEngine.ParticleSystem", true);

ParticleSystem::ParticleSystem(const SpawnParams& params, const AssetInfo* info)
    : BinaryAsset(params, info)
    , FramesPerSecond(1)
    , DurationFrames(0)
{
}

void ParticleSystem::Init(ParticleEmitter* emitter, float duration, float fps)
{
    CHECK(IsVirtual());
    ScopeLock lock(Locker);
    Version++;
    FramesPerSecond = fps;
    DurationFrames = Math::CeilToInt(duration * fps);
    Emitters.Resize(0);
    Tracks.Resize(0);
    if (emitter)
    {
        Emitters.Add(emitter);
        auto& track = Tracks.AddOne();
        track.Type = Track::Types::Emitter;
        track.Flag = Track::Flags::None;
        track.ChildrenCount = 0;
        track.ParentIndex = -1;
        track.Name = TEXT("");
        track.Disabled = false;
        track.AsEmitter.Index = 0;
        track.AsEmitter.StartFrame = 0;
        track.AsEmitter.DurationFrames = DurationFrames;
#if !BUILD_RELEASE
        _debugName = StringUtils::GetFileNameWithoutExtension(emitter->GetPath());
#endif
    }
}

BytesContainer ParticleSystem::LoadTimeline() const
{
    BytesContainer result;
    ScopeLock lock(Locker);

    // Serialize timeline to stream
    MemoryWriteStream stream(512);
    {
        // Save properties
        stream.WriteInt32(4);
        stream.WriteFloat(FramesPerSecond);
        stream.WriteInt32(DurationFrames);

        // Save emitters
        const int32 emittersCount = Emitters.Count();
        stream.WriteInt32(emittersCount);

        // Save tracks
        const int32 tracksCount = Tracks.Count();
        stream.WriteInt32(tracksCount);
        for (int32 i = 0; i < tracksCount; i++)
        {
            const auto& track = Tracks[i];

            stream.WriteByte((byte)track.Type);
            stream.WriteByte((byte)track.Flag);
            stream.WriteInt32(track.ParentIndex);
            stream.WriteInt32(track.ChildrenCount);
            stream.Write(track.Name, -13);
            stream.Write(track.Color);

            Guid id;
            switch (track.Type)
            {
            case Track::Types::Emitter:
                id = Emitters[track.AsEmitter.Index].GetID();
                stream.Write(id);
                stream.WriteInt32(track.AsEmitter.Index);
                stream.WriteInt32(track.AsEmitter.StartFrame);
                stream.WriteInt32(track.AsEmitter.DurationFrames);
                break;
            case Track::Types::Folder:
                break;
            default:
                return result;
            }
        }

        // Save parameters overrides
        if (EmittersParametersOverrides.HasItems())
        {
            stream.WriteInt32(EmittersParametersOverrides.Count());
            for (auto i = EmittersParametersOverrides.Begin(); i.IsNotEnd(); ++i)
            {
                stream.WriteInt32(i->Key.First);
                stream.Write(i->Key.Second);
                stream.Write(i->Value);
            }
        }
    }

    // Set output data
    result.Copy(ToSpan(stream));
    return result;
}

#if USE_EDITOR

bool ParticleSystem::SaveTimeline(const BytesContainer& data) const
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

#endif

ParticleEffect* ParticleSystem::Spawn(Actor* parent, const Transform& transform, bool autoDestroy)
{
    CHECK_RETURN(!WaitForLoaded(), nullptr);

    auto effect = New<ParticleEffect>();
    effect->SetTransform(transform);
    effect->ParticleSystem = this;
#if !BUILD_RELEASE
    effect->SetName(_debugName); // Give usable name in development builds
#endif

    Level::SpawnActor(effect, parent);

    if (autoDestroy)
        effect->DeleteObject(GetDuration(), true);

    return effect;
}

void ParticleSystem::InitAsVirtual()
{
    // Base
    BinaryAsset::InitAsVirtual();

    Version++;
}

#if USE_EDITOR

void ParticleSystem::GetReferences(Array<Guid>& assets, Array<String>& files) const
{
    // Base
    BinaryAsset::GetReferences(assets, files);

    for (int32 i = 0; i < Emitters.Count(); i++)
        assets.Add(Emitters[i].GetID());

    for (auto i = EmittersParametersOverrides.Begin(); i.IsNotEnd(); ++i)
    {
        const auto id = (Guid)i->Value;
        if (id.IsValid())
            assets.Add(id);
    }
}

bool ParticleSystem::Save(const StringView& path)
{
    if (OnCheckSave(path))
        return true;
    ScopeLock lock(Locker);
    BytesContainer data = LoadTimeline();
    return SaveTimeline(data);
}

#endif

Asset::LoadResult ParticleSystem::load()
{
    Version++;

    // Get the data chunk
    if (LoadChunk(0))
        return LoadResult::CannotLoadData;
    const auto chunk0 = GetChunk(0);
    if (chunk0 == nullptr || chunk0->IsMissing())
        return LoadResult::MissingDataChunk;
    MemoryReadStream stream(chunk0->Data.Get(), chunk0->Data.Length());

    int32 version;
    stream.Read(version);
#if USE_EDITOR
    // Skip unused parameters
#define SKIP_UNUSED_PARAM_OVERRIDE() if (key.First < 0 || key.First >= Emitters.Count() || Emitters[key.First] == nullptr || Emitters[key.First]->Graph.GetParameter(key.Second) == nullptr) continue
#else
#define SKIP_UNUSED_PARAM_OVERRIDE()
#endif
    switch (version)
    {
    PRAGMA_DISABLE_DEPRECATION_WARNINGS
    case 1:
    {
        // [Deprecated on 23.07.2019, expires on 27.04.2021]
        MARK_CONTENT_DEPRECATED();

        // Load properties
        stream.ReadFloat(&FramesPerSecond);
        stream.ReadInt32(&DurationFrames);

        // Load emitters
        int32 emittersCount;
        stream.ReadInt32(&emittersCount);
        Emitters.Resize(emittersCount, false);

        // Load tracks
        Guid id;
        int32 tracksCount;
        stream.ReadInt32(&tracksCount);
        Tracks.Resize(tracksCount, false);
        for (int32 i = 0; i < tracksCount; i++)
        {
            auto& track = Tracks[i];

            track.Type = (Track::Types)stream.ReadByte();
            track.Flag = (Track::Flags)stream.ReadByte();
            stream.ReadInt32(&track.ParentIndex);
            stream.ReadInt32(&track.ChildrenCount);
            stream.ReadString(&track.Name, -13);
            track.Disabled = (int32)track.Flag & (int32)Track::Flags::Mute || (track.ParentIndex != -1 && Tracks[track.ParentIndex].Disabled);

            switch (track.Type)
            {
            case Track::Types::Emitter:
                stream.Read(id);
                stream.ReadInt32(&track.AsEmitter.Index);
                stream.ReadInt32(&track.AsEmitter.StartFrame);
                stream.ReadInt32(&track.AsEmitter.DurationFrames);
                Emitters[track.AsEmitter.Index] = id;
                break;
            case Track::Types::Folder:
                stream.Read(track.Color);
                break;
            default:
                return LoadResult::InvalidData;
            }
        }

        // Wait for all tracks to be loaded - particle system cannot be used if any of the emitters is not loaded yet
        // Note: this loop might trigger loading referenced assets on this thread
        for (int32 i = 0; i < Emitters.Count(); i++)
        {
            if (Emitters[i])
                Emitters[i]->WaitForLoaded();
        }

        // Load parameters overrides
        int32 overridesCount = 0;
        if (stream.CanRead())
            stream.ReadInt32(&overridesCount);
        if (overridesCount != 0)
        {
            EmitterParameterOverrideKey key;
            CommonValue value;
            for (int32 i = 0; i < overridesCount; i++)
            {
                stream.ReadInt32(&key.First);
                stream.Read(key.Second);
                stream.ReadCommonValue(&value);
                SKIP_UNUSED_PARAM_OVERRIDE();
                EmittersParametersOverrides.Add(key, Variant(value));
            }
        }

        break;
    }
    case 2:
    {
        // [Deprecated on 31.07.2020, expires on 31.07.2022]
        MARK_CONTENT_DEPRECATED();

        // Load properties
        stream.ReadFloat(&FramesPerSecond);
        stream.ReadInt32(&DurationFrames);

        // Load emitters
        int32 emittersCount;
        stream.ReadInt32(&emittersCount);
        Emitters.Resize(emittersCount, false);

        // Load tracks
        Guid id;
        int32 tracksCount;
        stream.ReadInt32(&tracksCount);
        Tracks.Resize(tracksCount, false);
        for (int32 i = 0; i < tracksCount; i++)
        {
            auto& track = Tracks[i];

            track.Type = (Track::Types)stream.ReadByte();
            track.Flag = (Track::Flags)stream.ReadByte();
            stream.ReadInt32(&track.ParentIndex);
            stream.ReadInt32(&track.ChildrenCount);
            stream.ReadString(&track.Name, -13);
            track.Disabled = (int32)track.Flag & (int32)Track::Flags::Mute || (track.ParentIndex != -1 && Tracks[track.ParentIndex].Disabled);
            stream.Read(track.Color);

            switch (track.Type)
            {
            case Track::Types::Emitter:
                stream.Read(id);
                stream.ReadInt32(&track.AsEmitter.Index);
                stream.ReadInt32(&track.AsEmitter.StartFrame);
                stream.ReadInt32(&track.AsEmitter.DurationFrames);
                Emitters[track.AsEmitter.Index] = id;
                break;
            case Track::Types::Folder:
                break;
            default:
                return LoadResult::InvalidData;
            }
        }

        // Wait for all tracks to be loaded - particle system cannot be used if any of the emitters is not loaded yet
        // Note: this loop might trigger loading referenced assets on this thread
        for (int32 i = 0; i < Emitters.Count(); i++)
        {
            if (Emitters[i])
                Emitters[i]->WaitForLoaded();
        }

        // Load parameters overrides
        int32 overridesCount = 0;
        if (stream.CanRead())
            stream.ReadInt32(&overridesCount);
        if (overridesCount != 0)
        {
            EmitterParameterOverrideKey key;
            CommonValue value;
            for (int32 i = 0; i < overridesCount; i++)
            {
                stream.ReadInt32(&key.First);
                stream.Read(key.Second);
                stream.ReadCommonValue(&value);
                SKIP_UNUSED_PARAM_OVERRIDE();
                EmittersParametersOverrides[key] = Variant(value);
            }
        }

        break;
    }
    PRAGMA_ENABLE_DEPRECATION_WARNINGS
    case 3: // [Deprecated on 03.09.2021 expires on 03.09.2023]
        MARK_CONTENT_DEPRECATED();
    case 4:
    {
        // Load properties
        stream.ReadFloat(&FramesPerSecond);
        stream.ReadInt32(&DurationFrames);

        // Load emitters
        int32 emittersCount;
        stream.ReadInt32(&emittersCount);
        Emitters.Resize(emittersCount, false);

        // Load tracks
        Guid id;
        int32 tracksCount;
        stream.ReadInt32(&tracksCount);
        Tracks.Resize(tracksCount, false);
        for (int32 i = 0; i < tracksCount; i++)
        {
            auto& track = Tracks[i];

            track.Type = (Track::Types)stream.ReadByte();
            track.Flag = (Track::Flags)stream.ReadByte();
            stream.ReadInt32(&track.ParentIndex);
            stream.ReadInt32(&track.ChildrenCount);
            stream.Read(track.Name, -13);
            track.Disabled = (int32)track.Flag & (int32)Track::Flags::Mute || (track.ParentIndex != -1 && Tracks[track.ParentIndex].Disabled);
            stream.Read(track.Color);

            switch (track.Type)
            {
            case Track::Types::Emitter:
                stream.Read(id);
                stream.ReadInt32(&track.AsEmitter.Index);
                stream.ReadInt32(&track.AsEmitter.StartFrame);
                stream.ReadInt32(&track.AsEmitter.DurationFrames);
                Emitters[track.AsEmitter.Index] = id;
                break;
            case Track::Types::Folder:
                break;
            default:
                return LoadResult::InvalidData;
            }
        }

        // Wait for all tracks to be loaded - particle system cannot be used if any of the emitters is not loaded yet
        // Note: this loop might trigger loading referenced assets on this thread
        for (int32 i = 0; i < Emitters.Count(); i++)
        {
            if (Emitters[i])
                Emitters[i]->WaitForLoaded();
        }

        // Load parameters overrides
        int32 overridesCount = 0;
        if (stream.CanRead())
            stream.ReadInt32(&overridesCount);
        if (overridesCount != 0)
        {
            EmitterParameterOverrideKey key;
            Variant value;
            for (int32 i = 0; i < overridesCount; i++)
            {
                stream.ReadInt32(&key.First);
                stream.Read(key.Second);
                stream.Read(value);
                SKIP_UNUSED_PARAM_OVERRIDE();
                EmittersParametersOverrides[key] = value;
            }
        }

        break;
    }
    default:
        LOG(Warning, "Unknown timeline version {0}.", version);
        return LoadResult::InvalidData;
    }
#undef SKIP_UNUSED_PARAM_OVERRIDE

#if !BUILD_RELEASE
    _debugName = StringUtils::GetFileNameWithoutExtension(GetPath());
#endif
    return LoadResult::Ok;
}

void ParticleSystem::unload(bool isReloading)
{
    Version++;
    FramesPerSecond = 0.0f;
    DurationFrames = 0;
    Emitters.Resize(0);
    EmittersParametersOverrides.SetCapacity(0);
    Tracks.Resize(0);
#if !BUILD_RELEASE
    _debugName.Clear();
#endif
}

AssetChunksFlag ParticleSystem::getChunksToPreload() const
{
    return GET_CHUNK_FLAG(0);
}
