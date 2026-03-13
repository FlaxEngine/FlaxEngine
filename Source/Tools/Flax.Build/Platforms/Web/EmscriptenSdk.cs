// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.IO;
using System.Linq;

namespace Flax.Build.Platforms
{
    /// <summary>
    /// The Emscripten SDK (https://emscripten.org/).
    /// </summary>
    /// <seealso cref="Sdk" />
    public sealed class EmscriptenSdk : Sdk
    {
        /// <summary>
        /// The singleton instance.
        /// </summary>
        public static readonly EmscriptenSdk Instance = new EmscriptenSdk();

        /// <inheritdoc />
        public override TargetPlatform[] Platforms => new[]
        {
            TargetPlatform.Windows,
            TargetPlatform.Linux,
            TargetPlatform.Mac,
        };

        /// <summary>
        /// Full path to the current SDK folder with binaries, tools and sources (eg. '%EMSDK%\upstream').
        /// </summary>
        public string EmscriptenPath;

        /// <summary>
        /// Full path to the CMake toolchain file (eg. '%EMSDK%\upstream\emscripten\cmake\Modules\Platform\Emscripten.cmake').
        /// </summary>
        public string CMakeToolchainPath;

        /// <summary>
        /// Initializes a new instance of the <see cref="AndroidSdk"/> class.
        /// </summary>
        public EmscriptenSdk()
        {
            if (!Platforms.Contains(Platform.BuildTargetPlatform))
                return;

            // Find Emscripten SDK folder path
            var sdkPath = Environment.GetEnvironmentVariable("EMSDK");
            if (string.IsNullOrEmpty(sdkPath))
            {
                Log.Warning("Missing Emscripten SDK. Cannot build for Web platform.");
            }
            else if (!Directory.Exists(sdkPath))
            {
                Log.Warning(string.Format("Specified Emscripten SDK folder in EMSDK env variable doesn't exist ({0})", sdkPath));
            }
            else
            {
                RootPath = sdkPath;
                EmscriptenPath = Path.Combine(sdkPath, "upstream");
                CMakeToolchainPath = Path.Combine(EmscriptenPath, "emscripten/cmake/Modules/Platform/Emscripten.cmake");
                var versionPath = Path.Combine(EmscriptenPath, "emscripten", "emscripten-version.txt");
                if (File.Exists(versionPath))
                {
                    try
                    {
                        // Read version
                        var versionStr = File.ReadAllLines(versionPath)[0];
                        versionStr = versionStr.Trim();
                        if (versionStr.StartsWith('\"') && versionStr.EndsWith('\"'))
                            versionStr = versionStr.Substring(1, versionStr.Length - 2);
                        Version = new Version(versionStr);
                        
                        var minVersion = new Version(4, 0);
                        if (Version < minVersion)
                        {
                            Log.Error(string.Format("Unsupported Emscripten SDK version {0}. Minimum supported is {1}.", Version, minVersion));
                            return;
                        }
                        Log.Info(string.Format("Found Emscripten SDK {0} at {1}", Version, RootPath));
                        IsValid = true;
                    }
                    catch (Exception ex)
                    {
                        Log.Error($"Failed to read Emscripten SDK version from file '{versionPath}'");
                        Log.Exception(ex);
                    }
                }
                else
                    Log.Warning($"Missing file {versionPath}");
            }
        }
    }
}
