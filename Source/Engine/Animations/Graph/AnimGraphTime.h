// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"

/// <summary>
/// How an animation node turns its time into a position in its animation: its start position plus its own time,
/// wrapped (looping) or held at the ends, and that position as a frame of the animation.
/// </summary>
namespace AnimGraphTime
{
    /// <summary>
    /// The position in an animation of a node at <paramref name="timePos"/> seconds of its own from
    /// <paramref name="startTimePos"/>, within 0 to <paramref name="length"/>: wrapped when looping, held at the ends
    /// otherwise. Past its end it holds there, its own time no more than takes it to the end from the start position,
    /// so a node played by its start position alone (a speed of 0 and the time given each update) samples the time it's
    /// given next however far past its end it was given before.
    /// </summary>
    FLAXENGINE_API float GetAnimPos(float& timePos, float startTimePos, float speed, bool loop, float length);

    /// <summary>
    /// The frame of an animation <paramref name="animLength"/> seconds long (at <paramref name="framesPerSecond"/>)
    /// shown at <paramref name="pos"/> seconds into a node <paramref name="length"/> seconds long (a whole number of the
    /// animation's cycles fitted to it). At or past the node's end it's the animation's last frame.
    /// </summary>
    FLAXENGINE_API float GetAnimSamplePos(float length, float animLength, float framesPerSecond, float pos);
}
