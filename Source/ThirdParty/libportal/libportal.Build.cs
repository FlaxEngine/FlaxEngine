// Copyright (c) 2012-2025 Wojciech Figat. All rights reserved.

using System.IO;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// https://github.com/flatpak/libportal
/// </summary>
public class libportal : DepsModule
{
    /// <inheritdoc />
    public override void Init()
    {
        base.Init();

        LicenseType = LicenseTypes.Custom;
        LicenseFilePath = "LICENSE.txt";

        // Merge third-party modules into engine binary
        BinaryModuleName = "FlaxEngine";
    }

    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        var depsRoot = options.DepsFolder;
        switch (options.Platform.Target)
        {
            case TargetPlatform.Linux:
                options.OutputFiles.Add(Path.Combine(depsRoot, "libportal.a"));
                
                options.PublicIncludePaths.Add("/usr/include/glib-2.0");
                options.PublicIncludePaths.Add("/usr/lib/glib-2.0/include");
                
                
                //options.SourceFiles.Add(Path.Combine(FolderPath, "portal-enums.c"));
                options.SourceFiles.AddRange(Directory.GetFiles(FolderPath, "*.c", SearchOption.TopDirectoryOnly));
                break;
            default: throw new InvalidPlatformException(options.Platform.Target);
        }

        options.PublicIncludePaths.Add(Path.Combine(Globals.EngineRoot, @"Source\ThirdParty\libportal\include"));
    }
}
