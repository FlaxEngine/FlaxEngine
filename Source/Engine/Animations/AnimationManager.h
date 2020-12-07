// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

class AnimatedModel;

/// <summary>
/// The animations service.
/// </summary>
class AnimationManager
{
public:

    /// <summary>
    /// Adds an animated model to update.
    /// </summary>
    /// <param name="obj">The object.</param>
    static void AddToUpdate(AnimatedModel* obj);

    /// <summary>
    /// Removes the animated model from update.
    /// </summary>
    /// <param name="obj">The object.</param>
    static void RemoveFromUpdate(AnimatedModel* obj);
};
