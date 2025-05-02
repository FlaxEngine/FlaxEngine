// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Xml;
using Flax.Build;
using Flax.Build.Platforms;
using Flax.Deploy;
using System.IO.Compression;

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
                        TargetPlatform.PS5,
                        TargetPlatform.Switch,
                        TargetPlatform.XboxOne,
                        TargetPlatform.XboxScarlett,
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
        private bool cleanArtifacts;

        private void Build(BuildOptions options, TargetPlatform targetPlatform, TargetArchitecture architecture)
        {
            // Build configuration (see build.cmd -help)
            string configuration = "Release";
            string framework = "net8.0";

            // Clean output directory
            var artifacts = Path.Combine(root, "artifacts");
            SetupDirectory(artifacts, cleanArtifacts);
            cleanArtifacts = true;

            // Peek options
            string os, arch, runtimeFlavor, hostRuntimeName = DotNetSdk.GetHostRuntimeIdentifier(targetPlatform, architecture), buildArgs = string.Empty, buildMonoAotCrossArgs = string.Empty;
            bool setupVersion = false, buildMonoAotCross = false;
            var envVars = new Dictionary<string, string>();
            envVars.Add("VisualStudioVersion", null); // Unset this so 'init-vs-env.cmd' will properly init VS Environment
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
                break;
            case TargetPlatform.XboxOne:
            case TargetPlatform.XboxScarlett:
                os = "windows";
                runtimeFlavor = "Mono";
                buildMonoAotCross = true;
                buildArgs = $" -subset mono+libs -cmakeargs \"-DDISABLE_JIT=1-DENABLE_PERFTRACING=0-DDISABLE_REFLECTION_EMIT=1-DDISABLE_EVENTPIPE=1-DDISABLE_COM=1-DDISABLE_PROFILER=1-DDISABLE_COMPONENTS=1\" /p:FeaturePerfTracing=false /p:FeatureManagedEtw=false /p:FeatureManagedEtwChannels=false /p:FeatureEtw=false /p:ApiCompatValidateAssemblies=false";
                break;
            case TargetPlatform.Linux:
                os = "linux";
                runtimeFlavor = "CoreCLR";
                break;
            case TargetPlatform.Mac:
                os = "osx";
                runtimeFlavor = "CoreCLR";
                break;
            case TargetPlatform.Android:
                os = "android";
                runtimeFlavor = "Mono";
                break;
            case TargetPlatform.PS4:
            case TargetPlatform.PS5:
            case TargetPlatform.Switch:
                runtimeFlavor = "Mono";
                setupVersion = true;
                buildMonoAotCross = true;
                switch (targetPlatform)
                {
                case TargetPlatform.PS4:
                    os = "ps4";
                    break;
                case TargetPlatform.PS5:
                    os = "ps5";
                    break;
                case TargetPlatform.Switch:
                    os = "switch";
                    break;
                default: throw new InvalidPlatformException(targetPlatform);
                }
                buildArgs = $" /p:RuntimeOS={os} -subset mono+libs -cmakeargs \"-DDISABLE_JIT=1-DENABLE_PERFTRACING=0-DDISABLE_REFLECTION_EMIT=1-DDISABLE_EVENTPIPE=1-DDISABLE_COM=1-DDISABLE_PROFILER=1-DDISABLE_COMPONENTS=1\" /p:FeaturePerfTracing=false /p:FeatureManagedEtw=false /p:FeatureManagedEtwChannels=false /p:FeatureEtw=false /p:ApiCompatValidateAssemblies=false";
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
            if (runtimeFlavor == "CoreCLR")
                buildArgs = $"-os {os} -a {arch} -f {framework} -c {configuration} -lc {configuration} -rc {configuration} -rf {runtimeFlavor}{buildArgs}";
            else if (runtimeFlavor == "Mono")
                buildArgs = $"-os {os} -a {arch} -c {configuration} -rf {runtimeFlavor}{buildArgs}";
            Utilities.Run(Path.Combine(root, buildScript), buildArgs, null, root, Utilities.RunOptions.DefaultTool, envVars);
            if (buildMonoAotCross)
            {
                buildMonoAotCrossArgs = $"-c {configuration} -rf {runtimeFlavor} -subset mono /p:BuildMonoAotCrossCompiler=true /p:BuildMonoAOTCrossCompilerOnly=true /p:TargetOS={os} /p:HostOS=windows -cmakeargs \"-DCMAKE_CROSSCOMPILING=True\"{buildMonoAotCrossArgs}";
                Utilities.Run(Path.Combine(root, buildScript), buildMonoAotCrossArgs, null, root, Utilities.RunOptions.DefaultTool, envVars);
            }

            // Deploy build products
            var privateCoreLib = "System.Private.CoreLib.dll";
            var dstBinaries = GetThirdPartyFolder(options, targetPlatform, architecture);
            var dstPlatform = Path.Combine(options.PlatformsFolder, targetPlatform.ToString());
            var dstDotnet = Path.Combine(dstPlatform, "Dotnet");
            foreach (var file in new[] { "LICENSE.TXT", "THIRD-PARTY-NOTICES.TXT" })
                Utilities.FileCopy(Path.Combine(root, file), Path.Combine(dstDotnet, file));
            if (runtimeFlavor == "CoreCLR")
            {
                var srcHostRuntime = Path.Combine(artifacts, "bin", $"{hostRuntimeName}.{configuration}", "corehost");
                var dstClassLibrary = Path.Combine(dstDotnet, "shared", "Microsoft.NETCore.App", version);
                SetupDirectory(dstClassLibrary, true);
                var srcDotnetLibsPkg = Path.Combine(artifacts, "packages", "Release", "Shipping", $"Microsoft.NETCore.App.Runtime.Mono.{hostRuntimeName}.{version}.nupkg");
                if (!File.Exists(srcDotnetLibsPkg))
                    throw new Exception($"Missing .NET Core App class library package at '{srcDotnetLibsPkg}'");
                var unpackTemp = Path.Combine(Path.GetDirectoryName(srcDotnetLibsPkg), "UnpackTemp");
                SetupDirectory(unpackTemp, true);
                using (var zip = ZipFile.Open(srcDotnetLibsPkg, ZipArchiveMode.Read))
                    zip.ExtractToDirectory(unpackTemp);
                Utilities.FileCopy(Path.Combine(unpackTemp, "runtimes", hostRuntimeName, "native", privateCoreLib), Path.Combine(dstClassLibrary, privateCoreLib));
                Utilities.DirectoryCopy(Path.Combine(unpackTemp, "runtimes", hostRuntimeName, "lib", framework), dstClassLibrary, false, true);
                // TODO: host/fxr/<version>/hostfxr.dll
                // TODO: shared/Microsoft.NETCore.App/<version>/hostpolicy.dl
                // TODO: shared/Microsoft.NETCore.App/<version>/System.IO.Compression.Native.dll
                Utilities.DirectoryCopy(Path.Combine(unpackTemp, "runtimes", hostRuntimeName, "native"), Path.Combine(dstDotnet, "native"), true, true);
                Utilities.FileDelete(Path.Combine(dstDotnet, "native", privateCoreLib));
                Utilities.DirectoriesDelete(unpackTemp);
            }
            else if (runtimeFlavor == "Mono")
            {
                // Native libs
                var src1 = Path.Combine(artifacts, "obj", "mono", $"{os}.{arch}.{configuration}", "out");
                if (!Directory.Exists(src1))
                    throw new DirectoryNotFoundException(src1);
                var src2 = Path.Combine(artifacts, "bin", "native", $"{framework}-{os}-{configuration}-{arch}");
                if (!Directory.Exists(src2))
                    throw new DirectoryNotFoundException(src2);
                string[] libs1, libs2;
                switch (targetPlatform)
                {
                case TargetPlatform.Windows:
                case TargetPlatform.XboxOne:
                case TargetPlatform.XboxScarlett:
                    libs1 = new[]
                    {
                        "lib/coreclr.dll",
                        "lib/coreclr.import.lib",
                    };
                    libs2 = new string[]
                    {
                    };
                    break;
                default:
                    libs1 = new[]
                    {
                        "lib/libmonosgen-2.0.a",
                        "lib/libmono-profiler-aot.a",
                    };
                    libs2 = new[]
                    {
                        "lib/libSystem.Globalization.Native.a",
                        "lib/libSystem.IO.Compression.Native.a",
                        "lib/libSystem.IO.Ports.Native.a",
                        "lib/libSystem.Native.a",
                    };
                    break;
                }
                foreach (var file in libs1)
                    Utilities.FileCopy(Path.Combine(src1, file), Path.Combine(dstBinaries, Path.GetFileName(file)));
                foreach (var file in libs2)
                    Utilities.FileCopy(Path.Combine(src2, file), Path.Combine(dstBinaries, Path.GetFileName(file)));

                // Include headers
                Utilities.DirectoryDelete(Path.Combine(dstBinaries, "include"));
                Utilities.DirectoryCopy(Path.Combine(src1, "include"), Path.Combine(dstBinaries, "include"), true, true);

                if (buildMonoAotCross)
                {
                    // AOT compiler
                    Utilities.FileCopy(Path.Combine(artifacts, "bin", "mono", $"{os}.x64.{configuration}", "cross", $"{(os == "windows" ? "win" : os)}-x64", "mono-aot-cross.exe"), Path.Combine(dstPlatform, "Binaries", "Tools", "mono-aot-cross.exe"));
                }

                // Class library
                var dstDotnetLib = Path.Combine(dstPlatform, "Dotnet", "lib", framework);
                foreach (var subDir in Directory.GetDirectories(Path.Combine(dstPlatform, "Dotnet", "lib")))
                    Utilities.DirectoryDelete(subDir);
                SetupDirectory(dstDotnetLib, true);
                Utilities.FileCopy(Path.Combine(artifacts, "bin", "mono", $"{os}.{arch}.{configuration}", privateCoreLib), Path.Combine(dstDotnetLib, privateCoreLib));
                Utilities.DirectoryCopy(Path.Combine(artifacts, "bin", "runtime", $"{framework}-{os}-{configuration}-{arch}"), dstDotnetLib, false, true, "*.dll");
            }
        }

        /// <inheritdoc />
        public override void Build(BuildOptions options)
        {
            root = options.IntermediateFolder;

            // Ensure to have dependencies installed
            Utilities.Run("ninja", "--version", null, null, Utilities.RunOptions.ThrowExceptionOnError);
            Utilities.Run("cmake", "--version", null, null, Utilities.RunOptions.ThrowExceptionOnError);

            // Get the source
            if (!Directory.Exists(Path.Combine(root, ".git")))
            {
                CloneGitRepo(root, "https://github.com/FlaxEngine/dotnet-runtime.git", null, null, true);
                GitCheckout(root, "flax-master-8");
                SetupDirectory(Path.Combine(root, "src", "external"), false);
            }

            /*
             * Mono AOT for Windows:
             * .\build.cmd -c release -runtimeFlavor mono -subset mono+libs -cmakeargs "-DDISABLE_JIT=1-DENABLE_PERFTRACING=0-DDISABLE_REFLECTION_EMIT=1-DDISABLE_EVENTPIPE=1-DDISABLE_COM=1-DDISABLE_PROFILER=1-DDISABLE_COMPONENTS=1" /p:FeaturePerfTracing=false /p:FeatureManagedEtw=false /p:FeatureManagedEtwChannels=false /p:FeatureEtw=false
             *
             * Mono AOT cross-compiler for Windows:
             * .\build.cmd -c release -runtimeFlavor mono -subset mono /p:BuildMonoAotCrossCompiler=true /p:BuildMonoAOTCrossCompilerOnly=true
             *
             * Mono AOT for PS4:
             * .\build.cmd -os ps4 -a x64 /p:RuntimeOS=ps4 -c release -runtimeFlavor mono -subset mono+libs -cmakeargs "-DDISABLE_JIT=1-DENABLE_PERFTRACING=0-DDISABLE_REFLECTION_EMIT=1-DDISABLE_EVENTPIPE=1-DDISABLE_COM=1-DDISABLE_PROFILER=1-DDISABLE_COMPONENTS=1" /p:FeaturePerfTracing=false /p:FeatureManagedEtw=false /p:FeatureManagedEtwChannels=false /p:FeatureEtw=false /p:ApiCompatValidateAssemblies=false
             *
             * Mono AOT cross-compiler for PS4:
             * .\build.cmd -c release -runtimeFlavor mono -subset mono /p:BuildMonoAotCrossCompiler=true /p:BuildMonoAOTCrossCompilerOnly=true /p:TargetOS=ps4 /p:HostOS=windows -cmakeargs "-DCMAKE_CROSSCOMPILING=True"
             */

            foreach (var platform in options.Platforms)
            {
                BuildStarted(platform);
                var platformData = Path.Combine(GetBinariesFolder(options, platform), "Data", "nethost");
                if (Directory.Exists(platformData))
                    Utilities.DirectoryCopy(platformData, root, true, true);
                switch (platform)
                {
                case TargetPlatform.PS4:
                case TargetPlatform.PS5:
                case TargetPlatform.XboxOne:
                case TargetPlatform.XboxScarlett:
                    Build(options, platform, TargetArchitecture.x64);
                    break;
                case TargetPlatform.Android:
                    Build(options, platform, TargetArchitecture.ARM64);
                    break;
                case TargetPlatform.Switch:
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
