// Copyright (c) Wojciech Figat. All rights reserved.

#include "SceneAnimationPlayer.h"
#include "Engine/Core/Random.h"
#include "Engine/Engine/Time.h"
#include "Engine/Level/Scene/Scene.h"
#include "Engine/Level/SceneObjectsFactory.h"
#include "Engine/Level/Actors/Camera.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Serialization/MemoryReadStream.h"
#include "Engine/Audio/AudioClip.h"
#include "Engine/Audio/AudioSource.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Renderer/RenderList.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Scripting/Script.h"
#include "Engine/Scripting/ManagedCLR/MException.h"
#include "Engine/Scripting/ManagedCLR/MProperty.h"
#include "Engine/Scripting/ManagedCLR/MUtils.h"
#include "Engine/Scripting/ManagedCLR/MField.h"
#include "Engine/Scripting/ManagedCLR/MClass.h"
#include "Engine/Scripting/ManagedCLR/MMethod.h"
#include "Engine/Scripting/Internal/ManagedSerialization.h"

// This could be Update, LateUpdate or FixedUpdate
#define UPDATE_POINT Update
#define REGISTER_TICK GetScene()->Ticking.UPDATE_POINT.AddTick<SceneAnimationPlayer, &SceneAnimationPlayer::Tick>(this)
#define UNREGISTER_TICK GetScene()->Ticking.UPDATE_POINT.RemoveTick(this)

SceneAnimationPlayer::SceneAnimationPlayer(const SpawnParams& params)
    : Actor(params)
{
    Animation.Changed.Bind<SceneAnimationPlayer, &SceneAnimationPlayer::OnAnimationModified>(this);
    Animation.Loaded.Bind<SceneAnimationPlayer, &SceneAnimationPlayer::OnAnimationModified>(this);
}

float SceneAnimationPlayer::GetTime() const
{
    return _time;
}

void SceneAnimationPlayer::SetTime(float value)
{
    _time = value;
}

void SceneAnimationPlayer::Play()
{
    if (_state == PlayState::Playing)
        return;
    if (!IsDuringPlay())
    {
        LOG(Warning, "Cannot play scene animation. Actor is not in a game.");
        return;
    }
    if (Animation == nullptr)
    {
        LOG(Warning, "Cannot play scene animation. No asset assigned.");
        return;
    }
    if (Animation->WaitForLoaded())
    {
        LOG(Warning, "Cannot play scene animation. Failed to load asset.");
        return;
    }

    if (_state == PlayState::Stopped)
    {
        if (RandomStartTime)
        {
            _time = Random::Rand() * Animation->GetDuration();
        }
        else
        {
            _time = StartTime;
        }

        GetSceneRendering()->AddPostFxProvider(this);
    }

    _state = PlayState::Playing;
    _lastTime = _time;

    if (IsActiveInHierarchy())
    {
        REGISTER_TICK;
    }
}

void SceneAnimationPlayer::Pause()
{
    if (_state != PlayState::Playing)
        return;

    if (IsActiveInHierarchy())
    {
        UNREGISTER_TICK;
    }

    for (auto actor : _subActors)
    {
        if (auto audioSource = dynamic_cast<AudioSource*>(actor))
            audioSource->Pause();
    }

    _state = PlayState::Paused;
}

void SceneAnimationPlayer::Stop()
{
    if (_state == PlayState::Stopped)
        return;

    if (IsActiveInHierarchy() && _state == PlayState::Playing)
    {
        UNREGISTER_TICK;
    }

    if (RestoreStateOnStop && _restoreData.HasItems())
    {
        SceneAnimation* anim = Animation.Get();
        if (anim && anim->IsLoaded())
        {
            Restore(anim, 0);
        }
    }

    if (_isUsingCameraCuts && _cameraCutCam == Camera::CutSceneCamera)
        Camera::CutSceneCamera = _cameraCutCam = nullptr;
    _isUsingCameraCuts = false;

    for (auto actor : _subActors)
    {
        if (auto audioSource = dynamic_cast<AudioSource*>(actor))
            audioSource->Stop();
    }

    GetSceneRendering()->RemovePostFxProvider(this);
    _state = PlayState::Stopped;
    _time = _lastTime = 0.0f;
    _tracksDataStack.Resize(0);
}

void SceneAnimationPlayer::Tick(float dt)
{
    // Reset temporary state
    _postFxSettings.PostFxMaterials.Materials.Clear();
    _postFxSettings.CameraArtifacts.OverrideFlags &= ~CameraArtifactsSettings::Override::ScreenFadeColor;

    // Skip tick if animation asset is not ready for playback
    SceneAnimation* anim = Animation.Get();
    if (!anim || !anim->IsLoaded())
        return;

    // Setup state
    if (_tracks.Count() != anim->TrackStatesCount)
    {
        ResetState();
        _tracks.Resize(anim->TrackStatesCount);
    }

    // Update timing
    float time = _time;
    if (Math::NearEqual(_lastTime, _time))
    {
        // Delta time animation
        time += dt;
    }
    else
    {
        // Time was changed via SetTime
    }
    const float fps = anim->FramesPerSecond;
    const float duration = anim->DurationFrames / fps;
    if (time > duration)
    {
        if (Loop)
        {
            // Loop
            time = Math::Mod(time, duration);
        }
        else
        {
            // End
            Stop();
            return;
        }
    }

    const Camera* prevCamera = Camera::GetMainCamera();
    if (_isUsingCameraCuts && _cameraCutCam == Camera::CutSceneCamera)
    {
        Camera::CutSceneCamera = _cameraCutCam = nullptr;
    }
    _isUsingCameraCuts = false;
    _cameraCutCam = nullptr;

    // Tick the animation
    CallStack callStack;
    Tick(anim, time, dt, 0, callStack);
#if !BUILD_RELEASE
    if (_tracksDataStack.Count() != 0)
    {
        _tracksDataStack.Resize(0);
        LOG(Warning, "Invalid track states data stack size.");
    }
#endif

    if (_isUsingCameraCuts)
    {
        Camera::CutSceneCamera = _cameraCutCam;

        // Automatic camera-cuts injection for renderer
        if (prevCamera != Camera::GetMainCamera() && MainRenderTask::Instance)
            MainRenderTask::Instance->IsCameraCut = true;
    }

    // Update time
    _lastTime = _time = time;
}

