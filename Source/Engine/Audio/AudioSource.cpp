// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "AudioSource.h"
#include "Engine/Core/Log.h"
#include "Engine/Level/SceneObjectsFactory.h"
#include "Engine/Serialization/Serialization.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Engine/Time.h"
#include "Engine/Level/Scene/Scene.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "AudioBackend.h"
#include "Audio.h"

AudioSource::AudioSource(const SpawnParams& params)
    : Actor(params)
    , _velocity(Vector3::Zero)
    , _volume(1.0f)
    , _pitch(1.0f)
    , _minDistance(1000.0f)
    , _loop(false)
    , _playOnStart(false)
    , _startTime(0.0f)
    , _allowSpatialization(true)
{
    Clip.Changed.Bind<AudioSource, &AudioSource::OnClipChanged>(this);
    Clip.Loaded.Bind<AudioSource, &AudioSource::OnClipLoaded>(this);
}

void AudioSource::SetVolume(float value)
{
    value = Math::Saturate(value);
    if (Math::NearEqual(_volume, value))
        return;
    _volume = value;
    if (SourceIDs.HasItems())
        AudioBackend::Source::VolumeChanged(this);
}

void AudioSource::SetPitch(float value)
{
    value = Math::Clamp(value, 0.5f, 2.0f);
    if (Math::NearEqual(_pitch, value))
        return;
    _pitch = value;
    if (SourceIDs.HasItems())
        AudioBackend::Source::PitchChanged(this);
}

void AudioSource::SetPan(float value)
{
    value = Math::Clamp(value, -1.0f, 1.0f);
    if (Math::NearEqual(_pan, value))
        return;
    _pan = value;
    if (SourceIDs.HasItems())
        AudioBackend::Source::PanChanged(this);
}

void AudioSource::SetIsLooping(bool value)
{
    if (_loop == value)
        return;
    _loop = value;

    // When streaming we handle looping manually by the proper buffers submission
    if (SourceIDs.HasItems() && !UseStreaming())
        AudioBackend::Source::IsLoopingChanged(this);
}

void AudioSource::SetPlayOnStart(bool value)
{
    _playOnStart = value;
}

void AudioSource::SetStartTime(float value)
{
    _startTime = value;
}

void AudioSource::SetMinDistance(float value)
{
    value = Math::Max(0.0f, value);
    if (Math::NearEqual(_minDistance, value))
        return;
    _minDistance = value;
    if (SourceIDs.HasItems())
        AudioBackend::Source::SpatialSetupChanged(this);
}

void AudioSource::SetAttenuation(float value)
{
    value = Math::Max(0.0f, value);
    if (Math::NearEqual(_attenuation, value))
        return;
    _attenuation = value;
    if (SourceIDs.HasItems())
        AudioBackend::Source::SpatialSetupChanged(this);
}

void AudioSource::SetDopplerFactor(float value)
{
    value = Math::Max(0.0f, value);
    if (Math::NearEqual(_dopplerFactor, value))
        return;
    _dopplerFactor = value;
    if (SourceIDs.HasItems())
        AudioBackend::Source::SpatialSetupChanged(this);
}

void AudioSource::SetAllowSpatialization(bool value)
{
    if (_allowSpatialization == value)
        return;
    _allowSpatialization = value;
    if (SourceIDs.HasItems())
        AudioBackend::Source::SpatialSetupChanged(this);
}

void AudioSource::Play()
{
    auto state = _state;
    if (state == States::Playing)
        return;
    if (Clip == nullptr)
    {
        LOG(Warning, "Cannot play audio source without a clip ({0})", GetNamePath());
        return;
    }

    _state = States::Playing;
    _isActuallyPlayingSth = false;

    // Don't block scripting if audio is not loaded or has missing streaming data
    if (!Clip->IsLoaded())
        return;

    // Audio clips with disabled streaming are controlled by audio source, otherwise streaming manager will play it
    if (Clip->IsStreamable())
    {
        if (state == States::Paused)
        {
            // Resume
            PlayInternal();
        }
        else
        {
            // Request faster streaming update
            Clip->RequestStreamingUpdate();

            // If we are looping and streaming also update streaming buffers
            if (_loop || state == States::Stopped)
                RequestStreamingBuffersUpdate();
        }
    }
    else if (SourceIDs.HasItems())
    {
        // Play it right away
        SetNonStreamingBuffer();
        PlayInternal();
    }
    else
    {
        // Source was nt properly added to the Audio Backend
        LOG(Warning, "Cannot play unitialized audio source.");
    }
}

