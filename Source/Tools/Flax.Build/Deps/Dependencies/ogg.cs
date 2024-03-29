// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.IO;
using System.Linq;
using Flax.Build;
using Flax.Build.Platforms;

namespace Flax.Deps.Dependencies
{
    /// <summary>
    /// Ogg project codecs use the Ogg bitstream format to arrange the raw, compressed bitstream into a more robust, useful form. For example, the Ogg bitstream makes seeking, time stamping and error recovery possible, as well as mixing several sepearate, concurrent media streams into a single physical bitstream.
    /// </summary>
    /// <seealso cref="Flax.Deps.Dependency" />
    class ogg : Dependency
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

        private void PatchWindowsTargetPlatformVersion(string vcxprojPath, string vcxprojContents, string windowsTargetPlatformVersion, string platformToolset)
        {
            // Fix the MSVC project settings for Windows
            var contents = vcxprojContents.Replace("<PlatformToolset>v140</PlatformToolset>", string.Format("<PlatformToolset>{0}</PlatformToolset>", platformToolset));
            contents = contents.Replace("</RootNamespace>", string.Format("</RootNamespace><WindowsTargetPlatformVersion>{0}</WindowsTargetPlatformVersion>", windowsTargetPlatformVersion));
            File.WriteAllText(vcxprojPath, contents);
        }