void SceneAnimationPlayer::MapObject(const Guid& from, const Guid& to)
{
    _objectsMapping[from] = to;
}

void SceneAnimationPlayer::MapTrack(const StringView& from, const Guid& to)
{
    SceneAnimation* anim = Animation.Get();
    if (!anim || !anim->IsLoaded())
        return;
    for (int32 j = 0; j < anim->Tracks.Count(); j++)
    {
        const auto& track = anim->Tracks[j];
        if (track.Name != from)
            continue;
        switch (track.Type)
        {
        case SceneAnimation::Track::Types::Actor:
        {
            const auto trackData = track.GetData<SceneAnimation::ActorTrack::Data>();
            _objectsMapping[trackData->ID] = to;
            return;
        }
        case SceneAnimation::Track::Types::Script:
        {
            const auto trackData = track.GetData<SceneAnimation::ScriptTrack::Data>();
            _objectsMapping[trackData->ID] = to;
            return;
        }
        case SceneAnimation::Track::Types::CameraCut:
        {
            const auto trackData = track.GetData<SceneAnimation::CameraCutTrack::Data>();
            _objectsMapping[trackData->ID] = to;
            return;
        }
        default: ;
        }
    }
    LOG(Warning, "Missing track '{0}' in scene animation '{1}' to map into object ID={2}", from, anim->ToString(), to);
}

void SceneAnimationPlayer::Restore(SceneAnimation* anim, int32 stateIndexOffset)
{
#if USE_CSHARP
    // Restore all tracks
    for (int32 j = 0; j < anim->Tracks.Count(); j++)
    {
        const auto& track = anim->Tracks[j];
        if (track.Disabled)
            continue;
        switch (track.Type)
        {
        case SceneAnimation::Track::Types::Actor:
        case SceneAnimation::Track::Types::Script:
        case SceneAnimation::Track::Types::CameraCut:
        {
            auto& state = _tracks[stateIndexOffset + track.TrackStateIndex];
            state.ManagedObject = state.Object.GetOrCreateManagedInstance();
            break;
        }
        case SceneAnimation::Track::Types::KeyframesProperty:
        case SceneAnimation::Track::Types::CurveProperty:
        case SceneAnimation::Track::Types::StringProperty:
        case SceneAnimation::Track::Types::ObjectReferenceProperty:
        case SceneAnimation::Track::Types::StructProperty:
        case SceneAnimation::Track::Types::ObjectProperty:
        {
            if (track.ParentIndex == -1)
                break;
            auto& state = _tracks[stateIndexOffset + track.TrackStateIndex];
            const auto& parentTrack = anim->Tracks[track.ParentIndex];

            // Skip if cannot restore state
            if (parentTrack.Type == SceneAnimation::Track::Types::StructProperty
                || state.RestoreStateIndex == -1
                || (state.Field == nullptr && state.Property == nullptr))
                break;
            MObject* instance = _tracks[stateIndexOffset + parentTrack.TrackStateIndex].ManagedObject;
            if (!instance)
                break;

            // Get the value data
            void* value;
            switch (track.Type)
            {
            case SceneAnimation::Track::Types::StringProperty:
                value = &_restoreData[state.RestoreStateIndex];
                value = MUtils::ToString(StringView((Char*)value));
                break;
            case SceneAnimation::Track::Types::ObjectReferenceProperty:
            {
                value = &_restoreData[state.RestoreStateIndex];
                Guid id = *(Guid*)value;
                _objectsMapping.TryGet(id, id);
                auto obj = Scripting::FindObject<ScriptingObject>(id);
                value = obj ? obj->GetOrCreateManagedInstance() : nullptr;
                break;
            }
            case SceneAnimation::Track::Types::ObjectProperty:
            {
                if (state.Property)
                {
                    MObject* exception = nullptr;
                    state.ManagedObject = state.Property->GetValue(instance, &exception);
                    if (exception)
                    {
                        MException ex(exception);
                        ex.Log(LogType::Error, TEXT("Property"));
                        state.ManagedObject = nullptr;
                    }
                }
                else
                {
                    state.Field->GetValue(instance, &state.ManagedObject);
                }
                value = state.ManagedObject;
                break;
            }
            default:
                value = &_restoreData[state.RestoreStateIndex];
            }

            // Set the value
            if (state.Property)
            {
                MObject* exception = nullptr;
                state.Property->SetValue(instance, value, &exception);
                if (exception)
                {
                    MException ex(exception);
                    ex.Log(LogType::Error, TEXT("Property"));
                }
            }
            else
            {
                state.Field->SetValue(instance, value);
            }
            break;
        }
        default: ;
        }
    }
#endif
}

