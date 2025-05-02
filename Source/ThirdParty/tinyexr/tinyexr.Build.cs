// Copyright (c) Wojciech Figat. All rights reserved.

using Flax.Build;
using Flax.Build.NativeCpp;
using System.IO;

/// <summary>
/// https://github.com/syoyo/tinyexr
/// </summary>
public class tinyexr : ThirdPartyModule
{
    /// <inheritdoc />
    public override void Init()
    {
        base.Init();

        LicenseType = LicenseTypes.BSD3Clause;
        LicenseFilePath = "LICENSE";

        // Merge third-party modules into engine binary
        BinaryModuleName = "FlaxEngine";
    }

    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        options.PublicIncludePaths.Add(Path.Combine(Globals.EngineRoot, "Source/ThirdParty/tinyexr"));
    }
}
