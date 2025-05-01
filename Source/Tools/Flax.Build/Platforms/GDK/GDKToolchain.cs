// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.IO;
using Flax.Build.NativeCpp;

namespace Flax.Build.Platforms
{
    /// <summary>
    /// The GDK toolchain implementation.
    /// </summary>
    /// <seealso cref="Toolchain" />
    /// <seealso cref="Flax.Build.Platforms.WindowsToolchainBase" />
    public abstract class GDKToolchain : WindowsToolchainBase
    {
        /// <summary>
        /// Gets the version of Xbox Services toolset.
        /// </summary>
        public WindowsPlatformToolset XboxServicesToolset => Toolset > WindowsPlatformToolset.v142 ? WindowsPlatformToolset.v142 : Toolset;

        /// <summary>
        /// Initializes a new instance of the <see cref="GDKToolchain"/> class.
        /// </summary>
        /// <param name="platform">The platform.</param>
        /// <param name="architecture">The architecture.</param>
        /// <param name="toolset">The Windows toolset to use.</param>
        protected GDKToolchain(GDKPlatform platform, TargetArchitecture architecture, WindowsPlatformToolset toolset = WindowsPlatformToolset.Latest)
        : base(platform, architecture, toolset, WindowsPlatformSDK.Latest)
        {
            // Setup system paths
            SystemIncludePaths.Add(Path.Combine(GDK.Instance.RootPath, "GRDK\\GameKit\\Include"));
            SystemLibraryPaths.Add(Path.Combine(GDK.Instance.RootPath, "GRDK\\GameKit\\Lib\\amd64"));
            var xboxServicesPath = Path.Combine(GDK.Instance.RootPath, "GRDK\\ExtensionLibraries\\Xbox.Services.API.C\\DesignTime\\CommonConfiguration\\Neutral\\");
            var xboxServicesToolset = XboxServicesToolset;
            SystemIncludePaths.Add(xboxServicesPath + "Include");
            SystemLibraryPaths.Add(xboxServicesPath + "Lib\\Release\\" + xboxServicesToolset);
            var curlPath = Path.Combine(GDK.Instance.RootPath, "GRDK\\ExtensionLibraries\\Xbox.XCurl.API\\DesignTime\\CommonConfiguration\\Neutral\\");
            SystemIncludePaths.Add(curlPath + "Include");
            SystemLibraryPaths.Add(curlPath + "Lib");
        }

        /// <inheritdoc />
        public override void SetupEnvironment(BuildOptions options)
        {
            base.SetupEnvironment(options);

            options.CompileEnv.PreprocessorDefinitions.Add("PLATFORM_GDK");
            options.CompileEnv.PreprocessorDefinitions.Add("WINAPI_FAMILY=WINAPI_FAMILY_GAMES");
            options.CompileEnv.PreprocessorDefinitions.Add("_ATL_NO_DEFAULT_LIBS");
            options.CompileEnv.PreprocessorDefinitions.Add("__WRL_NO_DEFAULT_LIB__");

            options.LinkEnv.InputLibraries.Add("xgameruntime.lib");
            options.LinkEnv.InputLibraries.Add("xgameplatform.lib");
            var xboxServicesToolset = XboxServicesToolset;
            options.LinkEnv.InputLibraries.Add($"Microsoft.Xbox.Services.{(int)xboxServicesToolset}.GDK.C.lib");

            var toolsetPath = WindowsPlatformBase.GetToolsets()[Toolset];
            var toolsPath = WindowsPlatformBase.GetVCToolPath(Toolset, TargetArchitecture.x64, Architecture);
            if (options.CompileEnv.UseDebugCRT)
                throw new Exception("Don't use debug CRT on GDK.");
            var name = Path.GetFileName(toolsetPath);
            var redistToolsPath = Path.Combine(toolsPath, "..", "..", "..", "..", "..", "..", "Redist/MSVC/");
            var paths = Directory.GetDirectories(redistToolsPath, name.Substring(0, 2) + "*");
            if (paths.Length == 0)
                throw new Exception($"Failed to find MSVC redistribute binaries for toolset '{Toolset}' inside folder '{toolsPath}'");
            var crtToolset = Toolset > WindowsPlatformToolset.v143 ? WindowsPlatformToolset.v143 : Toolset;
            redistToolsPath = Path.Combine(paths[0], "x64", "Microsoft.VC" + (int)crtToolset + ".CRT");
            redistToolsPath = Utilities.RemovePathRelativeParts(redistToolsPath);
            options.DependencyFiles.Add(Path.Combine(redistToolsPath, "concrt140.dll"));
            options.DependencyFiles.Add(Path.Combine(redistToolsPath, "msvcp140.dll"));
            options.DependencyFiles.Add(Path.Combine(redistToolsPath, "msvcp140_1.dll"));
            options.DependencyFiles.Add(Path.Combine(redistToolsPath, "msvcp140_2.dll"));
            options.DependencyFiles.Add(Path.Combine(redistToolsPath, "msvcp140_codecvt_ids.dll"));
            options.DependencyFiles.Add(Path.Combine(redistToolsPath, "vccorlib140.dll"));
            options.DependencyFiles.Add(Path.Combine(redistToolsPath, "vcruntime140.dll"));
            options.DependencyFiles.Add(Path.Combine(redistToolsPath, "vcruntime140_1.dll"));
        }
    }
}