void AudioSource::Pause()
{
    if (_state != States::Playing)
        return;

    _state = States::Paused;

    if (_isActuallyPlayingSth)
    {
        AudioBackend::Source::Pause(this);
        _isActuallyPlayingSth = false;
    }
}

void AudioSource::Stop()
{
    if (_state == States::Stopped)
        return;

    _state = States::Stopped;
    _isActuallyPlayingSth = false;
    _streamingFirstChunk = 0;

    if (SourceIDs.HasItems())
        AudioBackend::Source::Stop(this);
}

float AudioSource::GetTime() const
{
    if (_state == States::Stopped || SourceIDs.IsEmpty() || !Clip->IsLoaded())
        return 0.0f;

    float time = AudioBackend::Source::GetCurrentBufferTime(this);

    if (UseStreaming())
    {
        // Apply time offset to the first streaming buffer binded to the source including the already queued buffers
        int32 numProcessedBuffers = 0;
        AudioBackend::Source::GetProcessedBuffersCount(const_cast<AudioSource*>(this), numProcessedBuffers);
        time += Clip->GetBufferStartTime(_streamingFirstChunk + numProcessedBuffers);
    }

    time = Math::Clamp(time, 0.0f, Clip->GetLength());

    return time;
}

void AudioSource::SetTime(float time)
{
    if (_state == States::Stopped)
        return;

    const bool isActuallyPlayingSth = _isActuallyPlayingSth;
    const auto state = _state;

    // Audio backend can perform seek operation if audio is not playing
    if (isActuallyPlayingSth)
    {
        Stop();
    }

    if (UseStreaming())
    {
        // Update the first audio clip chunk to use for streaming and peek the relative time offset from the chunk start
        float relativeTime = 0;
        _streamingFirstChunk = Clip->GetFirstBufferIndex(time, relativeTime);
        ASSERT(_streamingFirstChunk < Clip->Buffers.Count());
        time = relativeTime;
    }

    AudioBackend::Source::SetCurrentBufferTime(this, time);

    // Restore state if was stopped
    if (isActuallyPlayingSth)
    {
        if (state != States::Stopped)
            Play();
        if (state == States::Paused)
            Pause();
    }
}

bool AudioSource::Is3D() const
{
    if (Clip == nullptr || Clip->WaitForLoaded())
        return false;
    return _allowSpatialization && Clip->Is3D();
}

void AudioSource::RequestStreamingBuffersUpdate()
{
    _needToUpdateStreamingBuffers = true;
}

void AudioSource::Cleanup()
{
    _savedState = GetState();
    _savedTime = GetTime();
    Stop();

    if (SourceIDs.HasItems())
    {
        AudioBackend::Source::Cleanup(this);
        SourceIDs.Clear();
    }
}

void AudioSource::OnClipChanged()
{
    Stop();
    _clipChanged = true;
}

void AudioSource::OnClipLoaded()
{
    AudioBackend::Source::ClipLoaded(this);

    // Start playing if source was waiting for the clip to load
    if (SourceIDs.HasItems() && _state == States::Playing && !_isActuallyPlayingSth)
    {
        if (Clip->IsStreamable())
        {
            // Request faster streaming update
            Clip->RequestStreamingUpdate();
        }
        else
        {
            // Play it right away
            SetNonStreamingBuffer();
            PlayInternal();
        }
    }
}

bool AudioSource::UseStreaming() const
{
    return Clip && Clip->IsLoaded() && Clip->IsStreamable();
}

void AudioSource::Restore()
{
    if (Clip)
    {
        if (_savedState != States::Stopped)
            Play();
        if (_savedState == States::Paused)
            Pause();

        SetTime(_savedTime);

        if (_savedState != States::Stopped && UseStreaming())
            RequestStreamingBuffersUpdate();
    }
}

