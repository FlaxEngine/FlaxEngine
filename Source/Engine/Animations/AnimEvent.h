// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/SerializableScriptingObject.h"
#if USE_EDITOR
#include "Engine/Core/Math/Color.h"
#endif

class AnimatedModel;
class Animation;

/// <summary>
/// The animation notification event triggered during animation playback.
/// </summary>
API_CLASS(Abstract) class FLAXENGINE_API AnimEvent : public SerializableScriptingObject
{
    DECLARE_SCRIPTING_TYPE(AnimEvent);

    /// <summary>
    /// Indicates whether the event can be executed in async from a thread that updates the animated model. Otherwise, event execution will be delayed until the sync point of the animated model and called from the main thread. Async events need to precisely handle data access, especially when it comes to editing scene objects with multi-threading.
    /// </summary>
    API_FIELD(Attributes="HideInEditor, NoSerialize") bool Async = false;

#if USE_EDITOR
    /// <summary>
    /// Event display color in the Editor.
    /// </summary>
    API_FIELD(Attributes="HideInEditor, NoSerialize") Color Color = Color::White;
#endif

    /// <summary>
    /// Animation event notification.
    /// </summary>
    /// <param name="actor">The animated model actor instance.</param>
    /// <param name="anim">The source animation.</param>
    /// <param name="time">The current animation time (in seconds).</param>
    /// <param name="deltaTime">The current animation tick delta time (in seconds).</param>
    API_FUNCTION() virtual void OnEvent(AnimatedModel* actor, Animation* anim, float time, float deltaTime)
    {
    }
};

/// <summary>
/// The animation notification event (with duration) triggered during animation playback that contains begin and end (event notification is received as a tick).
/// </summary>
API_CLASS(Abstract) class FLAXENGINE_API AnimContinuousEvent : public AnimEvent
{
    DECLARE_SCRIPTING_TYPE(AnimContinuousEvent);

    /// <summary>
    /// Animation notification called before the first event.
    /// </summary>
    /// <param name="actor">The animated model actor instance.</param>
    /// <param name="anim">The source animation.</param>
    /// <param name="time">The current animation time (in seconds).</param>
    /// <param name="deltaTime">The current animation tick delta time (in seconds).</param>
    API_FUNCTION() virtual void OnBegin(AnimatedModel* actor, Animation* anim, float time, float deltaTime)
    {
    }

    /// <summary>
    /// Animation notification called after the last event (guaranteed to be always called).
    /// </summary>
    /// <param name="actor">The animated model actor instance.</param>
    /// <param name="anim">The source animation.</param>
    /// <param name="time">The current animation time (in seconds).</param>
    /// <param name="deltaTime">The current animation tick delta time (in seconds).</param>
    API_FUNCTION() virtual void OnEnd(AnimatedModel* actor, Animation* anim, float time, float deltaTime)
    {
    }
};