bool SceneAnimationPlayer::TickPropertyTrack(int32 trackIndex, int32 stateIndexOffset, SceneAnimation* anim, float time, const SceneAnimation::Track& track, TrackInstance& state, void* target)
{
#if USE_CSHARP
    switch (track.Type)
    {
    case SceneAnimation::Track::Types::KeyframesProperty:
    case SceneAnimation::Track::Types::ObjectReferenceProperty:
    {
        const auto trackRuntime = track.GetRuntimeData<SceneAnimation::KeyframesPropertyTrack::Runtime>();
        const int32 count = trackRuntime->KeyframesCount;
        if (count == 0)
            return false;

        // If size is 0 then track uses Json storage for keyframes data (variable memory length of keyframes), otherwise it's optimized simple data with O(1) access
        if (trackRuntime->ValueSize != 0)
        {
            // Find the keyframe at time (binary search)
            int32 keyframeSize = sizeof(float) + trackRuntime->ValueSize;
#define GET_KEY_TIME(idx) *(float*)((byte*)trackRuntime->Keyframes + keyframeSize * (idx))
            const float keyTime = Math::Clamp(time, 0.0f, GET_KEY_TIME(count - 1));
            int32 start = 0;
            int32 searchLength = count;
            while (searchLength > 0)
            {
                const int32 half = searchLength >> 1;
                int32 mid = start + half;
                if (keyTime < GET_KEY_TIME(mid))
                {
                    searchLength = half;
                }
                else
                {
                    start = mid + 1;
                    searchLength -= half + 1;
                }
            }
            int32 leftKey = Math::Max(0, start - 1);
#undef GET_KEY_TIME

            // Return the value
            void* value = (void*)((byte*)trackRuntime->Keyframes + keyframeSize * (leftKey) + sizeof(float));
            if (track.Type == SceneAnimation::Track::Types::ObjectReferenceProperty)
            {
                // Object ref track uses Guid for object Id storage
                Guid id = *(Guid*)value;
                _objectsMapping.TryGet(id, id);
                auto obj = Scripting::FindObject<ScriptingObject>(id);
                value = obj ? obj->GetOrCreateManagedInstance() : nullptr;
                *(void**)target = value;
            }
            else
            {
                // POD memory
                Platform::MemoryCopy(target, value, trackRuntime->ValueSize);
            }
        }
        else
        {
            // Clear pointer
            *(void**)target = nullptr;

            // Find the keyframe at time (linear search)
            MemoryReadStream stream((byte*)trackRuntime->Keyframes, trackRuntime->KeyframesSize);
            int32 prevKeyPos = sizeof(float);
            int32 jsonLen;
            for (int32 key = 0; key < count; key++)
            {
                float keyTime;
                stream.ReadFloat(&keyTime);
                if (keyTime > time)
                    break;
                prevKeyPos = stream.GetPosition();
                stream.ReadInt32(&jsonLen);
                stream.Move(jsonLen);
            }

            // Read json text
            stream.SetPosition(prevKeyPos);
            stream.ReadInt32(&jsonLen);
            const StringAnsiView json((const char*)stream.GetPositionHandle(), jsonLen);

            // Create empty value of the keyframe type
            const auto trackData = track.GetData<SceneAnimation::KeyframesPropertyTrack::Data>();
            const StringAnsiView propertyTypeName(trackRuntime->PropertyTypeName, trackData->PropertyTypeNameLength);
            MClass* klass = Scripting::FindClass(propertyTypeName);
            if (!klass)
                return false;
            MObject* obj = MCore::Object::New(klass);
            if (!obj)
                return false;
            if (!klass->IsValueType())
                MCore::Object::Init(obj);

            // Deserialize value from json
            ManagedSerialization::Deserialize(json, obj);

            // Set value
            *(void**)target = obj;
        }
        break;
    }
    case SceneAnimation::Track::Types::CurveProperty:
    {
        const auto trackDataKeyframes = track.GetRuntimeData<SceneAnimation::CurvePropertyTrack::Runtime>();
        const int32 count = trackDataKeyframes->KeyframesCount;
        if (count == 0)
            return false;

        // Evaluate the curve
        byte valueData[sizeof(Double4)];
        void* curveDst = trackDataKeyframes->DataType == trackDataKeyframes->ValueType ? target : valueData;
        switch (trackDataKeyframes->DataType)
        {
#define IMPL_CURVE(dataType, valueType) \
    case SceneAnimation::CurvePropertyTrack::DataTypes::dataType: \
    { \
        CurveBase<valueType, BezierCurveKeyframe<valueType>> volumeCurve; \
        CurveBase<valueType, BezierCurveKeyframe<valueType>>::KeyFrameData data((BezierCurveKeyframe<valueType>*)trackDataKeyframes->Keyframes, trackDataKeyframes->KeyframesCount); \
        static_assert(sizeof(valueData) >= sizeof(valueType), "Invalid valueData size."); \
        volumeCurve.Evaluate(data, *(valueType*)curveDst, time, false); \
        break; \
    }
        IMPL_CURVE(Float, float);
        IMPL_CURVE(Double, double);
        IMPL_CURVE(Float2, Float2);
        IMPL_CURVE(Float3, Float3);
        IMPL_CURVE(Float4, Float4);
        IMPL_CURVE(Double2, Double2);
        IMPL_CURVE(Double3, Double3);
        IMPL_CURVE(Double4, Double4);
        IMPL_CURVE(Quaternion, Quaternion);
        IMPL_CURVE(Color, Color);
        IMPL_CURVE(Color32, Color32);
#undef IMPL_CURVE
        default: ;
        }
        if (trackDataKeyframes->DataType != trackDataKeyframes->ValueType)
        {
            // Convert evaluated curve data into the runtime type (eg. when using animation saved with Vector3=Double3 and playing it in a build with Vector3=Float3)
            switch (trackDataKeyframes->DataType)
            {
            // Assume just Vector type converting
            case SceneAnimation::CurvePropertyTrack::DataTypes::Float2:
                *(Double2*)target = *(Float2*)valueData;
                break;
            case SceneAnimation::CurvePropertyTrack::DataTypes::Float3:
                *(Double3*)target = *(Float3*)valueData;
                break;
            case SceneAnimation::CurvePropertyTrack::DataTypes::Float4:
                *(Double4*)target = *(Float4*)valueData;
                break;
            case SceneAnimation::CurvePropertyTrack::DataTypes::Double2:
                *(Float2*)target = *(Double2*)valueData;
                break;
            case SceneAnimation::CurvePropertyTrack::DataTypes::Double3:
                *(Float3*)target = *(Double3*)valueData;
                break;
            case SceneAnimation::CurvePropertyTrack::DataTypes::Double4:
                *(Float4*)target = *(Double4*)valueData;
                break;
            }
        }
        break;
    }
    case SceneAnimation::Track::Types::StringProperty:
    {
        const auto trackRuntime = track.GetRuntimeData<SceneAnimation::StringPropertyTrack::Runtime>();
        const int32 count = trackRuntime->KeyframesCount;
        if (count == 0)
            return false;
        const auto keyframesTimes = (float*)((byte*)trackRuntime + sizeof(SceneAnimation::StringPropertyTrack::Runtime));
        const auto keyframesLengths = (int32*)((byte*)keyframesTimes + sizeof(float) * trackRuntime->KeyframesCount);
        const auto keyframesValues = (Char**)((byte*)keyframesLengths + sizeof(int32) * trackRuntime->KeyframesCount);

        // Find the keyframe at time
#define GET_KEY_TIME(idx) keyframesTimes[idx]
        const float keyTime = Math::Clamp(time, 0.0f, GET_KEY_TIME(count - 1));
        int32 start = 0;
        int32 searchLength = count;
        while (searchLength > 0)
        {
            const int32 half = searchLength >> 1;
            int32 mid = start + half;
            if (keyTime < GET_KEY_TIME(mid))
            {
                searchLength = half;
            }
            else
            {
                start = mid + 1;
                searchLength -= half + 1;
            }
        }
        int32 leftKey = Math::Max(0, start - 1);
#undef GET_KEY_TIME

        // Return the value
        StringView str(keyframesValues[leftKey], keyframesLengths[leftKey]);
        *(MString**)target = MUtils::ToString(str);
        break;
    }
    case SceneAnimation::Track::Types::StructProperty:
    {
        // Evaluate all child tracks
        for (int32 childTrackIndex = trackIndex + 1; childTrackIndex < anim->Tracks.Count(); childTrackIndex++)
        {
            auto& childTrack = anim->Tracks[childTrackIndex];
            if (childTrack.Disabled || childTrack.ParentIndex != trackIndex)
                continue;
            const auto childTrackRuntime = childTrack.GetRuntimeData<SceneAnimation::PropertyTrack::Runtime>();
            auto& childTrackState = _tracks[stateIndexOffset + childTrack.TrackStateIndex];

            // Cache field
            if (!childTrackState.Field)
            {
                MType* type = state.Property ? state.Property->GetType() : (state.Field ? state.Field->GetType() : nullptr);
                if (!type)
                    continue;
                MClass* mclass = MCore::Type::GetClass(type);
                childTrackState.Field = mclass->GetField(childTrackRuntime->PropertyName);
                if (!childTrackState.Field)
                    continue;
            }

            // Sample child track
            TickPropertyTrack(childTrackIndex, stateIndexOffset, anim, time, childTrack, childTrackState, (byte*)target + childTrackState.Field->GetOffset());
        }
        break;
    }
    case SceneAnimation::Track::Types::ObjectProperty:
    {
        // Cache the sub-object pointer for the sub-tracks
        state.ManagedObject = *(MObject**)target;
        return false;
    }
    default: ;
    }
#endif
    return true;
}

