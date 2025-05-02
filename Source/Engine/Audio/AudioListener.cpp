// Copyright (c) Wojciech Figat. All rights reserved.

#include "AudioListener.h"
#include "Engine/Engine/Time.h"
#include "Engine/Level/Scene/Scene.h"
#include "Engine/Core/Log.h"
#include "AudioBackend.h"
#include "Audio.h"

AudioListener::AudioListener(const SpawnParams& params)
    : Actor(params)
    , _velocity(Vector3::Zero)
    , _prevPos(Vector3::Zero)
{
}

bool AudioListener::IntersectsItself(const Ray& ray, Real& distance, Vector3& normal)
{
    return false;
}

void AudioListener::Update()
{
    // Update the velocity
    const Vector3 pos = GetPosition();
    const float dt = Time::Update.UnscaledDeltaTime.GetTotalSeconds();
    const auto prevVelocity = _velocity;
    _velocity = (pos - _prevPos) / dt;
    _prevPos = pos;
    if (_velocity != prevVelocity)
    {
        AudioBackend::Listener::VelocityChanged(_velocity);
    }
}

void AudioListener::OnEnable()
{
    _prevPos = GetPosition();
    _velocity = Vector3::Zero;
    if (Audio::Listeners.Count() >= AUDIO_MAX_LISTENERS)
    {
        LOG(Error, "Unsupported amount of the audio listeners!");
    }
    else
    {
        ASSERT(!Audio::Listeners.Contains(this));
        if (Audio::Listeners.Count() > 0)
            LOG(Warning, "There is more than one Audio Listener active. Please make sure only exactly one is active at any given time.");
        Audio::Listeners.Add(this);
        AudioBackend::Listener::Reset();
        AudioBackend::Listener::TransformChanged(GetPosition(), GetOrientation());
        GetScene()->Ticking.Update.AddTick<AudioListener, &AudioListener::Update>(this);
    }
#if USE_EDITOR
    GetSceneRendering()->AddViewportIcon(this);
#endif

    // Base
    Actor::OnEnable();
}

void AudioListener::OnDisable()
{
#if USE_EDITOR
    GetSceneRendering()->RemoveViewportIcon(this);
#endif
    if (!Audio::Listeners.Remove(this))
    {
        GetScene()->Ticking.Update.RemoveTick(this);
        AudioBackend::Listener::Reset();
    }

    // Base
    Actor::OnDisable();
}

void AudioListener::OnTransformChanged()
{
    // Base
    Actor::OnTransformChanged();

    _box = BoundingBox(_transform.Translation);
    _sphere = BoundingSphere(_transform.Translation, 0.0f);

    if (IsActiveInHierarchy() && IsDuringPlay())
    {
        AudioBackend::Listener::TransformChanged(GetPosition(), GetOrientation());
    }
}
