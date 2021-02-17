// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "SceneAnimationPlayer.h"
#include "Engine/Core/Random.h"
#include "Engine/Engine/Time.h"
#include "Engine/Level/Scene/Scene.h"
#include "Engine/Level/SceneObjectsFactory.h"
#include "Engine/Level/Actors/Camera.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Audio/AudioClip.h"
#include "Engine/Audio/AudioSource.h"
#include "Engine/Renderer/RenderList.h"
#include "Engine/Scripting/Scripting.h"
#include "Engine/Scripting/Script.h"
#include "Engine/Scripting/MException.h"
#include "Engine/Scripting/ManagedCLR/MProperty.h"
#include "Engine/Scripting/ManagedCLR/MUtils.h"
#include "Engine/Scripting/ManagedCLR/MType.h"
#include "Engine/Scripting/ManagedCLR/MField.h"
#include "Engine/Scripting/ManagedCLR/MClass.h"
#include "Engine/Scripting/ManagedCLR/MMethod.h"

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

    if (IsActiveInHierarchy() && _state == PlayState::Playing)
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
    CHECK(IsActiveInHierarchy() && _state == PlayState::Playing);

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
    Tick(anim, time, dt, 0);
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

void SceneAnimationPlayer::Restore(SceneAnimation* anim, int32 stateIndexOffset)
{
    // Restore all tracks
    for (int32 j = 0; j < anim->Tracks.Count(); j++)
    {
        const auto& track = anim->Tracks[j];
        if (track.Disabled)
            continue;
        switch (track.Type)
        {
        case SceneAnimation::Track::Types::Actor:
        case SceneAnimation::Track::Types::CameraCut:
        {
            auto& state = _tracks[stateIndexOffset + track.TrackStateIndex];
            state.ManagedObject = state.Object ? state.Object.GetOrCreateManagedInstance() : nullptr;
            break;
        }
        case SceneAnimation::Track::Types::Script:
        {
            auto& state = _tracks[stateIndexOffset + track.TrackStateIndex];
            state.ManagedObject = state.Object ? state.Object.GetOrCreateManagedInstance() : nullptr;
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
            MonoObject* instance = _tracks[stateIndexOffset + parentTrack.TrackStateIndex].ManagedObject;
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
                    MonoObject* exception = nullptr;
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
                MonoObject* exception = nullptr;
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
}

bool SceneAnimationPlayer::TickPropertyTrack(int32 trackIndex, int32 stateIndexOffset, SceneAnimation* anim, float time, const SceneAnimation::Track& track, TrackInstance& state, void* target)
{
    switch (track.Type)
    {
    case SceneAnimation::Track::Types::KeyframesProperty:
    case SceneAnimation::Track::Types::ObjectReferenceProperty:
    {
        const auto trackDataKeyframes = track.GetRuntimeData<SceneAnimation::KeyframesPropertyTrack::Runtime>();
        const int32 count = trackDataKeyframes->KeyframesCount;
        if (count == 0)
            return false;

        // Find the keyframe at time
        int32 keyframeSize = sizeof(float) + trackDataKeyframes->ValueSize;
#define GET_KEY_TIME(idx) *(float*)((byte*)trackDataKeyframes->Keyframes + keyframeSize * (idx))
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
        void* value = (void*)((byte*)trackDataKeyframes->Keyframes + keyframeSize * (leftKey) + sizeof(float));
        if (track.Type == SceneAnimation::Track::Types::ObjectReferenceProperty)
        {
            Guid id = *(Guid*)value;
            _objectsMapping.TryGet(id, id);
            auto obj = Scripting::FindObject<ScriptingObject>(id);
            value = obj ? obj->GetOrCreateManagedInstance() : nullptr;
            *(void**)target = value;
        }
        else
        {
            Platform::MemoryCopy(target, value, trackDataKeyframes->ValueSize);
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
        switch (trackDataKeyframes->DataType)
        {
#define IMPL_CURVE(dataType, valueType) \
    case SceneAnimation::CurvePropertyTrack::DataTypes::dataType: \
    { \
        CurveBase<valueType, BezierCurveKeyframe<valueType>> volumeCurve; \
        CurveBase<valueType, BezierCurveKeyframe<valueType>>::KeyFrameData data((BezierCurveKeyframe<valueType>*)trackDataKeyframes->Keyframes, trackDataKeyframes->KeyframesCount); \
        volumeCurve.Evaluate(data, *(valueType*)target, time, false); \
        break; \
    }
        IMPL_CURVE(Float, float);
        IMPL_CURVE(Double, double);
        IMPL_CURVE(Vector2, Vector2);
        IMPL_CURVE(Vector3, Vector3);
        IMPL_CURVE(Vector4, Vector4);
        IMPL_CURVE(Quaternion, Quaternion);
        IMPL_CURVE(Color, Color);
        IMPL_CURVE(Color32, Color32);
#undef IMPL_CURVE
        default: ;
        }
        break;
    }
    case SceneAnimation::Track::Types::StringProperty:
    {
        const auto trackDataKeyframes = track.GetRuntimeData<SceneAnimation::StringPropertyTrack::Runtime>();
        const int32 count = trackDataKeyframes->KeyframesCount;
        if (count == 0)
            return false;
        const auto keyframesTimes = (float*)((byte*)trackDataKeyframes + sizeof(SceneAnimation::StringPropertyTrack::Runtime));
        const auto keyframesLengths = (int32*)((byte*)keyframesTimes + sizeof(float) * trackDataKeyframes->KeyframesCount);
        const auto keyframesValues = (Char**)((byte*)keyframesLengths + sizeof(int32) * trackDataKeyframes->KeyframesCount);

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
        *(MonoString**)target = MUtils::ToString(str);
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
            const auto childTrackRuntimeData = childTrack.GetRuntimeData<SceneAnimation::PropertyTrack::Runtime>();
            auto& childTrackState = _tracks[stateIndexOffset + childTrack.TrackStateIndex];

            // Cache field
            if (!childTrackState.Field)
            {
                MType type = state.Property ? state.Property->GetType() : (state.Field ? state.Field->GetType() : MType());
                if (!type)
                    continue;
                MClass* mclass = Scripting::FindClass(mono_type_get_class(type.GetNative()));
                childTrackState.Field = mclass->GetField(childTrackRuntimeData->PropertyName);
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
        state.ManagedObject = *(MonoObject**)target;
        return false;
    }
    default: ;
    }

    return true;
}

void SceneAnimationPlayer::Tick(SceneAnimation* anim, float time, float dt, int32 stateIndexOffset)
{
    const float fps = anim->FramesPerSecond;

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
            const auto trackData = track.GetData<SceneAnimation::PostProcessMaterialTrack::Data>();
            const float startTime = trackData->StartFrame / fps;
            const float durationTime = trackData->DurationFrames / fps;
            const bool isActive = Math::IsInRange(time, startTime, startTime + durationTime);
            if (isActive && _postFxSettings.PostFxMaterials.Materials.Count() < POST_PROCESS_SETTINGS_MAX_MATERIALS)
            {
                _postFxSettings.PostFxMaterials.Materials.Add(track.Asset.As<MaterialBase>());
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
                stateIndexOffset += track.TrackStateIndex;
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

                Tick(nestedAnim, mediaTime, dt, stateIndexOffset);
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
            const auto trackData = track.GetData<SceneAnimation::AudioTrack::Data>();
            const auto runtimeData = track.GetRuntimeData<SceneAnimation::AudioTrack::Runtime>();
            const float startTime = trackData->StartFrame / fps;
            const float durationTime = trackData->DurationFrames / fps;
            const bool loop = ((int32)track.Flag & (int32)SceneAnimation::Track::Flags::Loop) == (int32)SceneAnimation::Track::Flags::Loop;
            float mediaTime = time - startTime;
            auto& state = _tracks[stateIndexOffset + track.TrackStateIndex];
            auto audioSource = state.Object.As<AudioSource>();
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

            if (mediaTime >= 0.0f && mediaTime <= durationTime)
            {
                // Sample volume track
                float volume = 1.0f;
                if (runtimeData->VolumeTrackIndex != -1)
                {
                    SceneAnimation::AudioVolumeTrack::CurveType volumeCurve(volume);
                    const auto volumeTrackRuntimeData = (anim->Tracks[runtimeData->VolumeTrackIndex].GetRuntimeData<SceneAnimation::AudioVolumeTrack::Runtime>());
                    if (volumeTrackRuntimeData)
                    {
                        SceneAnimation::AudioVolumeTrack::CurveType::KeyFrameData data(volumeTrackRuntimeData->Keyframes, volumeTrackRuntimeData->KeyframesCount);
                        volumeCurve.Evaluate(data, volume, mediaTime, false);
                    }
                }

                const float clipLength = clip->GetLength();
                if (loop)
                {
                    // Loop position
                    mediaTime = Math::Mod(mediaTime, clipLength);
                }
                else if (mediaTime >= clipLength)
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
                //LOG(Info, "Audio: {0}, Media : {1}", audioTime, mediaTime);
                if (Math::Abs(audioTime - mediaTime) > maxAudioLag &&
                    Math::Abs(audioTime + clipLength - mediaTime) > maxAudioLag &&
                    Math::Abs(mediaTime + clipLength - audioTime) > maxAudioLag)
                {
                    audioSource->SetTime(mediaTime);
                    //LOG(Info, "Set Time (current audio time: {0})", audioSource->GetTime());
                }

                // Keep playing
                audioSource->Play();
            }
            else
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
                state.Object = Scripting::FindObject<Actor>(trackData->ID);
                if (!state.Object)
                {
                    LOG(Warning, "Failed to find {3} of ID={0} for track '{1}' in scene animation '{2}'", id, track.Name, anim->ToString(), TEXT("actor"));
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
                state.Object = Scripting::FindObject<Script>(id);
                if (!state.Object)
                {
                    LOG(Warning, "Failed to find {3} of ID={0} for track '{1}' in scene animation '{2}'", id, track.Name, anim->ToString(), TEXT("script"));
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
            MonoObject* instance = _tracks[stateIndexOffset + parentTrack.TrackStateIndex].ManagedObject;
            if (!instance)
                break;

            // Cache property or field
            if (!state.Property && !state.Field)
            {
                MClass* mclass = Scripting::FindClass(mono_object_get_class(instance));
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
            MType valueType = state.Property ? state.Property->GetType() : state.Field->GetType();
            int32 valueAlignment;
            int32 valueSize = mono_type_stack_size(valueType.GetNative(), &valueAlignment);
            _tracksDataStack.AddDefault(valueSize);
            void* value = &_tracksDataStack[_tracksDataStack.Count() - valueSize];

            // Get the current value for the struct track so it can update only part of it or when need to capture the initial state for restore
            if (track.Type == SceneAnimation::Track::Types::StructProperty ||
                track.Type == SceneAnimation::Track::Types::ObjectProperty ||
                (RestoreStateOnStop && state.RestoreStateIndex == -1))
            {
                if (state.Property)
                {
                    MonoObject* exception = nullptr;
                    auto boxed = state.Property->GetValue(instance, &exception);
                    if (exception)
                    {
                        MException ex(exception);
                        ex.Log(LogType::Error, TEXT("Property"));
                    }
                    else if (!valueType.IsPointer())
                    {
                        if (boxed)
                            Platform::MemoryCopy(value, mono_object_unbox(boxed), valueSize);
                        else
                            Platform::MemoryClear(value, valueSize);
                    }
                    else
                    {
                        *(MonoObject**)value = (MonoObject*)boxed;
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
                        MUtils::ToString(*(MonoString**)value, str);
                        _restoreData.Add((byte*)str.Get(), str.Length());
                        _restoreData.Add('\0');
                        break;
                    }
                    case SceneAnimation::Track::Types::ObjectReferenceProperty:
                    {
                        auto obj = Scripting::FindObject(*(MonoObject**)value);
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
                if (valueType.IsPointer())
                    value = (void*)*(intptr*)value;
                if (state.Property)
                {
                    MonoObject* exception = nullptr;
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
            auto& state = _tracks[stateIndexOffset + track.TrackStateIndex];
            const auto& parentTrack = anim->Tracks[track.ParentIndex];

            // Skip if parent object is missing
            MonoObject* instance = _tracks[stateIndexOffset + parentTrack.TrackStateIndex].ManagedObject;
            if (!instance)
                break;

            // Cache method
            if (!state.Method)
            {
                MClass* mclass = Scripting::FindClass(mono_object_get_class(instance));
                state.Method = mclass->GetMethod(runtimeData->EventName, runtimeData->EventParamsCount);

                // Skip if method is missing
                if (!state.Method)
                    break;
            }

            void* params[SceneAnimation::EventTrack::MaxParams];

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
                        params[paramIndex] = (void*)ptr;
                        ptr += runtimeData->EventParamSizes[paramIndex];
                    }

                    // Invoke the method
                    MonoObject* exception = nullptr;
                    // TODO: use method thunk
                    state.Method->Invoke(instance, params, &exception);
                    if (exception)
                    {
                        MException ex(exception);
                        ex.Log(LogType::Error, TEXT("Event"));
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
            // Cache actor to animate
            const auto trackData = track.GetData<SceneAnimation::CameraCutTrack::Data>();
            auto& state = _tracks[stateIndexOffset + track.TrackStateIndex];
            if (!state.Object)
            {
                state.ManagedObject = nullptr;

                // Find actor
                Guid id = trackData->ID;
                _objectsMapping.TryGet(id, id);
                state.Object = Scripting::FindObject<Camera>(id);
                if (!state.Object)
                {
                    LOG(Warning, "Failed to find {3} of ID={0} for track '{1}' in scene animation '{2}'", id, track.Name, anim->ToString(), TEXT("actor"));
                    break;
                }
            }
            state.ManagedObject = state.Object.GetOrCreateManagedInstance();

            // Use camera
            _isUsingCameraCuts = true;
            const float startTime = trackData->StartFrame / fps;
            const float durationTime = trackData->DurationFrames / fps;
            float mediaTime = time - startTime;
            if (mediaTime >= 0.0f && mediaTime <= durationTime)
            {
                if (_cameraCutCam == nullptr)
                {
                    // Override camera
                    _cameraCutCam = (Camera*)state.Object.Get();
                }
            }
            else
            {
                // Skip updating child tracks if the current position is outside the media clip range
                j += track.ChildrenCount;
            }
            break;
        }
        default: ;
        }
    }
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

    _box = BoundingBox(_transform.Translation, _transform.Translation);
    _sphere = BoundingSphere(_transform.Translation, 0.0f);
}