void AudioSource::SetNonStreamingBuffer()
{
    ASSERT(Clip && !Clip->IsStreamable());

    AudioBackend::Source::SetNonStreamingBuffer(this);
}

void AudioSource::PlayInternal()
{
    if (_clipChanged && SourceIDs.HasItems())
    {
        // If clip was changed between source setup (OnEnable) and actual playback start then ensure to flush any runtime properties with the audio backend
        _clipChanged = false;
        AudioBackend::Source::SpatialSetupChanged(this);
    }
    AudioBackend::Source::Play(this);

    _isActuallyPlayingSth = true;
    _startingToPlay = true;
}

#if USE_EDITOR

#include "Engine/Debug/DebugDraw.h"

void AudioSource::OnDebugDrawSelected()
{
    // Draw influence range
    if (_allowSpatialization)
        DEBUG_DRAW_WIRE_SPHERE(BoundingSphere(_transform.Translation, _minDistance), Color::CornflowerBlue, 0, true);

    // Base
    Actor::OnDebugDrawSelected();
}

#endif

void AudioSource::Serialize(SerializeStream& stream, const void* otherObj)
{
    // Base
    Actor::Serialize(stream, otherObj);

    SERIALIZE_GET_OTHER_OBJ(AudioSource);

    SERIALIZE(Clip);
    SERIALIZE_MEMBER(Volume, _volume);
    SERIALIZE_MEMBER(Pitch, _pitch);
    SERIALIZE_MEMBER(Pan, _pan);
    SERIALIZE_MEMBER(MinDistance, _minDistance);
    SERIALIZE_MEMBER(Attenuation, _attenuation);
    SERIALIZE_MEMBER(DopplerFactor, _dopplerFactor);
    SERIALIZE_MEMBER(Loop, _loop);
    SERIALIZE_MEMBER(PlayOnStart, _playOnStart);
    SERIALIZE_MEMBER(StartTime, _startTime);
    SERIALIZE_MEMBER(AllowSpatialization, _allowSpatialization);
}

void AudioSource::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
{
    // Base
    Actor::Deserialize(stream, modifier);

    DESERIALIZE_MEMBER(Volume, _volume);
    DESERIALIZE_MEMBER(Pitch, _pitch);
    DESERIALIZE_MEMBER(Pan, _pan);
    DESERIALIZE_MEMBER(MinDistance, _minDistance);
    DESERIALIZE_MEMBER(Attenuation, _attenuation);
    DESERIALIZE_MEMBER(DopplerFactor, _dopplerFactor);
    DESERIALIZE_MEMBER(Loop, _loop);
    DESERIALIZE_MEMBER(PlayOnStart, _playOnStart);
    DESERIALIZE_MEMBER(StartTime, _startTime);
    DESERIALIZE_MEMBER(AllowSpatialization, _allowSpatialization);
    DESERIALIZE(Clip);
}

bool AudioSource::HasContentLoaded() const
{
    return Clip == nullptr || Clip->IsLoaded();
}

bool AudioSource::IntersectsItself(const Ray& ray, Real& distance, Vector3& normal)
{
    return false;
}

