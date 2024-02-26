// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "AudioListener.h"
#include "Engine/Engine/Time.h"
#include "Engine/Level/Scene/Scene.h"
#include "AudioBackend.h"
#include "Audio.h"

AudioListener::AudioListener(const SpawnParams& params)
    : Actor(params)
    , _velocity(Vector3::Zero)
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
        AudioBackend::Listener::VelocityChanged(this);
    }
}

void AudioListener::OnEnable()
{
    _prevPos = GetPosition();
    _velocity = Vector3::Zero;

    Audio::OnAddListener(this);
    GetScene()->Ticking.Update.AddTick<AudioListener, &AudioListener::Update>(this);
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
    GetScene()->Ticking.Update.RemoveTick(this);
    Audio::OnRemoveListener(this);

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
        AudioBackend::Listener::TransformChanged(this);
    }
}
