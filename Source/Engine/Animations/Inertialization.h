// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/Transform.h"

/// <summary>
/// Inertialization: a blend that switches straight to the new pose and hides the switch by adding the difference between the old and the new pose on top of it, decaying to zero over the blend time.
/// Unlike a cross-fade, only the new pose is evaluated, and the old pose's motion toward the new pose carries on smoothly at the switch (only motion too fast for the offset gets a small velocity jump). A new request during a blend starts from the current output, so it doesn't pop.
/// Each bone's translation, rotation and scale offsets decay along their own quintic curve that starts with the offset's value and velocity and ends at zero with zero velocity and acceleration.
/// The velocity kept is the old pose's motion along the offset (translation and scale) or about the offset's rotation axis; motion across it, or away from the new pose, isn't carried on, so a bone never moves further from the new pose than where the blend started. Fast motion is limited, so a limb's bones stay in step and its end stays near the path between the poses.
/// The technique follows the public description in David Bollo's GDC 2018 talk "Inertialization: High-Performance Animation Transitions in Gears of War".
/// </summary>
class FLAXENGINE_API Inertialization
{
public:
    /// <summary>
    /// The decay curve of a single offset value.
    /// </summary>
    struct Curve
    {
        /// <summary>
        /// The quintic polynomial coefficients (from t^5 down to t^0, there is no t^2 term).
        /// </summary>
        float C5 = 0.0f, C4 = 0.0f, C3 = 0.0f, C1 = 0.0f, C0 = 0.0f;

        /// <summary>
        /// The time (in seconds) at which the offset reaches zero.
        /// </summary>
        float Duration = 0.0f;

        /// <summary>
        /// Sets up the curve for the offset value and its velocity. A velocity away from zero is dropped and a velocity toward it is limited, so the offset never overshoots and all curves of a blend stay close in step.
        /// </summary>
        /// <param name="value">The offset value at the switch (zero or more).</param>
        /// <param name="velocity">The offset velocity at the switch (per second).</param>
        /// <param name="duration">The blend time (in seconds).</param>
        void Setup(float value, float velocity, float duration);

        /// <summary>
        /// Gets the offset value at the time since the switch (zero from the curve's duration on).
        /// </summary>
        float Evaluate(float time) const;
    };

    /// <summary>
    /// The offset of a single bone, kept in the bone's local space.
    /// </summary>
    struct BoneOffset
    {
        Float3 TranslationDirection;
        Float3 RotationAxis;
        Float3 ScaleDirection;
        Curve Translation;
        Curve Rotation;
        Curve Scale;
    };

public:
    /// <summary>
    /// Starts a blend: records the offset of each bone from the new pose to the old (last output) pose and its velocity.
    /// </summary>
    /// <param name="previous">The last output pose (local bone transformations).</param>
    /// <param name="beforePrevious">The output pose one frame before the last one, to get the velocity from. Can be the same as previous (no velocity).</param>
    /// <param name="target">The new pose (local bone transformations).</param>
    /// <param name="count">The amount of bones.</param>
    /// <param name="previousDeltaTime">The time between the two output poses (in seconds).</param>
    /// <param name="duration">The blend time (in seconds).</param>
    /// <param name="offsets">The output offsets (one per bone).</param>
    static void Start(const Transform* previous, const Transform* beforePrevious, const Transform* target, int32 count, float previousDeltaTime, float duration, BoneOffset* offsets);

    /// <summary>
    /// Adds the decayed offsets on top of the pose.
    /// </summary>
    /// <param name="offsets">The offsets (one per bone) from Start.</param>
    /// <param name="count">The amount of bones.</param>
    /// <param name="time">The time since the blend started (in seconds).</param>
    /// <param name="pose">The new pose (local bone transformations) to offset.</param>
    static void Apply(const BoneOffset* offsets, int32 count, float time, Transform* pose);
};