void SceneAnimationPlayer::Tick(SceneAnimation* anim, float time, float dt, int32 stateIndexOffset, CallStack& callStack)
{
#if USE_CSHARP
    const float fps = anim->FramesPerSecond;
#if !BUILD_RELEASE || USE_EDITOR
    callStack.Add(anim);
#endif

    // Update all tracks
    for (int32 j = 0; j < anim->Tracks.Count(); j++)
    {
        const auto& track = anim->Tracks[j];
        if (track.Disabled)
            continue;
        switch (track.Type)
        {
        case SceneAnimation::Track::Types::PostProcessMaterial:
        {
            const auto runtimeData = track.GetRuntimeData<SceneAnimation::PostProcessMaterialTrack::Runtime>();
            for (int32 k = 0; k < runtimeData->Count; k++)
            {
                const auto& media = runtimeData->Media[k];
                const float startTime = media.StartFrame / fps;
                const float durationTime = media.DurationFrames / fps;
                const bool isActive = Math::IsInRange(time, startTime, startTime + durationTime);
                if (isActive && _postFxSettings.PostFxMaterials.Materials.Count() < POST_PROCESS_SETTINGS_MAX_MATERIALS)
                {
                    _postFxSettings.PostFxMaterials.Materials.Add(track.Asset.As<MaterialBase>());
                    break;
                }
            }
            break;
        }
        case SceneAnimation::Track::Types::NestedSceneAnimation:
        {
            const auto nestedAnim = track.Asset.As<SceneAnimation>();
            if (!nestedAnim || !nestedAnim->IsLoaded())
                break;
            const auto trackData = track.GetData<SceneAnimation::NestedSceneAnimationTrack::Data>();
            const float startTime = trackData->StartFrame / fps;
            const float durationTime = trackData->DurationFrames / fps;
            const bool loop = ((int32)track.Flag & (int32)SceneAnimation::Track::Flags::Loop) == (int32)SceneAnimation::Track::Flags::Loop;
            float mediaTime = time - startTime;
            if (mediaTime >= 0.0f && mediaTime <= durationTime)
            {
                const float mediaDuration = nestedAnim->DurationFrames / nestedAnim->FramesPerSecond;
                if (mediaTime > mediaDuration)
                {
                    // Loop or clamp at the end
                    if (loop)
                        mediaTime = Math::Mod(mediaTime, mediaDuration);
                    else
                        mediaTime = mediaDuration;
                }

                // Validate state data space
                if (stateIndexOffset + nestedAnim->TrackStatesCount > _tracks.Count())
                {
                    LOG(Warning,
                        "Not enough tracks state data buckets. Has {0} but need {1}. Animation {2} for nested track {3} on actor {4}.",
                        _tracks.Count(),
                        stateIndexOffset + nestedAnim->TrackStatesCount,
                        Animation.Get()->ToString(),
                        nestedAnim->ToString(),
                        ToString());
                    return;
                }

#if !BUILD_RELEASE || USE_EDITOR
                // Validate recursive call
                if (callStack.Contains(nestedAnim))
                {
                    LOG(Warning,
                        "Recursive nested scene animation. Animation {0} for nested track {1} on actor {2}.",
                        callStack.Last()->ToString(),
                        nestedAnim->ToString(),
                        ToString());
                    return;
                }
#endif

                Tick(nestedAnim, mediaTime, dt, stateIndexOffset + track.TrackStateIndex, callStack);
            }
            break;
        }
        case SceneAnimation::Track::Types::ScreenFade:
        {
            const auto trackData = track.GetData<SceneAnimation::ScreenFadeTrack::Data>();
            const float startTime = trackData->StartFrame / fps;
            const float durationTime = trackData->DurationFrames / fps;
            const float mediaTime = time - startTime;
            if (mediaTime >= 0.0f && mediaTime <= durationTime)
            {
                const auto runtimeData = track.GetRuntimeData<SceneAnimation::ScreenFadeTrack::Runtime>();
                _postFxSettings.CameraArtifacts.OverrideFlags |= CameraArtifactsSettings::Override::ScreenFadeColor;
                Color& color = _postFxSettings.CameraArtifacts.ScreenFadeColor;
                const int32 count = trackData->GradientStopsCount;
                const auto stops = runtimeData->GradientStops;
                if (mediaTime >= stops[count - 1].Frame / fps)
                {
                    // Outside the range
                    color = stops[count - 1].Value;
                }
                else
                {
                    // Find 2 samples to blend between them
                    float prevTime = stops[0].Frame / fps;
                    Color prevColor = stops[0].Value;
                    for (int32 i = 1; i < count; i++)
                    {
                        const float curTime = stops[i].Frame / fps;
                        const Color curColor = stops[i].Value;

                        if (mediaTime <= curTime)
                        {
                            color = Color::Lerp(prevColor, curColor, Math::Saturate((mediaTime - prevTime) / (curTime - prevTime)));
                            break;
                        }

                        prevTime = curTime;
                        prevColor = curColor;
                    }
                }
            }
            break;
        }
        case SceneAnimation::Track::Types::Audio:
        {
            const auto clip = track.Asset.As<AudioClip>();
            if (!clip || !clip->IsLoaded())
                break;
            const auto runtimeData = track.GetRuntimeData<SceneAnimation::AudioTrack::Runtime>();
            float mediaTime = -1, mediaDuration, playTime;
            for (int32 k = 0; k < runtimeData->Count; k++)
            {
                const auto& media = runtimeData->Media[k];
                const float startTime = media.StartFrame / fps;
                const float durationTime = media.DurationFrames / fps;
                if (Math::IsInRange(time, startTime, startTime + durationTime))
                {
                    mediaTime = time - startTime;
                    playTime = mediaTime + media.Offset;
                    mediaDuration = durationTime;
                    break;
                }
            }

            auto& state = _tracks[stateIndexOffset + track.TrackStateIndex];
            auto audioSource = state.Object.As<AudioSource>();
            if (mediaTime >= 0.0f && mediaTime <= mediaDuration)
            {
                const bool loop = ((int32)track.Flag & (int32)SceneAnimation::Track::Flags::Loop) == (int32)SceneAnimation::Track::Flags::Loop;
                if (!audioSource)
                {
                    // Spawn audio source to play the clip
                    audioSource = New<AudioSource>();
                    audioSource->SetStaticFlags(StaticFlags::None);
                    audioSource->HideFlags = HideFlags::FullyHidden;
                    audioSource->Clip = clip;
                    audioSource->SetIsLooping(loop);
                    audioSource->SetParent(this, false, false);
                    _subActors.Add(audioSource);
                    state.Object = audioSource;
                }

                // Sample volume track
                float volume = 1.0f;
                if (runtimeData->VolumeTrackIndex != -1)
                {
                    SceneAnimation::AudioVolumeTrack::CurveType volumeCurve(volume);
                    const auto volumeTrackRuntimeData = (anim->Tracks[runtimeData->VolumeTrackIndex].GetRuntimeData<SceneAnimation::AudioVolumeTrack::Runtime>());
                    if (volumeTrackRuntimeData)
                    {
                        SceneAnimation::AudioVolumeTrack::CurveType::KeyFrameData data(volumeTrackRuntimeData->Keyframes, volumeTrackRuntimeData->KeyframesCount);
                        const auto& firstMedia = runtimeData->Media[0];
                        auto firstMediaTime = time - firstMedia.StartFrame / fps;
                        volumeCurve.Evaluate(data, volume, firstMediaTime, false);
                    }
                }

                const float clipLength = clip->GetLength();
                if (loop)
                {
                    // Loop position
                    playTime = Math::Mod(playTime, clipLength);
                }
                else if (playTime >= clipLength)
                {
                    // Stop updating after end
                    break;
                }

                // Sync playback options
                audioSource->SetPitch(Speed);
                audioSource->SetVolume(volume);
#if USE_EDITOR
                // Sync more in editor for better changes preview
                audioSource->Clip = clip;
                audioSource->SetIsLooping(loop);
#endif

                // Synchronize playback position
                const float maxAudioLag = 0.3f;
                const auto audioTime = audioSource->GetTime();
                //LOG(Info, "Audio: {0}, Media : {1}", audioTime, playTime);
                if (Math::Abs(audioTime - playTime) > maxAudioLag &&
                    Math::Abs(audioTime + clipLength - playTime) > maxAudioLag &&
                    Math::Abs(playTime + clipLength - audioTime) > maxAudioLag)
                {
                    audioSource->SetTime(playTime);
                    //LOG(Info, "Set Time (current audio time: {0})", audioSource->GetTime());
                }

                // Keep playing
                if (_state == PlayState::Playing)
                    audioSource->Play();
                else
                    audioSource->Pause();
            }
            else if (audioSource)
            {
                // End playback
                audioSource->Stop();
            }
            break;
        }
        case SceneAnimation::Track::Types::AudioVolume:
        {
            // Audio track samples the volume curve itself
            break;
        }
        case SceneAnimation::Track::Types::Actor:
        {
            // Cache actor to animate
            auto& state = _tracks[stateIndexOffset + track.TrackStateIndex];
            if (!state.Object)
            {
                state.ManagedObject = nullptr;

                // Find actor
                const auto trackData = track.GetData<SceneAnimation::ActorTrack::Data>();
                Guid id = trackData->ID;
                _objectsMapping.TryGet(id, id);
                state.Object = Scripting::TryFindObject<Actor>(id);
                if (!state.Object)
                {
                    if (state.Warn)
                        LOG(Warning, "Failed to find {3} of ID={0} for track '{1}' in scene animation '{2}'", id, track.Name, anim->ToString(), TEXT("actor"));
                    state.Warn = false;
                    break;
                }
            }
            state.ManagedObject = state.Object.GetOrCreateManagedInstance();
            break;
        }
        case SceneAnimation::Track::Types::Script:
        {
            // Cache script to animate
            auto& state = _tracks[stateIndexOffset + track.TrackStateIndex];
            if (!state.Object)
            {
                state.ManagedObject = nullptr;

                // Skip if parent track actor is missing
                const auto trackData = track.GetData<SceneAnimation::ScriptTrack::Data>();
                ASSERT(track.ParentIndex != -1 && (anim->Tracks[track.ParentIndex].Type == SceneAnimation::Track::Types::Actor || anim->Tracks[track.ParentIndex].Type == SceneAnimation::Track::Types::CameraCut));
                const auto& parentTrack = anim->Tracks[track.ParentIndex];
                const auto& parentState = _tracks[stateIndexOffset + parentTrack.TrackStateIndex];
                const auto parentActor = parentState.Object.As<Actor>();
                if (parentActor == nullptr)
                    break;

                // Find script
                Guid id = trackData->ID;
                _objectsMapping.TryGet(id, id);
                state.Object = Scripting::TryFindObject<Script>(id);
                if (!state.Object)
                {
                    if (state.Warn)
                        LOG(Warning, "Failed to find {3} of ID={0} for track '{1}' in scene animation '{2}'", id, track.Name, anim->ToString(), TEXT("script"));
                    state.Warn = false;
                    break;
                }

                // Ensure script is linked to the parent track actor
                if (state.Object.As<Script>()->GetParent() != parentActor)
                {
                    LOG(Warning, "Found script {0} is not the parent of actor {3} for track '{1}' in scene animation '{2}'", state.Object->ToString(), track.Name, anim->ToString(), parentActor->ToString());
                    break;
                }
            }
            state.ManagedObject = state.Object.GetOrCreateManagedInstance();
            break;
        }
        case SceneAnimation::Track::Types::KeyframesProperty:
        case SceneAnimation::Track::Types::CurveProperty:
        case SceneAnimation::Track::Types::StringProperty:
        case SceneAnimation::Track::Types::ObjectReferenceProperty:
        case SceneAnimation::Track::Types::StructProperty:
        case SceneAnimation::Track::Types::ObjectProperty:
        {
            if (track.ParentIndex == -1)
                break;
            const auto runtimeData = track.GetRuntimeData<SceneAnimation::PropertyTrack::Runtime>();
            auto& state = _tracks[stateIndexOffset + track.TrackStateIndex];
            const auto& parentTrack = anim->Tracks[track.ParentIndex];

            // Structure property tracks evaluates the child tracks manually
            if (parentTrack.Type == SceneAnimation::Track::Types::StructProperty)
                break;

            // Skip if parent object is missing
            MObject* instance = _tracks[stateIndexOffset + parentTrack.TrackStateIndex].ManagedObject;
            if (!instance)
                break;

            // Cache property or field
            if (!state.Property && !state.Field)
            {
                MClass* mclass = MCore::Object::GetClass(instance);
                state.Property = mclass->GetProperty(runtimeData->PropertyName);
                if (!state.Property)
                {
                    state.Field = mclass->GetField(runtimeData->PropertyName);

                    // Skip if property and field are missing
                    if (!state.Field)
                        break;
                }
            }

            // Get stack memory for data value
            MType* valueType = state.Property ? state.Property->GetType() : state.Field->GetType();
            int32 valueSize = MCore::Type::GetSize(valueType);
            _tracksDataStack.AddDefault(valueSize);
            void* value = &_tracksDataStack[_tracksDataStack.Count() - valueSize];

            // Get the current value for the struct track so it can update only part of it or when need to capture the initial state for restore
            if (track.Type == SceneAnimation::Track::Types::StructProperty ||
                track.Type == SceneAnimation::Track::Types::ObjectProperty ||
                (RestoreStateOnStop && state.RestoreStateIndex == -1))
            {
                if (state.Property)
                {
                    MObject* exception = nullptr;
                    auto boxed = state.Property->GetValue(instance, &exception);
                    if (exception)
                    {
                        MException ex(exception);
                        ex.Log(LogType::Error, TEXT("Property"));
                    }
                    else if (!MCore::Type::IsPointer(valueType) && !MCore::Type::IsReference(valueType))
                    {
                        if (boxed)
                            Platform::MemoryCopy(value, MCore::Object::Unbox(boxed), valueSize);
                        else
                            Platform::MemoryClear(value, valueSize);
                    }
                    else
                    {
                        *(MObject**)value = boxed;
                    }
                }
                else
                {
                    state.Field->GetValue(instance, value);
                }

                // Cache the initial state of the track property
                if (RestoreStateOnStop && state.RestoreStateIndex == -1)
                {
                    state.RestoreStateIndex = _restoreData.Count();
                    switch (track.Type)
                    {
                    case SceneAnimation::Track::Types::StringProperty:
                    {
                        StringView str;
                        MUtils::ToString(*(MString**)value, str);
                        _restoreData.Add((byte*)str.Get(), str.Length());
                        _restoreData.Add('\0');
                        break;
                    }
                    case SceneAnimation::Track::Types::ObjectReferenceProperty:
                    {
                        auto obj = Scripting::FindObject(*(MObject**)value);
                        auto id = obj ? obj->GetID() : Guid::Empty;
                        _restoreData.Add((byte*)&id, sizeof(Guid));
                        break;
                    }
                    case SceneAnimation::Track::Types::ObjectProperty:
                        break;
                    default:
                        _restoreData.Add((byte*)value, valueSize);
                        break;
                    }
                }
            }

            // Sample track
            if (TickPropertyTrack(j, stateIndexOffset, anim, time, track, state, value))
            {
                // Set the value
                auto valueTypes = MCore::Type::GetType(valueType);
                if (valueTypes == MTypes::Object || MCore::Type::IsPointer(valueType))
                    value = (void*)*(intptr*)value;
                if (state.Property)
                {
                    MObject* exception = nullptr;
                    state.Property->SetValue(instance, value, &exception);
                    if (exception)
                    {
                        MException ex(exception);
                        ex.Log(LogType::Error, TEXT("Property"));
                    }
                }
                else
                {
                    state.Field->SetValue(instance, value);
                }
            }

            // Free stack memory
            _tracksDataStack.Resize(_tracksDataStack.Count() - valueSize);

            break;
        }
        case SceneAnimation::Track::Types::Event:
        {
            if (track.ParentIndex == -1)
                break;
            const auto runtimeData = track.GetRuntimeData<SceneAnimation::EventTrack::Runtime>();
            void* paramsData[SceneAnimation::EventTrack::MaxParams];

            // Check if hit any event key since the last update
            float lastTime = time - dt;
            float minTime, maxTime;
            if (dt > 0)
            {
                minTime = lastTime;
                maxTime = time;
            }
            else
            {
                minTime = time;
                maxTime = lastTime;
            }
            int32 eventsCount = runtimeData->EventsCount;
            int32 paramsSize = runtimeData->EventParamsSize;
            const byte* ptr = runtimeData->DataBegin;
            int32 eventIndex = 0;
            for (; eventIndex < eventsCount; eventIndex++)
            {
                float eventTime = *(float*)ptr;
                if (Math::IsInRange(eventTime, minTime, maxTime))
                {
                    // Prepare parameters
                    ptr += sizeof(float);
                    for (int32 paramIndex = 0; paramIndex < runtimeData->EventParamsCount; paramIndex++)
                    {
                        paramsData[paramIndex] = (void*)ptr;
                        ptr += runtimeData->EventParamSizes[paramIndex];
                    }

                    auto& state = _tracks[stateIndexOffset + track.TrackStateIndex];
                    const auto& parentTrack = anim->Tracks[track.ParentIndex];
                    auto& parentTrackState = _tracks[stateIndexOffset + parentTrack.TrackStateIndex];
                    if (parentTrackState.ManagedObject)
                    {
                        auto instance = parentTrackState.ManagedObject;

                        // Cache method
                        if (!state.Method)
                        {
                            state.Method = MCore::Object::GetClass(instance)->FindMethod(runtimeData->EventName, runtimeData->EventParamsCount);
                            if (!state.Method)
                                break;
                        }

                        // Invoke the method
                        Variant result;
                        MObject* exception = nullptr;
                        state.Method->Invoke(instance, paramsData, &exception);
                        if (exception)
                        {
                            MException ex(exception);
                            ex.Log(LogType::Error, TEXT("Event"));
                        }
                    }
                }
                else
                {
                    ptr += sizeof(float) + paramsSize;
                }
            }
            break;
        }
        case SceneAnimation::Track::Types::CameraCut:
        {
            // Check if any camera cut media on a track is active
            bool isActive = false;
            const auto runtimeData = track.GetRuntimeData<SceneAnimation::CameraCutTrack::Runtime>();
            for (int32 k = 0; k < runtimeData->Count; k++)
            {
                const auto& media = runtimeData->Media[k];
                const float startTime = media.StartFrame / fps;
                const float durationTime = media.DurationFrames / fps;
                if (Math::IsInRange(time, startTime, startTime + durationTime))
                {
                    isActive = true;
                    break;
                }
            }
            if (!isActive)
            {
                // Skip updating child tracks if the current position is outside the media clip range
                j += track.ChildrenCount;
                break;
            }

            // Cache actor to animate
            const auto trackData = track.GetData<SceneAnimation::CameraCutTrack::Data>();
            auto& state = _tracks[stateIndexOffset + track.TrackStateIndex];
            if (!state.Object)
            {
                state.ManagedObject = nullptr;

                // Find actor
                Guid id = trackData->ID;
                _objectsMapping.TryGet(id, id);
                state.Object = Scripting::TryFindObject<Camera>(id);
                if (!state.Object)
                {
                    if (state.Warn)
                        LOG(Warning, "Failed to find {3} of ID={0} for track '{1}' in scene animation '{2}'", id, track.Name, anim->ToString(), TEXT("camera"));
                    state.Warn = false;
                    break;
                }
            }
            state.ManagedObject = state.Object.GetOrCreateManagedInstance();

            // Override camera
            if (_cameraCutCam == nullptr)
            {
                _cameraCutCam = (Camera*)state.Object.Get();
                _isUsingCameraCuts = true;
            }
            break;
        }
        default: ;
        }
    }
#endif
}

