// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.IO;
using System.Linq;
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
        /// Initializes a new instance of the <see cref="GDKToolchain"/> class.
        /// </summary>
        /// <param name="platform">The platform.</param>
        /// <param name="architecture">The architecture.</param>
        protected GDKToolchain(GDKPlatform platform, TargetArchitecture architecture)
        : base(platform, architecture, WindowsPlatformBase.GetToolsets().Keys.Where(x => x <= WindowsPlatformToolset.v142).Max(), WindowsPlatformSDK.v10_0_19041_0)
        {
            // Setup system paths
            SystemIncludePaths.Add(Path.Combine(GDK.Instance.RootPath, "GRDK\\GameKit\\Include"));
            SystemLibraryPaths.Add(Path.Combine(GDK.Instance.RootPath, "GRDK\\GameKit\\Lib\\amd64"));
            var xboxServicesPath = Path.Combine(GDK.Instance.RootPath, "GRDK\\ExtensionLibraries\\Xbox.Services.API.C\\DesignTime\\CommonConfiguration\\Neutral\\");
            SystemIncludePaths.Add(xboxServicesPath + "Include");
            SystemLibraryPaths.Add(xboxServicesPath + "Lib\\Release\\" + Toolset);
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
            options.LinkEnv.InputLibraries.Add("Microsoft.Xbox.Services.142.GDK.C.lib");

            var toolsetPath = WindowsPlatformBase.GetToolsets()[Toolset];
            var toolsPath = WindowsPlatformBase.GetVCToolPath64(Toolset);
            if (options.CompileEnv.UseDebugCRT)
                throw new Exception("Don't use debug CRT on GDK.");
            var name = Path.GetFileName(toolsetPath);
            var redistToolsPath = Path.Combine(toolsPath, "..", "..", "..", "..", "..", "..", "Redist/MSVC/");
            var paths = Directory.GetDirectories(redistToolsPath, name.Substring(0, 5) + "*");
            redistToolsPath = Path.Combine(paths[0], "x64", "Microsoft.VC" + (int)Toolset + ".CRT");
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
