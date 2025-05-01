// Copyright (c) Wojciech Figat. All rights reserved.

#include "VideoPlayer.h"
#include "Video.h"
#include "VideoBackend.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Vector2.h"
#if USE_EDITOR
#include "Engine/Engine/Time.h"
#include "Engine/Level/Scene/SceneRendering.h"
#endif

VideoPlayer::VideoPlayer(const SpawnParams& params)
    : Actor(params)
{
}

VideoPlayer::~VideoPlayer()
{
    // Ensure to free player memory
    Stop();
    if (_player.Backend)
        _player.Backend->Player_Destroy(_player);
}

void VideoPlayer::SetIsLooping(bool value)
{
    if (_loop == value)
        return;
    _loop = value;
    UpdateInfo();
}

void VideoPlayer::SetIsAudioSpatial(bool value)
{
    if (_isSpatial == value)
        return;
    _isSpatial = value;
    UpdateInfo();
}

void VideoPlayer::SetAudioVolume(float value)
{
    value = Math::Saturate(value);
    if (Math::NearEqual(_volume, value))
        return;
    _volume = value;
    UpdateInfo();
}

void VideoPlayer::SetAudioPan(float value)
{
    value = Math::Clamp(value, -1.0f, 1.0f);
    if (Math::NearEqual(_pan, value))
        return;
    _pan = value;
    UpdateInfo();
}

void VideoPlayer::SetAudioMinDistance(float value)
{
    value = Math::Max(0.0f, value);
    if (Math::NearEqual(_minDistance, value))
        return;
    _minDistance = value;
    UpdateInfo();
}

void VideoPlayer::SetAudioAttenuation(float value)
{
    value = Math::Max(0.0f, value);
    if (Math::NearEqual(_attenuation, value))
        return;
    _attenuation = value;
    UpdateInfo();
}

void VideoPlayer::Play()
{
    auto state = _state;
    if (state == States::Playing)
        return;

    if (!_player.Backend)
    {
        if (Url.IsEmpty())
        {
            LOG(Warning, "Cannot play Video source without an url ({0})", GetNamePath());
            return;
        }

        // Create video player
        VideoBackendPlayerInfo info;
        GetInfo(info);
        if (Video::CreatePlayerBackend(info, _player))
            return;

        // Pre-allocate output video texture
        _player.InitVideoFrame();
    }

    _player.Backend->Player_Play(_player);
    _state = States::Playing;
}

void VideoPlayer::Pause()
{
    if (_state != States::Playing)
        return;
    _state = States::Paused;
    if (_player.Backend)
        _player.Backend->Player_Pause(_player);
}

void VideoPlayer::Stop()
{
    if (_state == States::Stopped)
        return;
    _state = States::Stopped;
    if (_player.Backend)
        _player.Backend->Player_Stop(_player);
}

float VideoPlayer::GetTime() const
{
    if (_state == States::Stopped || _player.Backend == nullptr)
        return 0.0f;
    return _player.Backend->Player_GetTime(_player).GetTotalSeconds();
}

void VideoPlayer::SetTime(float time)
{
    if (_state == States::Stopped || _player.Backend == nullptr)
        return;
    TimeSpan timeSpan = TimeSpan::FromSeconds(time);
    timeSpan.Ticks = Math::Clamp<int64>(timeSpan.Ticks, 0, _player.Duration.Ticks);
    _player.Backend->Player_Seek(_player, timeSpan);
}

float VideoPlayer::GetDuration() const
{
    return _player.Duration.GetTotalSeconds();
}

float VideoPlayer::GetFrameRate() const
{
    return _player.FrameRate;
}

int32 VideoPlayer::GetFramesCount() const
{
    return _player.FramesCount;
}

#if USE_EDITOR
#include "Engine/Debug/DebugDraw.h"

void VideoPlayer::OnDebugDrawSelected()
{
    // Draw influence range
    if (_isSpatial)
        DEBUG_DRAW_WIRE_SPHERE(BoundingSphere(_transform.Translation, _minDistance), Color::CornflowerBlue, 0, true);
    Actor::OnDebugDrawSelected();
}
#endif

bool VideoPlayer::IntersectsItself(const Ray& ray, Real& distance, Vector3& normal)
{
    return false;
}

Int2 VideoPlayer::GetSize() const
{
    return Int2(_player.Width, _player.Height);
}

GPUTexture* VideoPlayer::GetFrame() const
{
    return _player.Frame;
}

void VideoPlayer::GetInfo(VideoBackendPlayerInfo& info) const
{
    info.Url = Url;
    info.Loop = _loop;
    info.Spatial = _isSpatial;
    info.Volume = _volume;
    info.Pan = _pan;
    info.MinDistance = _minDistance;
    info.Attenuation = _attenuation;
    info.Transform = &_transform;
}

void VideoPlayer::UpdateInfo()
{
    if (_player.Backend)
    {
        VideoBackendPlayerInfo info;
        GetInfo(info);
        _player.Backend->Player_UpdateInfo(_player, info);
    }
}

void VideoPlayer::OnEnable()
{
#if USE_EDITOR
    GetSceneRendering()->AddViewportIcon(this);
#endif

    Actor::OnEnable();
}

void VideoPlayer::OnDisable()
{
    Stop();
    if (_player.Backend)
        _player.Backend->Player_Destroy(_player);
#if USE_EDITOR
    GetSceneRendering()->RemoveViewportIcon(this);
#endif

    Actor::OnDisable();
}

void VideoPlayer::OnTransformChanged()
{
    Actor::OnTransformChanged();

    _box = BoundingBox(_transform.Translation);
    _sphere = BoundingSphere(_transform.Translation, 0.0f);
}

void VideoPlayer::BeginPlay(SceneBeginData* data)
{
    Actor::BeginPlay(data);

    // Play on start
    if (IsActiveInHierarchy() && PlayOnStart)
    {
#if USE_EDITOR
        if (Time::GetGamePaused())
            return;
#endif
        Play();
        if (StartTime > 0)
            SetTime(StartTime);
    }
}
