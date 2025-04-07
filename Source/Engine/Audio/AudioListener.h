// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Level/Actor.h"

/// <summary>
/// Represents a listener that hears audio sources. For spatial audio the volume and pitch of played audio is determined by the distance, orientation and velocity differences between the source and the listener.
/// </summary>
API_CLASS(Attributes="ActorContextMenu(\"New/Audio/Audio Listener\"), ActorToolbox(\"Other\")")
class FLAXENGINE_API AudioListener : public Actor
{
    DECLARE_SCENE_OBJECT(AudioListener);
private:
    Vector3 _velocity;
    Vector3 _prevPos;

public:
    /// <summary>
    /// Gets the velocity of the listener. Determines pitch in relation to AudioListener's position.
    /// </summary>
    API_PROPERTY() FORCE_INLINE const Vector3& GetVelocity() const
    {
        return _velocity;
    }

private:
    void Update();

public:
    // [Actor]
#if USE_EDITOR
    BoundingBox GetEditorBox() const override
    {
        const Vector3 size(50);
        return BoundingBox(_transform.Translation - size, _transform.Translation + size);
    }
#endif
    bool IntersectsItself(const Ray& ray, Real& distance, Vector3& normal) override;

protected:
    // [Actors]
    void OnEnable() override;
    void OnDisable() override;
    void OnTransformChanged() override;
};
