// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Xml;
using Flax.Build;
using Flax.Deploy;
using Ionic.Zip;

namespace Flax.Deps.Dependencies
{
    /// <summary>
    /// .NET runtime
    /// </summary>
    /// <seealso cref="Flax.Deps.Dependency" />
    class nethost : Dependency
    {
        /// <inheritdoc />
        public override TargetPlatform[] Platforms
        {
            get
            {
                switch (BuildPlatform)
                {
                case TargetPlatform.Windows:
                    return new[]
                    {
                        TargetPlatform.PS4,
                    };
                default: return new TargetPlatform[0];
                }
            }
        }

        private string root;

        private void Build(BuildOptions options, TargetPlatform targetPlatform, TargetArchitecture architecture)
        {
            // Build configuration (see build.cmd -help)
            string configuration = "Release";
            string framework = "net7.0";

            // Clean output directory
            var artifacts = Path.Combine(root, "artifacts");
            SetupDirectory(artifacts, true);

            // Peek options
            string os, arch, runtimeFlavor, subset, hostRuntimeName, buildArgsBase = string.Empty;
            bool setupVersion = false;
            string[] hostRuntimeFiles = Array.Empty<string>();
            var envVars = new Dictionary<string, string>();
            switch (architecture)
            {
            case TargetArchitecture.x86:
                arch = "x86";
                break;
            case TargetArchitecture.x64:
                arch = "x64";
                break;
            case TargetArchitecture.ARM:
                arch = "arm";
                break;
            case TargetArchitecture.ARM64:
                arch = "arm64";
                break;
            default: throw new InvalidArchitectureException(architecture);
            }
            switch (targetPlatform)
            {
            case TargetPlatform.Windows:
                os = "windows";
                runtimeFlavor = "CoreCLR";
                subset = "clr";
                hostRuntimeName = $"win-{arch}";
                break;
            case TargetPlatform.Linux:
                os = "Linux";
                runtimeFlavor = "CoreCLR";
                subset = "clr";
                hostRuntimeName = $"linux-{arch}";
                break;
            case TargetPlatform.Mac:
                os = "OSX";
                runtimeFlavor = "CoreCLR";
                subset = "clr";
                hostRuntimeName = $"osx-{arch}";
                break;
            case TargetPlatform.Android:
                os = "Android";
                runtimeFlavor = "Mono";
                subset = "mono+libs";
                hostRuntimeName = $"android-{arch}";
                break;
            case TargetPlatform.PS4:
                os = "PS4";
                runtimeFlavor = "Mono";
                subset = "mono+libs";
                setupVersion = true;
                buildArgsBase = " /p:RuntimeOS=ps4 -cmakeargs \"-DCLR_CROSS_COMPONENTS_BUILD=1\"";
                hostRuntimeName = $"ps4-{arch}";
                hostRuntimeFiles = new[]
                {
                    "coreclr_delegates.h",
                    "hostfxr.h",
                    "nethost.h",
                    "libnethost.a",
                };
                break;
            default: throw new InvalidPlatformException(targetPlatform);
            }

            // Setup build environment variables for build system
            string buildScript = null;
            switch (BuildPlatform)
            {
            case TargetPlatform.Windows:
            {
                buildScript = "build.cmd";
                if (setupVersion)
                {
                    // Setup _version.c file manually
                    SetupDirectory(Path.Combine(artifacts, "obj"), false);
                    foreach (var file in new[] { "_version.c", "_version.h" })
                        File.Copy(Path.Combine(root, "eng", "native", "version", file), Path.Combine(artifacts, "obj", file), true);
                }
                var msBuild = VCEnvironment.MSBuildPath;
                if (File.Exists(msBuild))
                {
                    envVars.Add("PATH", Path.GetDirectoryName(msBuild));
                }
                break;
            }
            case TargetPlatform.Linux:
            {
                buildScript = "build.sh";
                break;
            }
            case TargetPlatform.Mac:
            {
                buildScript = "build.sh";
                break;
            }
            default: throw new InvalidPlatformException(BuildPlatform);
            }

            // Print the runtime version
            string version;
            {
                var doc = new XmlDocument();
                doc.Load(Path.Combine(root, "eng", "Versions.props"));
                version = doc["Project"]["PropertyGroup"]["ProductVersion"].InnerText;
                Log.Info("Building dotnet/runtime version " + version);
            }

            // Build
            buildArgsBase = $"-os {os} -a {arch} -f {framework} -c {configuration} -lc {configuration} -rc {configuration} -rf {runtimeFlavor}{buildArgsBase}";
            foreach (var buildStep in new[] { subset, "host.pkg", "packs.product" })
            {
                var buildArgs = $"{buildArgsBase} {buildStep}";
                if (BuildPlatform == TargetPlatform.Windows)
                {
                    // For some reason running from Visual Studio fails the build so use command shell
                    //buildArgs = $"/C {buildScript} {buildArgs}";
                    //buildApp = "cmd.exe";
                    // TODO: maybe write command line into bat file and run it here?
                    WinAPI.SetClipboard($"{buildScript} {buildArgs}");
                    WinAPI.MessageBox.Show($"Open console command in folder '{root}' and run command from clipboard. Then close this dialog.", "Run command manually", WinAPI.MessageBox.Buttons.Ok);
                }
                else
                {
                    Utilities.Run(Path.Combine(root, buildScript), buildArgs, null, root, Utilities.RunOptions.ThrowExceptionOnError, envVars);
                }
            }

            // Deploy build products
            var dstBinaries = GetThirdPartyFolder(options, targetPlatform, architecture);
            var srcHostRuntime = Path.Combine(artifacts, "bin", $"{hostRuntimeName}.{configuration}", "corehost");
            foreach (var file in hostRuntimeFiles)
            {
                Utilities.FileCopy(Path.Combine(srcHostRuntime, file), Path.Combine(dstBinaries, file));
            }
            var dstDotnet = Path.Combine(dstBinaries, "Dotnet");
            var dstClassLibrary = Path.Combine(dstDotnet, "shared", "Microsoft.NETCore.App", version);
            SetupDirectory(dstClassLibrary, true);
            foreach (var file in new[]
                     {
                         "LICENSE.TXT",
                         "THIRD-PARTY-NOTICES.TXT",
                     })
            {
                Utilities.FileCopy(Path.Combine(root, file), Path.Combine(dstDotnet, file));
            }
            var srcDotnetLibsPkg = Path.Combine(artifacts, "packages", "Release", "Shipping", $"Microsoft.NETCore.App.Runtime.Mono.{hostRuntimeName}.{version}.nupkg");
            if (!File.Exists(srcDotnetLibsPkg))
                throw new Exception($"Missing .NET Core App class library package at '{srcDotnetLibsPkg}'");
            var srcDotnetLibsPkgTemp = srcDotnetLibsPkg + "Temp";
            using (var zip = new ZipFile(srcDotnetLibsPkg))
            {
                zip.ExtractAll(srcDotnetLibsPkgTemp, ExtractExistingFileAction.OverwriteSilently);
            }
            var privateCorelib = "System.Private.CoreLib.dll";
            Utilities.FileCopy(Path.Combine(srcDotnetLibsPkgTemp, "runtimes", hostRuntimeName, "native", privateCorelib), Path.Combine(dstClassLibrary, privateCorelib));
            Utilities.DirectoryCopy(Path.Combine(srcDotnetLibsPkgTemp, "runtimes", hostRuntimeName, "lib", "net7.0"), dstClassLibrary, false, true);
            Utilities.DirectoriesDelete(srcDotnetLibsPkgTemp);
            // TODO: host/fxr/<version>/hostfxr.dll
            // TODO: shared/Microsoft.NETCore.App/<version>/hostpolicy.dl
            // TODO: shared/Microsoft.NETCore.App/<version>/System.IO.Compression.Native.dll
        }

