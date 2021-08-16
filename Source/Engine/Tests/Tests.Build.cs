// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using Flax.Build;

/// <summary>
/// Engine tests module.
/// </summary>
public class Tests : EngineModule
{
    /// <inheritdoc />
    public Tests()
    {
        Deploy = false;
    }

    /// <inheritdoc />
    public override void GetFilesToDeploy(List<string> files)
    {
    }
}
