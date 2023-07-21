// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Xml;
using Flax.Build;
using Flax.Build.Platforms;
using Flax.Deploy;
using System.IO.Compression;

#pragma warning disable 0219

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
                case TargetPlatform.Linux:
                    return new[]
                    {
                        TargetPlatform.Android,
                    };
                default: return new TargetPlatform[0];
                }
            }
        }

        /// <inheritdoc />
        public override bool BuildByDefault => false;

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
            string os, arch, runtimeFlavor, subset, hostRuntimeName = DotNetSdk.GetHostRuntimeIdentifier(targetPlatform, architecture), buildArgsBase = string.Empty;
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
                break;
            case TargetPlatform.Linux:
                os = "Linux";
                runtimeFlavor = "CoreCLR";
                subset = "clr";
                break;
            case TargetPlatform.Mac:
                os = "OSX";
                runtimeFlavor = "CoreCLR";
                subset = "clr";
                break;
            case TargetPlatform.Android:
                os = "Android";
                runtimeFlavor = "Mono";
                subset = "mono+libs";
                break;
            case TargetPlatform.PS4:
                os = "PS4";
                runtimeFlavor = "Mono";
                subset = "mono+libs";
                setupVersion = true;
                buildArgsBase = " /p:RuntimeOS=ps4 -cmakeargs \"-DCLR_CROSS_COMPONENTS_BUILD=1\"";
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
            if (AndroidNdk.Instance.IsValid)
            {
                var path = AndroidNdk.Instance.RootPath;
                envVars.Add("ANDROID_NDK", path);
                envVars.Add("ANDROID_NDK_HOME", path);
                envVars.Add("ANDROID_NDK_ROOT", path);
            }
            if (AndroidSdk.Instance.IsValid)
            {
                var path = AndroidSdk.Instance.RootPath;
                envVars.Add("ANDROID_SDK", path);
                envVars.Add("ANDROID_SDK_HOME", path);
                envVars.Add("ANDROID_SDK_ROOT", path);
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
            //foreach (var buildStep in new[] { subset, "host.pkg", "packs.product" })
            /*var buildStep = "host.pkg";
            {
                var buildArgs = $"{buildArgsBase} -s {buildStep}";
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
                    //Utilities.Run(Path.Combine(root, buildScript), buildArgs, null, root, Utilities.RunOptions.ThrowExceptionOnError, envVars);
                }
            }*/
            Utilities.Run(Path.Combine(root, buildScript), buildArgsBase, null, root, Utilities.RunOptions.ThrowExceptionOnError, envVars);

            // Deploy build products
            var dstBinaries = GetThirdPartyFolder(options, targetPlatform, architecture);
            var srcHostRuntime = Path.Combine(artifacts, "bin", $"{hostRuntimeName}.{configuration}", "corehost");
            foreach (var file in hostRuntimeFiles)
            {
                Utilities.FileCopy(Path.Combine(srcHostRuntime, file), Path.Combine(dstBinaries, file));
            }
            var dstDotnet = Path.Combine(GetBinariesFolder(options, targetPlatform), "Dotnet");
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
            var unpackTemp = Path.Combine(Path.GetDirectoryName(srcDotnetLibsPkg), "UnpackTemp");
            SetupDirectory(unpackTemp, true);
            using (var zip = ZipFile.Open(srcDotnetLibsPkg, ZipArchiveMode.Read))
            {
                zip.ExtractToDirectory(unpackTemp);
            }
            var privateCorelib = "System.Private.CoreLib.dll";
            Utilities.FileCopy(Path.Combine(unpackTemp, "runtimes", hostRuntimeName, "native", privateCorelib), Path.Combine(dstClassLibrary, privateCorelib));
            Utilities.DirectoryCopy(Path.Combine(unpackTemp, "runtimes", hostRuntimeName, "lib", "net7.0"), dstClassLibrary, false, true);
            // TODO: host/fxr/<version>/hostfxr.dll
            // TODO: shared/Microsoft.NETCore.App/<version>/hostpolicy.dl
            // TODO: shared/Microsoft.NETCore.App/<version>/System.IO.Compression.Native.dll
            if (runtimeFlavor == "Mono")
            {
                // TODO: implement automatic deployment based on the setup:
                // PS4 outputs mono into artifacts\obj\mono\PS4.x64.Release\out
                // PS4 outputs native libs into artifacts\bin\native\net7.0-PS4-Release-x64\lib
                // PS4 outputs System.Private.CoreLib lib into artifacts\bin\mono\PS4.x64.Release
                // PS4 outputs C# libs into artifacts\bin\runtime\net7.0-PS4.Release.x64
                // PS4 outputs AOT compiler into artifacts\bin\mono\PS4.x64.Release\cross\ps4-x64
                Utilities.DirectoryCopy(Path.Combine(unpackTemp, "runtimes", hostRuntimeName, "native"), Path.Combine(dstDotnet, "native"), true, true);
                Utilities.FileDelete(Path.Combine(dstDotnet, "native", privateCorelib));
            }
            else
                throw new InvalidPlatformException(targetPlatform);
            Utilities.DirectoriesDelete(unpackTemp);
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

            /*
             * Mono AOT for Windows:
             * .\build.cmd -c release -runtimeFlavor mono -subset mono+libs -cmakeargs "-DDISABLE_JIT=1-DENABLE_PERFTRACING=0-DDISABLE_REFLECTION_EMIT=1-DDISABLE_EVENTPIPE=1-DDISABLE_COM=1-DDISABLE_PROFILER=1-DDISABLE_COMPONENTS=1" /p:FeaturePerfTracing=false /p:FeatureManagedEtw=false /p:FeatureManagedEtwChannels=false /p:FeatureEtw=false
             *
             * Mono AOT cross-compiler for Windows:
             * .\build.cmd -c release -runtimeFlavor mono -subset mono /p:BuildMonoAotCrossCompiler=true /p:BuildMonoAOTCrossCompilerOnly=true
             *
             * Mono AOT for PS4:
             * .\build.cmd -os PS4 -a x64 /p:RuntimeOS=ps4 -c release -runtimeFlavor mono -subset mono+libs -cmakeargs "-DDISABLE_JIT=1-DENABLE_PERFTRACING=0-DDISABLE_REFLECTION_EMIT=1-DDISABLE_EVENTPIPE=1-DDISABLE_COM=1-DDISABLE_PROFILER=1-DDISABLE_COMPONENTS=1" /p:FeaturePerfTracing=false /p:FeatureManagedEtw=false /p:FeatureManagedEtwChannels=false /p:FeatureEtw=false
             *
             * Mono AOT cross-compiler for PS4:
             * .\build.cmd -c release -runtimeFlavor mono -subset mono /p:BuildMonoAotCrossCompiler=true /p:BuildMonoAOTCrossCompilerOnly=true /p:TargetOS=PS4 /p:HostOS=windows -cmakeargs "-DCMAKE_CROSSCOMPILING=True"
             */

            foreach (var platform in options.Platforms)
            {
                var platformData = Path.Combine(GetBinariesFolder(options, platform), "Data", "nethost");
                if (Directory.Exists(platformData))
                    Utilities.DirectoryCopy(platformData, root, true, true);
                switch (platform)
                {
                case TargetPlatform.PS4:
                case TargetPlatform.PS5:
                    Build(options, platform, TargetArchitecture.x64);
                    break;
                case TargetPlatform.Android:
                    Build(options, platform, TargetArchitecture.ARM64);
                    break;
                }
            }

            // Copy license
            var dstIncludePath = Path.Combine(options.ThirdPartyFolder, "nethost");
            Utilities.FileCopy(Path.Combine(root, "LICENSE.TXT"), Path.Combine(dstIncludePath, "LICENSE.TXT"));
        }
    }
}
