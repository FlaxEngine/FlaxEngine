%copyright%using System;
using System.Collections.Generic;
using FlaxEngine;

namespace %namespace%;

/// <summary>
/// %class% Actor.
/// </summary>
public class %class% : Actor
{
    /// <inheritdoc/>
    public override void OnBeginPlay()
    {
        base.OnBeginPlay();
        // Here you can add code that needs to be called when Actor added to the game. This is called during edit time as well.
    }

    /// <inheritdoc/>
    public override void OnEndPlay()
    {
        base.OnEndPlay();
        // Here you can add code that needs to be called when Actor removed to the game. This is called during edit time as well.
    }
    
    /// <inheritdoc/>
    public override void OnEnable()
    {
        base.OnEnable();
        // Here you can add code that needs to be called when Actor is enabled (eg. register for events). This is called during edit time as well.
    }

    /// <inheritdoc/>
    public override void OnDisable()
    {
        base.OnDisable();
        // Here you can add code that needs to be called when Actor is disabled (eg. unregister from events). This is called during edit time as well.
    }
}