void SceneAnimationPlayer::Tick()
{
    if (UpdateMode == UpdateModes::Manual)
        return;

    float dt = 0.0f;
    if (Math::NearEqual(_lastTime, _time))
    {
        // Delta time animation
        const auto& tickData = Time::UPDATE_POINT;
        const TimeSpan deltaTime = UseTimeScale ? tickData.DeltaTime : tickData.UnscaledDeltaTime;
        dt = static_cast<float>(deltaTime.GetTotalSeconds()) * Speed;
    }

    Tick(dt);
}

void SceneAnimationPlayer::OnAnimationModified()
{
    _restoreData.Resize(0);
    Stop();
    ResetState();
}

void SceneAnimationPlayer::ResetState()
{
    for (auto actor : _subActors)
        actor->DeleteObject();
    _subActors.Resize(0);
    _tracks.Resize(0);
    _restoreData.Resize(0);
}

bool SceneAnimationPlayer::HasContentLoaded() const
{
    return Animation == nullptr || Animation->IsLoaded();
}

void SceneAnimationPlayer::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    Actor::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(SceneAnimationPlayer);

    SERIALIZE(Animation);
    SERIALIZE(Speed);
    SERIALIZE(StartTime);
    SERIALIZE(UseTimeScale);
    SERIALIZE(Loop);
    SERIALIZE(PlayOnStart);
    SERIALIZE(RandomStartTime);
    SERIALIZE(RestoreStateOnStop);
    SERIALIZE(UpdateMode);
    SERIALIZE(UsePrefabObjects);
}

