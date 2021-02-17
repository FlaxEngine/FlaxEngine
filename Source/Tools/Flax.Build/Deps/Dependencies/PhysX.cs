// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Xml;
using Flax.Build;
using Flax.Build.Platforms;
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
                        TargetPlatform.UWP,
                        TargetPlatform.XboxOne,
                        TargetPlatform.PS4,
                        TargetPlatform.XboxScarlett,
                        TargetPlatform.Android,
                    };
                case TargetPlatform.Linux:
                    return new[]
                    {
                        TargetPlatform.Linux,
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
            ConfigureCmakeSwitch(cmakeSwitches, "PX_BUILDSAMPLES", "False");
            ConfigureCmakeSwitch(cmakeSwitches, "PX_BUILDPUBLICSAMPLES", "False");
            ConfigureCmakeSwitch(cmakeSwitches, "PX_GENERATE_STATIC_LIBRARIES", "True");
            ConfigureCmakeSwitch(cmakeSwitches, "NV_USE_STATIC_WINCRT", "False");
            ConfigureCmakeSwitch(cmakeSwitches, "NV_USE_DEBUG_WINCRT", "False");
            ConfigureCmakeSwitch(cmakeSwitches, "PX_FLOAT_POINT_PRECISE_MATH", "False");
            var cmakeParams = presetXml["preset"]["CMakeParams"];
            switch (targetPlatform)
            {
            case TargetPlatform.Android:
                ConfigureCmakeSwitch(cmakeParams, "CMAKE_INSTALL_PREFIX", $"install/android-{Configuration.AndroidPlatformApi}/PhysX");
                ConfigureCmakeSwitch(cmakeParams, "ANDROID_NATIVE_API_LEVEL", $"android-{Configuration.AndroidPlatformApi}");
                ConfigureCmakeSwitch(cmakeParams, "ANDROID_ABI", AndroidToolchain.GetAbiName(architecture));
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

            switch (targetPlatform)
            {
            case TargetPlatform.Windows:
                binariesSubDir = string.Format("win.{0}_{1}.vc140.md", arch, bits);
                break;
            case TargetPlatform.XboxOne:
            case TargetPlatform.UWP:
                binariesSubDir = string.Format("uwp.{0}_{1}.vc141", arch, bits);
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
            case TargetPlatform.XboxScarlett:
                binariesSubDir = "win.x86_64.vc142.md";
                break;
            case TargetPlatform.Android:
                switch (architecture)
                {
                case TargetArchitecture.ARM64:
                    binariesSubDir = "android.arm64-v8a.fp-soft";
                    break;
                default: throw new InvalidArchitectureException(architecture);
                }
                binariesPrefix = "lib";
                suppressBitsPostfix = true;
                break;
            default: throw new InvalidPlatformException(targetPlatform);
            }

            // Setup build environment variables for PhysX build system
            var envVars = new Dictionary<string, string>();
            switch (BuildPlatform)
            {
            case TargetPlatform.Windows:
            {
                var msBuild = VCEnvironment.MSBuildPath;
                if (File.Exists(msBuild))
                {
                    envVars.Add("PATH", Path.GetDirectoryName(msBuild));
                }
                break;
            }
            case TargetPlatform.Linux:
            {
                envVars.Add("CC", "clang-7");
                envVars.Add("CC_FOR_BUILD", "clang-7");
                break;
            }
            default: throw new InvalidPlatformException(BuildPlatform);
            }
            if (AndroidNdk.Instance.IsValid)
                envVars.Add("PM_ANDROIDNDK_PATH", AndroidNdk.Instance.RootPath);

            // Print the PhysX version
            Log.Info("Building PhysX version " + File.ReadAllText(Path.Combine(root, "physx", "version.txt")) + " to " + binariesSubDir);

            // Generate project files
            Utilities.Run(projectGenPath, preset, null, projectGenDir, Utilities.RunOptions.Default, envVars);

            switch (targetPlatform)
            {
            case TargetPlatform.PS4:
                // Hack: PS4 uses .o extension for compiler output files but CMake uses .obj even if CMAKE_CXX_OUTPUT_EXTENSION/CMAKE_C_OUTPUT_EXTENSION are specified
                Utilities.ReplaceInFiles(Path.Combine(root, "physx\\compiler\\ps4"), "*.vcxproj", SearchOption.AllDirectories, ".obj", ".o");
                break;
            case TargetPlatform.XboxScarlett:
                // Hack: force to use proper Win10 SDK
                Utilities.ReplaceInFiles(Path.Combine(root, "physx\\compiler\\vc16win64"), "*.vcxproj", SearchOption.AllDirectories, "10.0.18362.0", "10.0.19041.0");

                // Hack: fix STL include
                Utilities.ReplaceInFile(Path.Combine(root, "physx\\source\\foundation\\include\\PsAllocator.h"), "#include <typeinfo.h>", "#include <typeinfo>");
                break;
            case TargetPlatform.Android:
                // Hack: fix compilation errors
                if (!File.ReadAllText(Path.Combine(root, "physx\\source\\foundation\\include\\PsUtilities.h")).Contains("#if PX_GCC_FAMILY && !PX_EMSCRIPTEN  && !PX_LINUX && !PX_ANDROID"))
                    Utilities.ReplaceInFile(Path.Combine(root, "physx\\source\\foundation\\include\\PsUtilities.h"), "#if PX_GCC_FAMILY && !PX_EMSCRIPTEN  && !PX_LINUX", "#if PX_GCC_FAMILY && !PX_EMSCRIPTEN  && !PX_LINUX && !PX_ANDROID");
                Utilities.ReplaceInFile(Path.Combine(root, "physx\\source\\compiler\\cmake\\android\\CMakeLists.txt"), "-Wno-maybe-uninitialized", "-Wno-unused-local-typedef -Wno-unused-private-field");

                // PhysX build system for Android is old and doesn't support new NDK with clang so invoke cmake manually
                if (!Directory.Exists(Path.Combine(root, "physx\\compiler\\android")))
                    Directory.CreateDirectory(Path.Combine(root, "physx\\compiler\\android"));
                envVars.Add("PHYSX_ROOT_DIR", Path.Combine(root, "physx"));
                envVars.Add("PM_CMAKEMODULES_PATH", Path.Combine(root, "externals/CMakeModules"));
                envVars.Add("PM_PXSHARED_PATH", Path.Combine(root, "pxshared"));
                envVars.Add("PM_TARGA_PATH", Path.Combine(root, "externals/targa"));
                envVars.Add("PM_PATHS", Path.Combine(root, "externals/CMakeModules") + ';' + Path.Combine(root, "externals/targa"));
                RunCmake(Path.Combine(root, "physx\\compiler\\android"), targetPlatform, architecture, string.Format("\"{0}/physx/compiler/public\" -Wno-dev -DANDROID_NATIVE_API_LEVEL=android-{1} -DTARGET_BUILD_PLATFORM=android --no-warn-unused-cli -DCMAKE_BUILD_TYPE={2} -DCMAKE_PREFIX_PATH=\"{0}/externals/CMakeModules;{0}/externals/targa\" -DPHYSX_ROOT_DIR=\"{0}/physx\" -DPX_OUTPUT_LIB_DIR=\"{0}/physx\" -DPX_OUTPUT_BIN_DIR=\"{0}/physx\" -DPX_BUILDSNIPPETS=FALSE -DPX_GENERATE_STATIC_LIBRARIES=TRUE -DCMAKE_INSTALL_PREFIX=\"{0}/physx/install/android-{1}/PhysX\"", root, Configuration.AndroidPlatformApi, configuration), envVars);
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
            };
            var dstBinaries = GetThirdPartyFolder(options, targetPlatform, architecture);
            var srcBinaries = Path.Combine(root, "physx", "bin", binariesSubDir, configuration);
            switch (BuildPlatform)
            {
            case TargetPlatform.Windows:
                switch (targetPlatform)
                {
                case TargetPlatform.Android:
                    Utilities.Run("cmake", "--build .", null, Path.Combine(root, "physx\\compiler\\android"), Utilities.RunOptions.None, envVars);
                    break;
                default:
                    VCEnvironment.BuildSolution(Path.Combine(solutionFilesRoot, preset, "PhysXSDK.sln"), configuration, buildPlatform);
                    break;
                }
                break;
            case TargetPlatform.Linux:
                Utilities.Run("make", null, null, Path.Combine(projectGenDir, "compiler", "linux-" + configuration), Utilities.RunOptions.None);
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
            if (BuildPlatform == TargetPlatform.Windows)
            {
                projectGenPath = Path.Combine(projectGenDir, "generate_projects.bat");
            }
            else if (BuildPlatform == TargetPlatform.Linux)
            {
                projectGenPath = Path.Combine(projectGenDir, "generate_projects.sh");
            }

            // Get the source
            CloneGitRepoSingleBranch(root, "https://github.com/NVIDIAGameWorks/PhysX.git", "4.1");

            foreach (var platform in options.Platforms)
            {
                switch (platform)
                {
                case TargetPlatform.Windows:
                {
                    Build(options, "vc14win64", platform, TargetArchitecture.x64);
                    break;
                }
                case TargetPlatform.UWP:
                {
                    Build(options, "vc15uwp64", platform, TargetArchitecture.x64);
                    break;
                }
                case TargetPlatform.XboxOne:
                {
                    Build(options, "vc15uwp64", platform, TargetArchitecture.x64);
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
                case TargetPlatform.XboxScarlett:
                {
                    Build(options, "vc16win64", platform, TargetArchitecture.x64);
                    break;
                }
                case TargetPlatform.Android:
                {
                    Build(options, "android", platform, TargetArchitecture.ARM64);
                    break;
                }
                }
            }

            // Copy header files
            var dstIncludePath = Path.Combine(options.ThirdPartyFolder, "PhysX");
            Directory.GetFiles(dstIncludePath, "*.h", SearchOption.AllDirectories).ToList().ForEach(File.Delete);
            Utilities.DirectoryCopy(Path.Combine(root, "physx", "include"), dstIncludePath);
            Utilities.DirectoryCopy(Path.Combine(root, "pxshared", "include"), dstIncludePath);
            foreach (var filename in new[]
            {
                "Ps.h",
                "PsAllocator.h",
                "PsSync.h",
            })
                Utilities.FileCopy(Path.Combine(root, "physx", "source", "foundation", "include", filename), Path.Combine(dstIncludePath, "foundation", filename));
        }
    }
}
