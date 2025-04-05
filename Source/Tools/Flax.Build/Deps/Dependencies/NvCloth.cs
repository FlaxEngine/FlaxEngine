// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.IO;
using System.Linq;
using Flax.Build;
using Flax.Build.Platforms;

namespace Flax.Deps.Dependencies
{
    /// <summary>
    /// NVIDIA NvCloth
    /// </summary>
    /// <seealso cref="Flax.Deps.Dependency" />
    class NvCloth : Dependency
    {
        private string root, nvCloth;

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
                        TargetPlatform.XboxScarlett,
                        TargetPlatform.PS4,
                        TargetPlatform.PS5,
                        TargetPlatform.Switch,
                        TargetPlatform.Android,
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

        /// <inheritdoc />
        public override void Build(BuildOptions options)
        {
            root = options.IntermediateFolder;
            nvCloth = Path.Combine(root, "NvCloth");

            // Get the source
            CloneGitRepoSingleBranch(root, "https://github.com/FlaxEngine/NvCloth.git", "master");

            foreach (var platform in options.Platforms)
            {
                BuildStarted(platform);
                switch (platform)
                {
                case TargetPlatform.Windows:
                    Build(options, platform, TargetArchitecture.x64);
                    Build(options, platform, TargetArchitecture.ARM64);
                    break;
                case TargetPlatform.XboxOne:
                case TargetPlatform.XboxScarlett:
                    Build(options, platform, TargetArchitecture.x64);
                    break;
                case TargetPlatform.PS4:
                case TargetPlatform.PS5:
                    Utilities.DirectoryCopy(Path.Combine(GetBinariesFolder(options, platform), "Data", "NvCloth"), root, true, true);
                    Build(options, platform, TargetArchitecture.x64);
                    break;
                case TargetPlatform.Switch:
                    Utilities.DirectoryCopy(Path.Combine(GetBinariesFolder(options, platform), "Data", "NvCloth"), root, true, true);
                    Build(options, platform, TargetArchitecture.ARM64);
                    break;
                case TargetPlatform.Android:
                    Build(options, platform, TargetArchitecture.ARM64);
                    break;
                case TargetPlatform.Mac:
                    Build(options, platform, TargetArchitecture.x64);
                    Build(options, platform, TargetArchitecture.ARM64);
                    break;
                case TargetPlatform.iOS:
                    Build(options, platform, TargetArchitecture.ARM64);
                    break;
                case TargetPlatform.Linux:
                    Build(options, platform, TargetArchitecture.x64);
                    break;
                }
            }

            // Copy header files
            var dstIncludePath = Path.Combine(options.ThirdPartyFolder, "NvCloth");
            Directory.GetFiles(dstIncludePath, "*.h", SearchOption.AllDirectories).ToList().ForEach(File.Delete);
            Utilities.FileCopy(Path.Combine(nvCloth, "license.txt"), Path.Combine(dstIncludePath, "License.txt"));
            Utilities.DirectoryCopy(Path.Combine(nvCloth, "include", "NvCloth"), dstIncludePath, true, true, "*.h");
            Utilities.DirectoryCopy(Path.Combine(nvCloth, "extensions", "include"), dstIncludePath, true, true, "*.h");
        }

