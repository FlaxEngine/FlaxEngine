// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.IO;
using Flax.Build;

namespace Flax.Deps.Dependencies;

/// <summary>
/// libportal.
/// </summary>
/// <seealso cref="Flax.Deps.Dependency" />
class libportal : Dependency
{
    /// <inheritdoc />
    public override TargetPlatform[] Platforms
    {
        get
        {
            switch (BuildPlatform)
            {
            case TargetPlatform.Linux:
                return new[]
                {
                    TargetPlatform.Linux,
                };
            default: return new TargetPlatform[0];
            }
        }
    }

    /// <inheritdoc />
    public override void Build(BuildOptions options)
    {
        var dstPath = Path.Combine(options.ThirdPartyFolder, "libportal");
        var includePath = Path.Combine(dstPath, "include");
        
        string root = options.IntermediateFolder;
        var configs = new string[]
        {
            "-Dbackend-qt6=disabled",
            "-Dbackend-qt5=disabled",
            "-Dbackend-gtk4=disabled",
            "-Dbackend-gtk3=disabled",
            "-Dintrospection=false",
            "-Dtests=false",
            "-Ddocs=false",
            "-Dvapi=false",
            "--buildtype release",
            "--default-library static",
        };
        
        CloneGitRepo(root, "https://github.com/flatpak/libportal");
        GitFetch(root);
        GitResetToCommit(root, "8f5dc8d192f6e31dafe69e35219e3b707bde71ce");  // 0.9.1

        foreach (var platform in options.Platforms)
        {
            foreach (var architecture in options.Architectures)
            {
                BuildStarted(platform, architecture);
                switch (platform)
                {
                case TargetPlatform.Linux:
                {
                    var buildDir = $"build_{architecture}";
                    Utilities.Run("meson", $"{buildDir} {string.Join(" ", configs)}", null, root, Utilities.RunOptions.DefaultTool);
                    Utilities.Run("ninja", $"-C {buildDir} ", null, root, Utilities.RunOptions.DefaultTool);

                    var depsFolder = GetThirdPartyFolder(options, platform, architecture);
                    Utilities.FileCopy(Path.Combine(root, buildDir, "libportal", "libportal.a"), Path.Combine(depsFolder, "libportal.a"));
                    Utilities.FileCopy(Path.Combine(root, buildDir, "libportal", "portal-enums.h"), Path.Combine(includePath, "portal-enums.h"));
                    Utilities.FileCopy(Path.Combine(root, buildDir, "libportal", "portal-enums.c"), Path.Combine(dstPath, "portal-enums.c"));

                    Utilities.FileCopy(Path.Combine(root, "COPYING"), Path.Combine(dstPath, "LICENSE.txt"));

                    if (!Directory.Exists(includePath))
                        Directory.CreateDirectory(includePath);
                    if (!Directory.Exists(Path.Combine(includePath, "libportal")))
                        Directory.CreateDirectory(Path.Combine(includePath, "libportal"));

                    foreach (var file in Directory.GetFiles(Path.Combine(root, "libportal"), "*.h"))
                        Utilities.FileCopy(file, Path.Combine(includePath, "libportal", Path.GetFileName(file)));
                    break;
                }
                }
            }
        }
    }
}