        /// <inheritdoc />
        public override void Build(BuildOptions options)
        {
            root = options.IntermediateFolder;

            // Ensure to have dependencies installed
            Utilities.Run("ninja", "--version", null, null, Utilities.RunOptions.ThrowExceptionOnError);
            Utilities.Run("cmake", "--version", null, null, Utilities.RunOptions.ThrowExceptionOnError);

            // Get the source
            CloneGitRepo(root, "https://github.com/FlaxEngine/dotnet-runtime.git", "flax-master", null, true);
            SetupDirectory(Path.Combine(root, "src", "external"), false);

            foreach (var platform in options.Platforms)
            {
                var platformData = Path.Combine(GetBinariesFolder(options, platform), "Data", "nethost");
                if (Directory.Exists(platformData))
                    Utilities.DirectoryCopy(platformData, root, true, true);
                switch (platform)
                {
                case TargetPlatform.PS4:
                {
                    Build(options, platform, TargetArchitecture.x64);
                    break;
                }
                }
            }

            // Copy license
            var dstIncludePath = Path.Combine(options.ThirdPartyFolder, "nethost");
            Utilities.FileCopy(Path.Combine(root, "LICENSE.TXT"), Path.Combine(dstIncludePath, "LICENSE.TXT"));
        }
    }
}