void AudioSource::Update()
{
    PROFILE_CPU();

    // Update the velocity
    const Vector3 pos = GetPosition();
    const float dt = Math::Max(Time::Update.UnscaledDeltaTime.GetTotalSeconds(), ZeroTolerance);
    const auto prevVelocity = _velocity;
    _velocity = (pos - _prevPos) / dt;
    _prevPos = pos;
    if (_velocity != prevVelocity)
    {
        AudioBackend::Source::VelocityChanged(this);
    }

    // Reset starting to play value once time is greater than zero
    if (_startingToPlay && GetTime() > 0.0f)
    {
        _startingToPlay = false;
    }

    if (!UseStreaming() && Math::NearEqual(GetTime(), 0.0f) && _isActuallyPlayingSth && !_startingToPlay)
    {
        int32 queuedBuffers;
        AudioBackend::Source::GetQueuedBuffersCount(this, queuedBuffers);
        if (queuedBuffers)
        {
            if (GetIsLooping())
            {
                Stop();
                Play();
            }
            else
            {
                Stop();
            }
        }
    }

    // Skip other update logic if it's not valid streamable source
    if (!UseStreaming() || SourceIDs.IsEmpty())
        return;
    auto clip = Clip.Get();
    clip->Locker.Lock();

    // Handle streaming buffers queue submit (ensure that clip has loaded the first chunk with audio data)
    if (_needToUpdateStreamingBuffers && clip->Buffers[_streamingFirstChunk] != AUDIO_BUFFER_ID_INVALID)
    {
        // Get buffers in a queue count
        int32 numQueuedBuffers;
        AudioBackend::Source::GetQueuedBuffersCount(this, numQueuedBuffers);

        // Queue missing buffers
        uint32 bufferId;
        if (numQueuedBuffers < 1 && (bufferId = clip->Buffers[_streamingFirstChunk]) != AUDIO_BUFFER_ID_INVALID)
        {
            AudioBackend::Source::QueueBuffer(this, bufferId);
        }
        if (numQueuedBuffers < 2 && _streamingFirstChunk + 1 < clip->Buffers.Count() && (bufferId = clip->Buffers[_streamingFirstChunk + 1]) != AUDIO_BUFFER_ID_INVALID)
        {
            AudioBackend::Source::QueueBuffer(this, bufferId);
        }

        // Clear flag
        _needToUpdateStreamingBuffers = false;

        // Play it if need to
        if (!_isActuallyPlayingSth)
        {
            PlayInternal();
        }
    }

    // Track the current buffer index via processed buffers gather
    if (_isActuallyPlayingSth)
    {
        // Get the processed buffers count
        int32 numProcessedBuffers = 0;
        AudioBackend::Source::GetProcessedBuffersCount(this, numProcessedBuffers);
        if (numProcessedBuffers > 0)
        {
            ASSERT(numProcessedBuffers <= ASSET_FILE_DATA_CHUNKS);

            // Unbind processed buffers from the source
            AudioBackend::Source::DequeueProcessedBuffers(this);

            // Move the chunk pointer (AudioStreamingHandler will request new chunks streaming)
            _streamingFirstChunk += numProcessedBuffers;

            // Check if reached the end
            if (_streamingFirstChunk >= clip->Buffers.Count())
            {
                // Loop over the clip or end play
                if (GetIsLooping())
                {
                    // Move to the begin
                    _streamingFirstChunk = 0;

                    // Stop audio and request buffers re-sync and then play continue
                    Stop();
                    Play();
                }
                else
                {
                    Stop();
                }
            }
            ASSERT(_streamingFirstChunk < clip->Buffers.Count());

            // Update clip data streaming
            clip->RequestStreamingUpdate();
        }
    }

    clip->Locker.Unlock();
}

void AudioSource::OnEnable()
{
    _prevPos = GetPosition();
    _velocity = Vector3::Zero;
    _clipChanged = false;

    Audio::OnAddSource(this);
    GetScene()->Ticking.Update.AddTick<AudioSource, &AudioSource::Update>(this);
#if USE_EDITOR
    GetSceneRendering()->AddViewportIcon(this);
#endif

    // Base
    Actor::OnEnable();
}

void AudioSource::OnDisable()
{
    Stop();

#if USE_EDITOR
    GetSceneRendering()->RemoveViewportIcon(this);
#endif
    GetScene()->Ticking.Update.RemoveTick(this);
    Audio::OnRemoveSource(this);

    // Base
    Actor::OnDisable();
}

void AudioSource::OnTransformChanged()
{
    // Base
    Actor::OnTransformChanged();

    _box = BoundingBox(_transform.Translation);
    _sphere = BoundingSphere(_transform.Translation, 0.0f);

    if (IsActiveInHierarchy() && SourceIDs.HasItems())
    {
        AudioBackend::Source::TransformChanged(this);
    }
}

void AudioSource::BeginPlay(SceneBeginData* data)
{
    // Base
    Actor::BeginPlay(data);

    if (IsActiveInHierarchy() && _playOnStart)
    {
#if USE_EDITOR
        if (Time::GetGamePaused())
            return;
#endif
        Play();
        if (GetStartTime() > 0)
            SetTime(GetStartTime());
    }
}
