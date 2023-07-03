// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System.IO;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// https://github.com/NVIDIAGameWorks/NvCloth
/// </summary>
public class NvCloth : DepsModule
{
    /// <inheritdoc />
    public override void Init()
    {
        base.Init();

        LicenseType = LicenseTypes.Custom;
        LicenseFilePath = "License.txt";

        // Merge third-party modules into engine binary
        BinaryModuleName = "FlaxEngine";
    }

    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        options.PublicDefinitions.Add("WITH_CLOTH");
        options.PublicDefinitions.Add("NV_CLOTH_IMPORT=");

        var libName = "NvCloth_x64";
        switch (options.Platform.Target)
        {
        case TargetPlatform.PS4:
        case TargetPlatform.PS5:
        case TargetPlatform.Android:
            libName = "NvCloth";
            break;
        case TargetPlatform.Switch:
            libName = "NvCloth";
            options.PublicIncludePaths.Add(Path.Combine(Globals.EngineRoot, "Source/Platforms/Switch/Binaries/Data/PhysX/physx/include"));
            options.PublicIncludePaths.Add(Path.Combine(Globals.EngineRoot, "Source/Platforms/Switch/Binaries/Data/PhysX/physx/include/foundation"));
            break;
        }
        AddLib(options, options.DepsFolder, libName);
    }
}