        private void Build(BuildOptions options, TargetPlatform platform, TargetArchitecture architecture)
        {
            // Peek options
            var binariesPrefix = string.Empty;
            var binariesPostfix = string.Empty;
            var cmakeArgs = "-DNV_CLOTH_ENABLE_DX11=0 -DNV_CLOTH_ENABLE_CUDA=0 -DPX_GENERATE_GPU_PROJECTS=0";
            var cmakeName = string.Empty;
            var buildFolder = Path.Combine(nvCloth, "compiler", platform.ToString() + '_' + architecture.ToString());
            var envVars = new Dictionary<string, string>();
            envVars["GW_DEPS_ROOT"] = root;
            switch (platform)
            {
            case TargetPlatform.Windows:
            case TargetPlatform.XboxOne:
            case TargetPlatform.XboxScarlett:
                cmakeArgs += " -DTARGET_BUILD_PLATFORM=windows -DSTATIC_WINCRT=0";
                cmakeName = "windows";
                binariesPostfix = "_x64";
                break;
            case TargetPlatform.PS4:
                cmakeArgs += " -DTARGET_BUILD_PLATFORM=ps4";
                cmakeArgs += $" -DCMAKE_TOOLCHAIN_FILE=\"{Path.Combine(nvCloth, "Externals/CMakeModules/ps4/PS4Toolchain.txt").Replace('\\', '/')}\"";
                cmakeName = "ps4";
                binariesPrefix = "lib";
                break;
            case TargetPlatform.PS5:
                cmakeArgs += " -DTARGET_BUILD_PLATFORM=ps5";
                cmakeArgs += $" -DCMAKE_TOOLCHAIN_FILE=\"{Path.Combine(nvCloth, "Externals/CMakeModules/ps5/PS5Toolchain.txt").Replace('\\', '/')}\"";
                cmakeName = "ps5";
                binariesPrefix = "lib";
                break;
            case TargetPlatform.Switch:
                cmakeArgs += " -DTARGET_BUILD_PLATFORM=NX64";
                cmakeName = "switch";
                binariesPrefix = "lib";
                envVars.Add("NintendoSdkRoot", Sdk.Get("SwitchSdk").RootPath + '\\');
                break;
            case TargetPlatform.Android:
                cmakeArgs += " -DTARGET_BUILD_PLATFORM=android";
                cmakeName = "android";
                binariesPrefix = "lib";
                if (AndroidNdk.Instance.IsValid)
                {
                    envVars.Add("ANDROID_NDK_HOME", AndroidNdk.Instance.RootPath);
                    envVars.Add("PM_ANDROIDNDK_PATH", AndroidNdk.Instance.RootPath);
                }
                break;
            case TargetPlatform.Mac:
                cmakeArgs += " -DTARGET_BUILD_PLATFORM=mac";
                cmakeName = "mac";
                binariesPrefix = "lib";
                break;
            case TargetPlatform.iOS:
                cmakeArgs += " -DTARGET_BUILD_PLATFORM=ios";
                cmakeName = "ios";
                binariesPrefix = "lib";
                break;
            case TargetPlatform.Linux:
                cmakeArgs += " -DTARGET_BUILD_PLATFORM=linux";
                cmakeName = "linux";
                binariesPrefix = "lib";
                break;
            default: throw new InvalidPlatformException(platform);
            }
            var cmakeFolder = Path.Combine(nvCloth, "compiler", "cmake", cmakeName);

            // Setup build environment variables for the build system
            switch (BuildPlatform)
            {
            case TargetPlatform.Windows:
            {
                GetMsBuildForPlatform(platform, out var vsVersion, out var msBuild);
                if (File.Exists(msBuild))
                {
                    envVars.Add("PATH", Path.GetDirectoryName(msBuild));
                }
                break;
            }
            }

            // Print the NvCloth version
            Log.Info($"Building {File.ReadAllLines(Path.Combine(root, "README.md"))[0].Trim()} to {platform} {architecture}");

            // Generate project files
            SetupDirectory(buildFolder, false);
            Utilities.FileDelete(Path.Combine(cmakeFolder, "CMakeCache.txt"));
            cmakeArgs += $" -DPX_STATIC_LIBRARIES=1 -DPX_OUTPUT_DLL_DIR=\"{Path.Combine(buildFolder, "bin")}\" -DPX_OUTPUT_LIB_DIR=\"{Path.Combine(buildFolder, "lib")}\" -DPX_OUTPUT_EXE_DIR=\"{Path.Combine(buildFolder, "bin")}\"";
            RunCmake(cmakeFolder, platform, architecture, " -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF " + cmakeArgs, envVars);

            // Run build
            BuildCmake(cmakeFolder, envVars: envVars);

            // Deploy binaries
            var libs = new[]
            {
                "NvCloth",
            };
            var dstBinaries = GetThirdPartyFolder(options, platform, architecture);
            var srcBinaries = Path.Combine(buildFolder, "lib");
            var platformObj = Platform.GetPlatform(platform);
            var binariesExtension = platformObj.StaticLibraryFileExtension;
            foreach (var lib in libs)
            {
                var filename = binariesPrefix + lib + binariesPostfix + binariesExtension;
                Utilities.FileCopy(Path.Combine(srcBinaries, filename), Path.Combine(dstBinaries, filename));

                var filenamePdb = Path.ChangeExtension(filename, "pdb");
                if (File.Exists(Path.Combine(srcBinaries, filenamePdb)))
                    Utilities.FileCopy(Path.Combine(srcBinaries, filenamePdb), Path.Combine(dstBinaries, filenamePdb));
            }
        }
    }
}