        /// <inheritdoc />
        public override void Build(BuildOptions options)
        {
            var root = options.IntermediateFolder;
            var configuration = "Release";
            var filesToKeep = new[]
            {
                "ogg.Build.cs",
            };

            // Get the source
            CloneGitRepo(root, "https://github.com/xiph/ogg");
            GitCheckout(root, "master", "4380566a44b8d5e85ad511c9c17eb04197863ec5");

            var binariesToCopyMsvc = new[]
            {
                "libogg_static.lib",
            };
            var vsSolutionPath = Path.Combine(root, "win32", "VS2015", "libogg_static.sln");
            var vcxprojPath = Path.Combine(root, "win32", "VS2015", "libogg_static.vcxproj");
            var vcxprojContents = File.ReadAllText(vcxprojPath);
            var libraryFileName = "libogg.a";
            vcxprojContents = vcxprojContents.Replace("<RuntimeLibrary>MultiThreaded</RuntimeLibrary>", "<RuntimeLibrary>MultiThreadedDLL</RuntimeLibrary>");
            vcxprojContents = vcxprojContents.Replace("<RuntimeLibrary>MultiThreadedDebug</RuntimeLibrary>", "<RuntimeLibrary>MultiThreadedDebugDLL</RuntimeLibrary>");
            vcxprojContents = vcxprojContents.Replace("<WholeProgramOptimization>true</WholeProgramOptimization>", "<WholeProgramOptimization>false</WholeProgramOptimization>");
            var buildDir = Path.Combine(root, "build");

            foreach (var platform in options.Platforms)
            {
                switch (platform)
                {
                case TargetPlatform.Windows:
                {
                    // Fix the MSVC project settings for Windows
                    PatchWindowsTargetPlatformVersion(vcxprojPath, vcxprojContents, "8.1", "140");

                    // Build for Win64
                    Deploy.VCEnvironment.BuildSolution(vsSolutionPath, configuration, "x64");
                    var depsFolder = GetThirdPartyFolder(options, platform, TargetArchitecture.x64);
                    foreach (var file in binariesToCopyMsvc)
                        Utilities.FileCopy(Path.Combine(root, "win32", "VS2015", "x64", configuration, file), Path.Combine(depsFolder, file));

                    break;
                }
                case TargetPlatform.UWP:
                {
                    // Fix the MSVC project settings for UWP
                    PatchWindowsTargetPlatformVersion(vcxprojPath, vcxprojContents, "10.0.17763.0", "v141");

                    // Build for UWP x64
                    Deploy.VCEnvironment.BuildSolution(vsSolutionPath, configuration, "x64");
                    var depsFolder = GetThirdPartyFolder(options, platform, TargetArchitecture.x64);
                    foreach (var file in binariesToCopyMsvc)
                        Utilities.FileCopy(Path.Combine(root, "win32", "VS2015", "x64", configuration, file), Path.Combine(depsFolder, file));

                    break;
                }
                case TargetPlatform.Linux:
                {
                    var envVars = new Dictionary<string, string>
                    {
                        { "CC", "clang-7" },
                        { "CC_FOR_BUILD", "clang-7" }
                    };

                    Utilities.Run(Path.Combine(root, "autogen.sh"), null, null, root, Utilities.RunOptions.Default, envVars);

                    // Build for Linux
                    var toolchain = UnixToolchain.GetToolchainName(platform, TargetArchitecture.x64);
                    Utilities.Run(Path.Combine(root, "configure"), string.Format("--host={0}", toolchain), null, root, Utilities.RunOptions.Default, envVars);
                    SetupDirectory(buildDir, true);
                    Utilities.Run("cmake", "-G \"Unix Makefiles\" -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_BUILD_TYPE=Release ..", null, buildDir, Utilities.RunOptions.ConsoleLogOutput, envVars);
                    Utilities.Run("cmake", "--build .", null, buildDir, Utilities.RunOptions.ConsoleLogOutput, envVars);
                    var depsFolder = GetThirdPartyFolder(options, platform, TargetArchitecture.x64);
                    Utilities.FileCopy(Path.Combine(buildDir, libraryFileName), Path.Combine(depsFolder, libraryFileName));

                    break;
                }
                case TargetPlatform.PS4:
                {
                    // Get the build data files
                    Utilities.DirectoryCopy(
                                            Path.Combine(GetBinariesFolder(options, platform), "Data", "ogg"),
                                            Path.Combine(root, "PS4"), true, true);

                    // Build for PS4
                    var solutionPath = Path.Combine(root, "PS4", "libogg_static.sln");
                    Deploy.VCEnvironment.BuildSolution(solutionPath, "Release", "ORBIS");
                    var depsFolder = GetThirdPartyFolder(options, platform, TargetArchitecture.x64);
                    Utilities.FileCopy(Path.Combine(root, "PS4", "lib", libraryFileName), Path.Combine(depsFolder, libraryFileName));

                    break;
                }
                case TargetPlatform.PS5:
                {
                    // Get the build data files
                    Utilities.DirectoryCopy(
                                            Path.Combine(GetBinariesFolder(options, platform), "Data", "ogg"),
                                            Path.Combine(root, "PS5"), true, true);

                    // Build for PS5
                    var solutionPath = Path.Combine(root, "PS5", "libogg_static.sln");
                    Deploy.VCEnvironment.BuildSolution(solutionPath, "Release", "PROSPERO");
                    var depsFolder = GetThirdPartyFolder(options, platform, TargetArchitecture.x64);
                    Utilities.FileCopy(Path.Combine(root, "PS5", "lib", libraryFileName), Path.Combine(depsFolder, libraryFileName));

                    break;
                }
                case TargetPlatform.XboxOne:
                {
                    // Fix the MSVC project settings for Xbox Scarlett
                    PatchWindowsTargetPlatformVersion(vcxprojPath, vcxprojContents, "10.0.19041.0", "v142");

                    // Build for Xbox Scarlett x64
                    Deploy.VCEnvironment.BuildSolution(vsSolutionPath, configuration, "x64");
                    var depsFolder = GetThirdPartyFolder(options, platform, TargetArchitecture.x64);
                    foreach (var file in binariesToCopyMsvc)
                        Utilities.FileCopy(Path.Combine(root, "win32", "VS2015", "x64", configuration, file), Path.Combine(depsFolder, file));

                    break;
                }
                case TargetPlatform.XboxScarlett:
                {
                    // Fix the MSVC project settings for Xbox Scarlett
                    PatchWindowsTargetPlatformVersion(vcxprojPath, vcxprojContents, "10.0.19041.0", "v142");

                    // Build for Xbox Scarlett x64
                    Deploy.VCEnvironment.BuildSolution(vsSolutionPath, configuration, "x64");
                    var depsFolder = GetThirdPartyFolder(options, platform, TargetArchitecture.x64);
                    foreach (var file in binariesToCopyMsvc)
                        Utilities.FileCopy(Path.Combine(root, "win32", "VS2015", "x64", configuration, file), Path.Combine(depsFolder, file));

                    break;
                }
                case TargetPlatform.Android:
                {
                    // Build for Android
                    SetupDirectory(buildDir, true);
                    RunCmake(buildDir, platform, TargetArchitecture.ARM64, ".. -DCMAKE_BUILD_TYPE=Release");
                    BuildCmake(buildDir);
                    var depsFolder = GetThirdPartyFolder(options, platform, TargetArchitecture.ARM64);
                    Utilities.FileCopy(Path.Combine(buildDir, libraryFileName), Path.Combine(depsFolder, libraryFileName));
                    break;
                }
                case TargetPlatform.Switch:
                {
                    // Get the build data files
                    Utilities.DirectoryCopy(Path.Combine(GetBinariesFolder(options, platform), "Data", "ogg"), root, true, true);

                    // Build for Switch
                    SetupDirectory(buildDir, true);
                    RunCmake(buildDir, platform, TargetArchitecture.ARM64, ".. -DCMAKE_BUILD_TYPE=Release");
                    BuildCmake(buildDir);
                    var depsFolder = GetThirdPartyFolder(options, platform, TargetArchitecture.ARM64);
                    Utilities.FileCopy(Path.Combine(buildDir, libraryFileName), Path.Combine(depsFolder, libraryFileName));
                    break;
                }
                case TargetPlatform.Mac:
                {
                    // Build for Mac
                    foreach (var architecture in new[] { TargetArchitecture.x64, TargetArchitecture.ARM64 })
                    {
                        SetupDirectory(buildDir, true);
                        RunCmake(buildDir, platform, architecture, ".. -DCMAKE_BUILD_TYPE=Release");
                        BuildCmake(buildDir);
                        var depsFolder = GetThirdPartyFolder(options, platform, architecture);
                        Utilities.FileCopy(Path.Combine(buildDir, libraryFileName), Path.Combine(depsFolder, libraryFileName));
                    }
                    break;
                }
                case TargetPlatform.iOS:
                {
                    SetupDirectory(buildDir, true);
                    RunCmake(buildDir, platform, TargetArchitecture.ARM64, ".. -DCMAKE_BUILD_TYPE=Release");
                    BuildCmake(buildDir);
                    var depsFolder = GetThirdPartyFolder(options, platform, TargetArchitecture.ARM64);
                    Utilities.FileCopy(Path.Combine(buildDir, libraryFileName), Path.Combine(depsFolder, libraryFileName));
                    break;
                }
                }
            }

            // Backup files
            var srcIncludePath = Path.Combine(root, "include", "ogg");
            var dstIncludePath = Path.Combine(options.ThirdPartyFolder, "ogg");
            foreach (var filename in filesToKeep)
            {
                var src = Path.Combine(dstIncludePath, filename);
                var dst = Path.Combine(options.IntermediateFolder, filename + ".tmp");
                Utilities.FileCopy(src, dst);
            }

            // Setup headers directory
            SetupDirectory(dstIncludePath, true);

            // Deploy header files and restore files
            Directory.GetFiles(srcIncludePath, "Makefile*").ToList().ForEach(File.Delete);
            Directory.GetFiles(srcIncludePath, "*.in").ToList().ForEach(File.Delete);
            Utilities.DirectoryCopy(srcIncludePath, dstIncludePath, true, true);
            File.Copy(Path.Combine(root, "COPYING"), Path.Combine(dstIncludePath, "COPYING"));
            foreach (var filename in filesToKeep)
            {
                var src = Path.Combine(options.IntermediateFolder, filename + ".tmp");
                var dst = Path.Combine(dstIncludePath, filename);
                Utilities.FileCopy(src, dst);
            }
        }
    }
}