void SceneAnimationPlayer::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    Actor::Deserialize(stream, modifier);

    DESERIALIZE(Animation);
    DESERIALIZE(Speed);
    DESERIALIZE(StartTime);
    DESERIALIZE(UseTimeScale);
    DESERIALIZE(Loop);
    DESERIALIZE(PlayOnStart);
    DESERIALIZE(RandomStartTime);
    DESERIALIZE(RestoreStateOnStop);
    DESERIALIZE(UpdateMode);
    DESERIALIZE(UsePrefabObjects);

    if (UsePrefabObjects && Animation && !Animation->WaitForLoaded())
    {
        // When loading from prefab automatically map objects from prefab instance into animation tracks with object references
        for (auto& track : Animation->Tracks)
        {
            if (track.Disabled || !(track.Flag & SceneAnimation::Track::Flags::PrefabObject))
                continue;
            switch (track.Type)
            {
            case SceneAnimation::Track::Types::Actor:
            case SceneAnimation::Track::Types::Script:
            case SceneAnimation::Track::Types::CameraCut:
            {
                const auto trackData = track.GetData<SceneAnimation::ObjectTrack::Data>();
                Guid id;
                if (modifier->IdsMapping.TryGet(trackData->ID, id))
                    _objectsMapping[trackData->ID] = id;
                break;
            }
            }
        }
    }
}

