// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using Flax.Build.Graph;
using Flax.Build.NativeCpp;
using Flax.Build.Projects.VisualStudio;

namespace Flax.Build.Platforms
{
    /// <summary>
    /// The Universal Windows Platform (UWP) toolchain implementation.
    /// </summary>
    /// <seealso cref="Toolchain" />
    /// <seealso cref="Flax.Build.Platforms.WindowsToolchainBase" />
    public class UWPToolchain : WindowsToolchainBase
    {
        private readonly List<string> _usingDirs = new List<string>();

        /// <summary>
        /// Initializes a new instance of the <see cref="UWPToolchain"/> class.
        /// </summary>
        /// <param name="platform">The platform.</param>
        /// <param name="architecture">The target architecture.</param>
        /// <param name="toolsetVer">The target platform toolset version.</param>
        /// <param name="sdkVer">The target platform SDK version.</param>
        public UWPToolchain(UWPPlatform platform, TargetArchitecture architecture, WindowsPlatformToolset toolsetVer = WindowsPlatformToolset.Latest, WindowsPlatformSDK sdkVer = WindowsPlatformSDK.Latest)
        : base(platform, architecture, toolsetVer, sdkVer)
        {
            var visualStudio = VisualStudioInstance.GetInstances().FirstOrDefault(x => x.Version == VisualStudioVersion.VisualStudio2017 || x.Version == VisualStudioVersion.VisualStudio2019 || x.Version == VisualStudioVersion.VisualStudio2022);
            if (visualStudio == null)
                throw new Exception("Missing Visual Studio 2017 or newer. It's required to build for UWP.");
            _usingDirs.Add(Path.Combine(visualStudio.Path, "VC", "vcpackages"));

            var sdks = WindowsPlatformBase.GetSDKs();
            var sdk10 = sdks.FirstOrDefault(x => x.Key != WindowsPlatformSDK.v8_1).Value;
            if (sdk10 == null)
                throw new Exception("Missing Windows 10 SDK. It's required to build for UWP.");
            var sdk10Ver = WindowsPlatformBase.GetSDKVersion(SDK).ToString();
            _usingDirs.Add(Path.Combine(sdk10, "References"));
            _usingDirs.Add(Path.Combine(sdk10, "References", sdk10Ver));
            _usingDirs.Add(Path.Combine(sdk10, "References", "CommonConfiguration", "Neutral"));
            _usingDirs.Add(Path.Combine(sdk10, "UnionMetadata", sdk10Ver));
        }

        /// <inheritdoc />
        public override void LogInfo()
        {
            base.LogInfo();

            Log.Error("UWP (Windows Store) platform has been deprecated and is no longer supported!");
        }

        /// <inheritdoc />
        protected override void SetupCompileCppFilesArgs(TaskGraph graph, BuildOptions options, List<string> args)
        {
            base.SetupCompileCppFilesArgs(graph, options, args);

            if (options.CompileEnv.WinRTComponentExtensions)
            {
                foreach (var path in _usingDirs)
                {
                    args.Add(string.Format("/AI\"{0}\"", path));
                }
            }
        }

        /// <inheritdoc />
        public override void SetupEnvironment(BuildOptions options)
        {
            base.SetupEnvironment(options);

            options.CompileEnv.PreprocessorDefinitions.Add("PLATFORM_UWP");
            options.CompileEnv.PreprocessorDefinitions.Add("WINAPI_FAMILY=WINAPI_FAMILY_PC_APP");
            options.CompileEnv.PreprocessorDefinitions.Add("_WINRT_DLL");
            options.CompileEnv.PreprocessorDefinitions.Add("_WINDLL");
            options.CompileEnv.PreprocessorDefinitions.Add("__WRL_NO_DEFAULT_LIB__");
            options.CompileEnv.PreprocessorDefinitions.Add("WINVER=0x0A00");

            options.LinkEnv.InputLibraries.Add("WindowsApp.lib");
        }
    }
}
