// Copyright (c) Wojciech Figat. All rights reserved.

using System.IO;
using Flax.Build.NativeCpp;

namespace Flax.Build
{
    /// <summary>
    /// The build module from 3rd Party provided but used as a precompiled dependency.
    /// </summary>
    /// <seealso cref="Flax.Build.ThirdPartyModule" />
    public abstract class DepsModule : ThirdPartyModule
    {
        /// <summary>
        /// Adds the library to the deps module output files (handles platform specific switches).
        /// </summary>
        /// <param name="options">The build options.</param>
        /// <param name="path">The path fo the folder with deps.</param>
        /// <param name="name">The library name.</param>
        public static void AddLib(BuildOptions options, string path, string name)
        {
            switch (options.Platform.Target)
            {
            case TargetPlatform.Windows:
            case TargetPlatform.XboxOne:
            case TargetPlatform.UWP:
            case TargetPlatform.XboxScarlett:
                options.OutputFiles.Add(Path.Combine(path, string.Format("{0}.lib", name)));
                options.OptionalDependencyFiles.Add(Path.Combine(path, string.Format("{0}.pdb", name)));
                break;
            case TargetPlatform.Linux:
            case TargetPlatform.PS4:
            case TargetPlatform.PS5:
            case TargetPlatform.Android:
            case TargetPlatform.Switch:
            case TargetPlatform.Mac:
            case TargetPlatform.iOS:
                options.OutputFiles.Add(Path.Combine(path, string.Format("lib{0}.a", name)));
                break;
            default: throw new InvalidPlatformException(options.Platform.Target);
            }
        }
    }
}
