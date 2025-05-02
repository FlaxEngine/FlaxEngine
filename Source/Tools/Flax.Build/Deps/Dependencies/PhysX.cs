// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Xml;
using Flax.Build;
using Flax.Build.Platforms;
using Flax.Build.Projects.VisualStudio;
using Flax.Deploy;

namespace Flax.Deps.Dependencies
{
    /// <summary>
    /// NVIDIA PhysX SDK
    /// </summary>
    /// <seealso cref="Flax.Deps.Dependency" />
    class PhysX : Dependency
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
                        TargetPlatform.Windows,
                        TargetPlatform.XboxOne,
                        TargetPlatform.PS4,
                        TargetPlatform.PS5,
                        TargetPlatform.XboxScarlett,
                        TargetPlatform.Android,
                        TargetPlatform.Switch,
                    };
                case TargetPlatform.Linux:
                    return new[]
                    {
                        TargetPlatform.Linux,
                    };
                case TargetPlatform.Mac:
                    return new[]
                    {
                        TargetPlatform.Mac,
                        TargetPlatform.iOS,
                    };
                default: return new TargetPlatform[0];
                }
            }
        }

        private string root;
        private string projectGenDir;
        private string projectGenPath;
        private string solutionFilesRoot;

        private static void ConfigureCmakeSwitch(XmlElement cmakeSwitches, string name, string value)
        {
            if (cmakeSwitches == null)
                return;
            foreach (XmlElement cmakeSwitch in cmakeSwitches.ChildNodes)
            {
                if (cmakeSwitch.HasAttribute("name") && cmakeSwitch.Attributes["name"].Value == name)
                {
                    cmakeSwitch.Attributes["value"].Value = value;
                }
            }
        }

        private void Build(BuildOptions options, string preset, TargetPlatform targetPlatform, TargetArchitecture architecture)
        {
            // Load preset configuration
            var presetPath = Path.Combine(projectGenDir, "buildtools", "presets", "public", preset + ".xml");
            if (!File.Exists(presetPath))
                throw new Exception(string.Format("Missing PhysX preset {0} (file: {1})", preset, presetPath));
            var presetXml = new XmlDocument();
            presetXml.Load(presetPath);

            // Configure preset
            var cmakeSwitches = presetXml["preset"]["CMakeSwitches"];
            ConfigureCmakeSwitch(cmakeSwitches, "PX_BUILDSNIPPETS", "False");
            ConfigureCmakeSwitch(cmakeSwitches, "PX_BUILDPVDRUNTIME", "False");
            ConfigureCmakeSwitch(cmakeSwitches, "PX_BUILDSAMPLES", "False");
            ConfigureCmakeSwitch(cmakeSwitches, "PX_BUILDPUBLICSAMPLES", "False");
            ConfigureCmakeSwitch(cmakeSwitches, "PX_GENERATE_STATIC_LIBRARIES", "True");
            ConfigureCmakeSwitch(cmakeSwitches, "NV_USE_STATIC_WINCRT", "False");
            ConfigureCmakeSwitch(cmakeSwitches, "NV_USE_DEBUG_WINCRT", "False");
            ConfigureCmakeSwitch(cmakeSwitches, "PX_FLOAT_POINT_PRECISE_MATH", "False");
            var cmakeParams = presetXml["preset"]["CMakeParams"];
            switch (targetPlatform)
            {
            case TargetPlatform.Windows:
                if (architecture == TargetArchitecture.ARM64)
                {
                    // Windows ARM64 doesn't have GPU support, so avoid copying those DLLs around
                    ConfigureCmakeSwitch(cmakeSwitches, "PX_COPY_EXTERNAL_DLL", "OFF");
                    ConfigureCmakeSwitch(cmakeParams, "PX_COPY_EXTERNAL_DLL", "OFF");
                }
                break;
            case TargetPlatform.Android:
                ConfigureCmakeSwitch(cmakeParams, "CMAKE_INSTALL_PREFIX", $"install/android-{Configuration.AndroidPlatformApi}/PhysX");
                ConfigureCmakeSwitch(cmakeParams, "ANDROID_NATIVE_API_LEVEL", $"android-{Configuration.AndroidPlatformApi}");
                ConfigureCmakeSwitch(cmakeParams, "ANDROID_ABI", AndroidToolchain.GetAbiName(architecture));
                break;
            case TargetPlatform.Mac:
                ConfigureCmakeSwitch(cmakeParams, "CMAKE_OSX_DEPLOYMENT_TARGET", Configuration.MacOSXMinVer);
                break;
            case TargetPlatform.iOS:
                ConfigureCmakeSwitch(cmakeParams, "CMAKE_OSX_DEPLOYMENT_TARGET", Configuration.iOSMinVer);
                ConfigureCmakeSwitch(cmakeParams, "CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET", Configuration.iOSMinVer);
                break;
            }

            // Save preset
            presetXml.Save(presetPath);

            // Peek options
            var platform = Platform.GetPlatform(targetPlatform);
            var configuration = "release";
            string bits;
            string arch;
            string binariesSubDir;
            string buildPlatform;
            bool suppressBitsPostfix = false;
            string binariesPrefix = string.Empty;
            var envVars = new Dictionary<string, string>();
            switch (architecture)
            {
            case TargetArchitecture.x86:
                arch = "x86";
                bits = "32";
                break;
            case TargetArchitecture.x64:
                arch = "x86";
                bits = "64";
                break;
            case TargetArchitecture.ARM:
                arch = "arm";
                bits = "32";
                break;
            case TargetArchitecture.ARM64:
                arch = "arm";
                bits = "64";
                break;
            default: throw new InvalidArchitectureException(architecture);
            }
            switch (architecture)
            {
            case TargetArchitecture.x86:
                buildPlatform = "Win32";
                break;
            default:
                buildPlatform = architecture.ToString();
                break;
            }
            var msBuildProps = new Dictionary<string, string>();
            switch (targetPlatform)
            {
            case TargetPlatform.Windows:
                binariesSubDir = string.Format("win.{0}_{1}.vc143.md", arch, bits);
                break;
            case TargetPlatform.Linux:
                binariesSubDir = "linux.clang";
                binariesPrefix = "lib";
                break;
            case TargetPlatform.PS4:
                binariesSubDir = "ps4";
                buildPlatform = "ORBIS";
                suppressBitsPostfix = true;
                binariesPrefix = "lib";
                break;
            case TargetPlatform.PS5:
                binariesSubDir = "ps5";
                buildPlatform = "PROSPERO";
                suppressBitsPostfix = true;
                binariesPrefix = "lib";
                break;
            case TargetPlatform.XboxOne:
            case TargetPlatform.XboxScarlett:
                binariesSubDir = "win.x86_64.vc142.md";
                break;
            case TargetPlatform.Android:
                switch (architecture)
                {
                case TargetArchitecture.ARM64:
                    binariesSubDir = "android.arm64-v8a";
                    break;
                default: throw new InvalidArchitectureException(architecture);
                }
                binariesPrefix = "lib";
                break;
            case TargetPlatform.Switch:
                binariesSubDir = "switch64";
                buildPlatform = "NX64";
                suppressBitsPostfix = true;
                binariesPrefix = "lib";
                envVars.Add("NintendoSdkRoot", Sdk.Get("SwitchSdk").RootPath + '\\');
                msBuildProps.Add("NintendoSdkRoot", envVars["NintendoSdkRoot"]);
                break;
            case TargetPlatform.Mac:
                switch (architecture)
                {
                case TargetArchitecture.x64:
                    binariesSubDir = "mac.x86_64";
                    break;
                case TargetArchitecture.ARM64:
                    binariesSubDir = "mac.arm64";
                    break;
                default: throw new InvalidArchitectureException(architecture);
                }
                binariesPrefix = "lib";
                envVars.Add("MACOSX_DEPLOYMENT_TARGET", Configuration.MacOSXMinVer);
                break;
            case TargetPlatform.iOS:
                binariesSubDir = "ios.arm_64";
                binariesPrefix = "lib";
                envVars.Add("IPHONEOS_DEPLOYMENT_TARGET", Configuration.iOSMinVer);
                break;
            default: throw new InvalidPlatformException(targetPlatform);
            }

            // Setup build environment variables for PhysX build system
            string msBuild = null;
            switch (BuildPlatform)
            {
            case TargetPlatform.Windows:
            {
                GetMsBuildForPlatform(targetPlatform, out var vsVersion, out msBuild);
                // TODO: override VS version in cmake_generate_projects.py too
                if (File.Exists(msBuild))
                {
                    envVars.Add("PATH", Path.GetDirectoryName(msBuild));
                }
                break;
            }
            case TargetPlatform.Linux:
                envVars.Add("CC", "clang-7");
                envVars.Add("CC_FOR_BUILD", "clang-7");
                break;
            case TargetPlatform.Mac: break;
            default: throw new InvalidPlatformException(BuildPlatform);
            }
            if (AndroidNdk.Instance.IsValid)
            {
                envVars.Add("ANDROID_NDK_HOME", AndroidNdk.Instance.RootPath);
                envVars.Add("PM_ANDROIDNDK_PATH", AndroidNdk.Instance.RootPath);
            }

            // Update packman for old PhysX version (https://github.com/NVIDIA-Omniverse/PhysX/issues/229)
            if (BuildPlatform == TargetPlatform.Windows)
                Utilities.Run(Path.Combine(projectGenDir, "buildtools", "packman", "packman.cmd"), "update -y");
            else
                Utilities.Run(Path.Combine(projectGenDir, "buildtools", "packman", "packman"), "update -y");

            // Print the PhysX version
            Log.Info("Building PhysX version " + File.ReadAllText(Path.Combine(root, "physx", "version.txt")) + " to " + binariesSubDir);

            // Generate project files
            Utilities.Run(projectGenPath, preset, null, projectGenDir, Utilities.RunOptions.ThrowExceptionOnError, envVars);

            switch (targetPlatform)
            {
            case TargetPlatform.PS4:
            case TargetPlatform.PS5:
            case TargetPlatform.Switch:
                // Hack: Platform compiler uses .o extension for compiler output files but CMake uses .obj even if CMAKE_CXX_OUTPUT_EXTENSION/CMAKE_C_OUTPUT_EXTENSION are specified
                Utilities.ReplaceInFiles(Path.Combine(root, "physx\\compiler\\" + binariesSubDir), "*.vcxproj", SearchOption.AllDirectories, ".obj", ".o");
                break;
            case TargetPlatform.XboxOne:
            case TargetPlatform.XboxScarlett:
                // Hack: force to use proper Win10 SDK
                Utilities.ReplaceInFiles(Path.Combine(root, "physx\\compiler", preset), "*.vcxproj", SearchOption.AllDirectories, "10.0.18362.0", "10.0.19041.0");
                break;
            }

            // Run building based on the platform
            var defaultPhysXLibs = new[]
            {
                "PhysX",
                "PhysXCharacterKinematic",
                "PhysXCommon",
                "PhysXCooking",
                "PhysXExtensions",
                "PhysXFoundation",
                "PhysXPvdSDK",
                "PhysXVehicle",
                "PhysXVehicle2",
            };
            var dstBinaries = GetThirdPartyFolder(options, targetPlatform, architecture);
            var srcBinaries = Path.Combine(root, "physx", "bin", binariesSubDir, configuration);
            switch (BuildPlatform)
            {
            case TargetPlatform.Windows:
                switch (targetPlatform)
                {
                case TargetPlatform.Android:
                    Utilities.Run("cmake", "--build .", null, Path.Combine(root, "physx\\compiler\\android-" + configuration), Utilities.RunOptions.ConsoleLogOutput, envVars);
                    break;
                default:
                    VCEnvironment.BuildSolution(Path.Combine(solutionFilesRoot, preset, "PhysXSDK.sln"), configuration, buildPlatform, msBuildProps, msBuild);
                    break;
                }
                break;
            case TargetPlatform.Linux:
                Utilities.Run("make", null, null, Path.Combine(projectGenDir, "compiler", "linux-" + configuration), Utilities.RunOptions.ConsoleLogOutput);
                break;
            case TargetPlatform.Mac:
                Utilities.Run("xcodebuild", "-project PhysXSDK.xcodeproj -alltargets -configuration " + configuration, null, Path.Combine(projectGenDir, "compiler", preset), Utilities.RunOptions.ConsoleLogOutput);
                break;
            default: throw new InvalidPlatformException(BuildPlatform);
            }

            // Deploy binaries
            var binariesExtension = platform.StaticLibraryFileExtension;
            Log.Verbose("Copy PhysX binaries from " + srcBinaries);
            foreach (var physXLib in defaultPhysXLibs)
            {
                var filename = suppressBitsPostfix ? string.Format("{0}{1}_static", binariesPrefix, physXLib) : string.Format("{0}{1}_static_{2}", binariesPrefix, physXLib, bits);
                filename += binariesExtension;
                Utilities.FileCopy(Path.Combine(srcBinaries, filename), Path.Combine(dstBinaries, filename));

                var filenamePdb = Path.ChangeExtension(filename, "pdb");
                if (File.Exists(Path.Combine(srcBinaries, filenamePdb)))
                    Utilities.FileCopy(Path.Combine(srcBinaries, filenamePdb), Path.Combine(dstBinaries, filenamePdb));

                // Strip debug symbols to reduce binaries size
                switch (BuildPlatform)
                {
                case TargetPlatform.Linux:
                case TargetPlatform.Mac:
                    switch (targetPlatform)
                    {
                    case TargetPlatform.Mac:
                    case TargetPlatform.Android:
                        Utilities.Run("strip", "\"" + filename + "\"", null, dstBinaries, Utilities.RunOptions.ConsoleLogOutput);
                        break;
                    }
                    break;
                }
            }
            srcBinaries = Path.Combine(root, "physx", "compiler", preset, "sdk_source_bin", configuration);
            var additionalPhysXLibs = new[]
            {
                "FastXml",
                "LowLevel",
                "LowLevelAABB",
                "LowLevelDynamics",
                "PhysXTask",
                "SceneQuery",
                "SimulationController",
            };
            foreach (var additionalPhysXLib in additionalPhysXLibs)
            {
                var filenamePdb = suppressBitsPostfix ? string.Format("{0}{1}", binariesPrefix, additionalPhysXLib) : string.Format("{0}{1}_{2}", binariesPrefix, additionalPhysXLib, bits);
                filenamePdb += ".pdb";
                if (File.Exists(Path.Combine(srcBinaries, filenamePdb)))
                    Utilities.FileCopy(Path.Combine(srcBinaries, filenamePdb), Path.Combine(dstBinaries, filenamePdb));
            }
        }

        /// <inheritdoc />
        public override void Build(BuildOptions options)
        {
            root = options.IntermediateFolder;
            projectGenDir = Path.Combine(root, "physx");
            solutionFilesRoot = Path.Combine(root, "physx", "compiler");
            switch (BuildPlatform)
            {
            case TargetPlatform.Windows:
                projectGenPath = Path.Combine(projectGenDir, "generate_projects.bat");
                break;
            case TargetPlatform.Linux:
            case TargetPlatform.Mac:
                projectGenPath = Path.Combine(projectGenDir, "generate_projects.sh");
                break;
            default: throw new InvalidPlatformException(BuildPlatform);
            }

            // Get the source
            CloneGitRepoSingleBranch(root, "https://github.com/FlaxEngine/PhysX-5.git", "flax-master");

            foreach (var platform in options.Platforms)
            {
                BuildStarted(platform);
                switch (platform)
                {
                case TargetPlatform.Windows:
                {
                    Build(options, "vc17win64", platform, TargetArchitecture.x64);
                    Build(options, "vc17win-arm64", platform, TargetArchitecture.ARM64);
                    break;
                }
                case TargetPlatform.Linux:
                {
                    Build(options, "linux", platform, TargetArchitecture.x64);
                    break;
                }
                case TargetPlatform.PS4:
                {
                    Utilities.DirectoryCopy(Path.Combine(GetBinariesFolder(options, platform), "Data", "PhysX"), root, true, true);
                    Build(options, "ps4", platform, TargetArchitecture.x64);
                    break;
                }
                case TargetPlatform.PS5:
                {
                    Utilities.DirectoryCopy(Path.Combine(GetBinariesFolder(options, platform), "Data", "PhysX"), root, true, true);
                    Build(options, "ps5", platform, TargetArchitecture.x64);
                    break;
                }
                case TargetPlatform.XboxScarlett:
                case TargetPlatform.XboxOne:
                {
                    Build(options, "vc16win64", platform, TargetArchitecture.x64);
                    break;
                }
                case TargetPlatform.Android:
                {
                    Build(options, "android", platform, TargetArchitecture.ARM64);
                    break;
                }
                case TargetPlatform.Switch:
                {
                    Utilities.DirectoryCopy(Path.Combine(GetBinariesFolder(options, platform), "Data", "PhysX"), root, true, true);
                    Build(options, "switch64", platform, TargetArchitecture.ARM64);
                    break;
                }
                case TargetPlatform.Mac:
                {
                    Build(options, "mac64", platform, TargetArchitecture.x64);
                    Build(options, "mac-arm64", platform, TargetArchitecture.ARM64);
                    break;
                }
                case TargetPlatform.iOS:
                {
                    Build(options, "ios64", platform, TargetArchitecture.ARM64);
                    break;
                }
                }
            }

            // Copy header files
            var dstIncludePath = Path.Combine(options.ThirdPartyFolder, "PhysX");
            Directory.GetFiles(dstIncludePath, "*.h", SearchOption.AllDirectories).ToList().ForEach(File.Delete);
            Utilities.FileCopy(Path.Combine(root, "LICENSE.md"), Path.Combine(dstIncludePath, "License.txt"));
            Utilities.DirectoryCopy(Path.Combine(root, "physx", "include"), dstIncludePath);
        }
    }
}