void SceneAnimationPlayer::Collect(RenderContext& renderContext)
{
    if (!IsDuringPlay() || !IsActiveInHierarchy() || _state == PlayState::Stopped)
        return;

    renderContext.List->AddSettingsBlend(this, 1.0f, 100000000, 1.0f);
}

void SceneAnimationPlayer::Blend(PostProcessSettings& other, float weight)
{
    other.CameraArtifacts.BlendWith(_postFxSettings.CameraArtifacts, weight);
    other.PostFxMaterials.BlendWith(_postFxSettings.PostFxMaterials, weight);
}

void SceneAnimationPlayer::BeginPlay(SceneBeginData* data)
{
    // Base
    Actor::BeginPlay(data);

    if (IsActiveInHierarchy() && PlayOnStart)
    {
#if USE_EDITOR
        if (Time::GetGamePaused())
            return;
#endif
        Play();
    }
}

void SceneAnimationPlayer::EndPlay()
{
    Stop();
    ResetState();

    // Base
    Actor::EndPlay();
}

void SceneAnimationPlayer::OnEnable()
{
    if (_state == PlayState::Playing)
    {
        REGISTER_TICK;
    }
#if USE_EDITOR
    GetSceneRendering()->AddViewportIcon(this);
#endif

    // Base
    Actor::OnEnable();
}

void SceneAnimationPlayer::OnDisable()
{
    if (_state == PlayState::Playing)
    {
        UNREGISTER_TICK;
    }
#if USE_EDITOR
    GetSceneRendering()->RemoveViewportIcon(this);
#endif

    // Base
    Actor::OnDisable();
}

void SceneAnimationPlayer::OnTransformChanged()
{
    // Base
    Actor::OnTransformChanged();

    _box = BoundingBox(_transform.Translation);
    _sphere = BoundingSphere(_transform.Translation, 0.0f);
}
