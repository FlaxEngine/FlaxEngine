// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.Linq;
using Flax.Build;
using Flax.Build.Platforms;

namespace Flax.Deps.Dependencies
{
    /// <summary>
    /// vorbis project codecs use the vorbis bitstream format to arrange the raw, compressed bitstream into a more robust, useful form. For example, the vorbis bitstream makes seeking, time stamping and error recovery possible, as well as mixing several sepearate, concurrent media streams into a single physical bitstream.
    /// </summary>
    /// <seealso cref="Flax.Deps.Dependency" />
    class vorbis : Dependency
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

        private struct Binary
        {
            public string Filename;
            public string SrcFolder;

            public Binary(string filename, string srcFolder)
            {
                Filename = filename;
                SrcFolder = srcFolder;
            }
        }

        private bool hasSourcesReady;
        private string root;
        private string rootMsvcLib;
        private string configurationMsvc;
        private List<string> vcxprojContentsWindows;
        private string[] vcxprojPathsWindows;

        private Binary[] vorbisBinariesToCopyWindows =
        {
            new Binary("libvorbis_static.lib", "libvorbis"),
            new Binary("libvorbisfile_static.lib", "libvorbisfile"),
        };

        private (string, string)[] vorbisBinariesToCopyWindowsCmake =
        {
            ("vorbis.lib", "libvorbis_static.lib"),
            ("vorbisfile.lib", "libvorbisfile_static.lib"),
        };

        private Binary[] oggBinariesToCopyWindows =
        {
            new Binary("libogg_static.lib", "ogg"),
        };

        private (string, string)[] oggBinariesToCopyWindowsCmake =
        {
            ("ogg.lib", "libogg_static.lib"),
        };

        private void PatchWindowsTargetPlatformVersion(string windowsTargetPlatformVersion, string platformToolset)
        {
            // Fix the MSVC project settings for Windows
            var rootNamespaceEnding = string.Format("</RootNamespace><WindowsTargetPlatformVersion>{0}</WindowsTargetPlatformVersion><PlatformToolset>{1}</PlatformToolset>", windowsTargetPlatformVersion, platformToolset);
            for (var i = 0; i < vcxprojContentsWindows.Count; i++)
            {
                var contents = vcxprojContentsWindows[i].Replace("</RootNamespace>", rootNamespaceEnding);
                File.WriteAllText(vcxprojPathsWindows[i], contents);
            }
        }

        private void GetSources()
        {
            if (hasSourcesReady)
                return;

            hasSourcesReady = true;
            configurationMsvc = "Release";

            string oggRoot = Path.Combine(root, "libogg");
            string vorbisRoot = Path.Combine(root, "libvorbis");

            SetupDirectory(oggRoot, false);
            CloneGitRepo(oggRoot, "https://github.com/xiph/ogg.git");
            GitResetLocalChanges(oggRoot); // Reset patches
            GitCheckout(oggRoot, "master", "db5c7a49ce7ebda47b15b78471e78fb7f2483e22");

            SetupDirectory(vorbisRoot, false);
            CloneGitRepo(vorbisRoot, "https://github.com/xiph/vorbis.git");
            GitResetLocalChanges(vorbisRoot); // Reset patches
            GitCheckout(vorbisRoot, "master", "84c023699cdf023a32fa4ded32019f194afcdad0");

            rootMsvcLib = vorbisRoot;

            // Patch Windows projects which use MSBuild
            vcxprojPathsWindows = new[]
            {
                Path.Combine(rootMsvcLib, "win32", "VS2010", "libvorbis", "libvorbis_static.vcxproj"),
                Path.Combine(rootMsvcLib, "win32", "VS2010", "libvorbisfile", "libvorbisfile_static.vcxproj"),
            };
            vcxprojContentsWindows = vcxprojPathsWindows.Select(File.ReadAllText).ToList();
            for (var i = 0; i < vcxprojContentsWindows.Count; i++)
            {
                var contents = vcxprojContentsWindows[i].Replace("<RuntimeLibrary>MultiThreaded</RuntimeLibrary>", "<RuntimeLibrary>MultiThreadedDLL</RuntimeLibrary>");
                contents = contents.Replace("<RuntimeLibrary>MultiThreadedDebug</RuntimeLibrary>", "<RuntimeLibrary>MultiThreadedDebugDLL</RuntimeLibrary>");
                contents = contents.Replace("<DebugInformationFormat>ProgramDatabase</DebugInformationFormat>", "<DebugInformationFormat></DebugInformationFormat>");
                vcxprojContentsWindows[i] = contents.Replace("<WholeProgramOptimization>true</WholeProgramOptimization>", "<WholeProgramOptimization>false</WholeProgramOptimization>");
            }

            // TODO: FIXME for UWP/XBoxOne (use CMake for these too?)
#if false
            var packagePath = Path.Combine(root, "package.zip");
            configurationMsvc = "Release";

            // Get the additional source (ogg dependency)
            if (!Directory.Exists(Path.Combine(root, "libogg")))
            {
                File.Delete(packagePath);
                Downloader.DownloadFileFromUrlToPath("http://downloads.xiph.org/releases/ogg/libogg-1.3.3.zip", packagePath);
                using (ZipArchive archive = ZipFile.Open(packagePath, ZipArchiveMode.Read))
                {
                    archive.ExtractToDirectory(root);
                    Directory.Move(Path.Combine(root, archive.Entries.First().FullName), Path.Combine(root, "libogg"));
                }
            }

            // Get the source
            if (!Directory.Exists(Path.Combine(root, "libvorbis")))
            {
                File.Delete(packagePath);
                Downloader.DownloadFileFromUrlToPath("http://downloads.xiph.org/releases/vorbis/libvorbis-1.3.6.zip", packagePath);
                using (ZipArchive archive = ZipFile.Open(packagePath, ZipArchiveMode.Read))
                {
                    archive.ExtractToDirectory(root);
                    Directory.Move(Path.Combine(root, archive.Entries.First().FullName), Path.Combine(root, "libvorbis"));
                }
            }
#endif
        }

        private void BuildMsbuild(BuildOptions options, TargetPlatform platform, TargetArchitecture architecture)
        {
            GetSources();

            string buildPlatform, buildDir;
            string[] vcxprojPaths;
            List<Binary> binariesToCopy = new List<Binary>();
            switch (platform)
            {
            case TargetPlatform.Windows:
            {
                buildDir = Path.Combine(rootMsvcLib, "win32", "VS2010");
                vcxprojPaths = vcxprojPathsWindows;
                PatchWindowsTargetPlatformVersion("10.0", "v143");
                switch (architecture)
                {
                case TargetArchitecture.x86:
                    buildPlatform = "Win32";
                    break;
                case TargetArchitecture.x64:
                    buildPlatform = "x64";
                    break;
                case TargetArchitecture.ARM64:
                    buildPlatform = "ARM64";
                    break;
                default: throw new InvalidArchitectureException(architecture);
                }
                binariesToCopy.AddRange(vorbisBinariesToCopyWindows.Select(x => new Binary(x.Filename, Path.Combine(buildDir, x.SrcFolder, buildPlatform, configurationMsvc))));
                break;
            }
            case TargetPlatform.PS4:
            {
                buildDir = Path.Combine(rootMsvcLib, "PS4");
                var binariesToCopyVorbis = new[]
                {
                    new Binary("libvorbis.a", "libvorbis"),
                };
                vcxprojPaths = new[]
                {
                    Path.Combine(buildDir, "libvorbis", "libvorbis_static.vcxproj"),
                };
                buildPlatform = "ORBIS";
                Utilities.DirectoryCopy(Path.Combine(GetBinariesFolder(options, platform), "Data", "vorbis"),
                                        buildDir, true, true);
                Utilities.FileCopy(Path.Combine(GetBinariesFolder(options, platform), "Data", "ogg", "ogg", "config_types.h"),
                                   Path.Combine(root, "libogg", "include", "ogg", "config_types.h"));
                binariesToCopy.AddRange(binariesToCopyVorbis.Select(x => new Binary(x.Filename, Path.Combine(buildDir, x.SrcFolder, buildPlatform, configurationMsvc))));
                break;
            }
            case TargetPlatform.PS5:
            {
                buildDir = Path.Combine(rootMsvcLib, "PS5");
                var binariesToCopyVorbis = new[]
                {
                    new Binary("libvorbis.a", "libvorbis"),
                };
                vcxprojPaths = new[]
                {
                    Path.Combine(buildDir, "libvorbis", "libvorbis_static.vcxproj"),
                };
                buildPlatform = "PROSPERO";
                Utilities.DirectoryCopy(
                                        Path.Combine(GetBinariesFolder(options, platform), "Data", "vorbis"),
                                        buildDir, true, true);
                Utilities.FileCopy(
                                   Path.Combine(GetBinariesFolder(options, platform), "Data", "ogg", "ogg", "config_types.h"),
                                   Path.Combine(root, "libogg", "include", "ogg", "config_types.h"));
                binariesToCopy.AddRange(binariesToCopyVorbis.Select(x => new Binary(x.Filename, Path.Combine(buildDir, x.SrcFolder, buildPlatform, configurationMsvc))));
                break;
            }
            case TargetPlatform.XboxOne:
                buildDir = Path.Combine(rootMsvcLib, "win32", "VS2010");
                vcxprojPaths = vcxprojPathsWindows;
                buildPlatform = "x64";
                PatchWindowsTargetPlatformVersion("10.0", "v143");
                binariesToCopy.AddRange(vorbisBinariesToCopyWindows.Select(x => new Binary(x.Filename, Path.Combine(buildDir, x.SrcFolder, buildPlatform, configurationMsvc))));
                break;
            case TargetPlatform.XboxScarlett:
                buildDir = Path.Combine(rootMsvcLib, "win32", "VS2010");
                vcxprojPaths = vcxprojPathsWindows;
                buildPlatform = "x64";
                PatchWindowsTargetPlatformVersion("10.0", "v143");
                binariesToCopy.AddRange(vorbisBinariesToCopyWindows.Select(x => new Binary(x.Filename, Path.Combine(buildDir, x.SrcFolder, buildPlatform, configurationMsvc))));
                break;
            default: throw new InvalidPlatformException(platform);
            }

            // Build
            foreach (var vcxprojPath in vcxprojPaths)
                Deploy.VCEnvironment.BuildSolution(vcxprojPath, configurationMsvc, buildPlatform);

            // Copy binaries
            var depsFolder = GetThirdPartyFolder(options, platform, architecture);
            foreach (var filename in binariesToCopy)
                Utilities.FileCopy(Path.Combine(filename.SrcFolder, filename.Filename), Path.Combine(depsFolder, filename.Filename));
        }

        private void BuildCmake(BuildOptions options, TargetPlatform platform, TargetArchitecture architecture)
        {
            GetSources();

            string oggRoot = Path.Combine(root, "libogg");
            string vorbisRoot = Path.Combine(root, "libvorbis");

            var oggBuildDir = Path.Combine(oggRoot, "build-" + architecture.ToString());
            var vorbisBuildDir = Path.Combine(vorbisRoot, "build-" + architecture.ToString());

            string ext;
            switch (platform)
            {
            case TargetPlatform.Windows:
            case TargetPlatform.UWP:
            case TargetPlatform.XboxOne:
                ext = ".lib";
                break;
            case TargetPlatform.Linux:
                ext = ".a";
                break;
            default: throw new InvalidPlatformException(platform);
            }

            var binariesToCopy = new List<(string, string)>();

            // Build ogg
            {
                var solutionPath = Path.Combine(oggBuildDir, "ogg.sln");

                RunCmake(oggRoot, platform, architecture, $"-B\"{oggBuildDir}\" -DBUILD_SHARED_LIBS=OFF");
                Deploy.VCEnvironment.BuildSolution(solutionPath, configurationMsvc, architecture.ToString());
                foreach (var file in oggBinariesToCopyWindowsCmake)
                    binariesToCopy.Add((Path.Combine(oggBuildDir, configurationMsvc, file.Item1), file.Item2));
            }

            // Build vorbis
            {
                var oggLibraryPath = Path.Combine(oggBuildDir, configurationMsvc, "ogg" + ext);
                var solutionPath = Path.Combine(vorbisBuildDir, "vorbis.sln");

                RunCmake(vorbisRoot, platform, architecture, $"-B\"{vorbisBuildDir}\" -DOGG_INCLUDE_DIR=\"{Path.Combine(oggRoot, "include")}\" -DOGG_LIBRARY=\"{oggLibraryPath}\" -DBUILD_SHARED_LIBS=OFF");
                Deploy.VCEnvironment.BuildSolution(solutionPath, configurationMsvc, architecture.ToString());
                foreach (var file in vorbisBinariesToCopyWindowsCmake)
                    binariesToCopy.Add((Path.Combine(vorbisBuildDir, "lib", configurationMsvc, file.Item1), file.Item2));
            }

            // Copy binaries
            var depsFolder = GetThirdPartyFolder(options, platform, architecture);
            foreach (var file in binariesToCopy)
                Utilities.FileCopy(file.Item1, Path.Combine(depsFolder, file.Item2));
        }

        /// <inheritdoc />
        public override void Build(BuildOptions options)
        {
            root = options.IntermediateFolder;
            var filesToKeep = new[]
            {
                "vorbis.Build.cs",
            };
            var binariesToCopyUnix = new[]
            {
                new Binary("libvorbis.a", "lib"),
                new Binary("libvorbisenc.a", "lib"),
                new Binary("libvorbisfile.a", "lib"),
            };

            foreach (var platform in options.Platforms)
            {
                BuildStarted(platform);
                switch (platform)
                {
                case TargetPlatform.Windows:
                {
                    BuildCmake(options, TargetPlatform.Windows, TargetArchitecture.x64);
                    BuildCmake(options, TargetPlatform.Windows, TargetArchitecture.ARM64);
                    break;
                }
                case TargetPlatform.UWP:
                {
                    BuildMsbuild(options, TargetPlatform.UWP, TargetArchitecture.x64);
                    break;
                }
                case TargetPlatform.XboxOne:
                {
                    BuildMsbuild(options, TargetPlatform.XboxOne, TargetArchitecture.x64);
                    break;
                }
                case TargetPlatform.Linux:
                {
                    // Note: assumes the libogg-dev package is pre-installed on the system

                    // Get the source
                    CloneGitRepoFast(root, "https://github.com/xiph/vorbis.git");

                    var envVars = new Dictionary<string, string>
                    {
                        { "CC", "clang-7" },
                        { "CC_FOR_BUILD", "clang-7" }
                    };
                    var buildDir = Path.Combine(root, "build");

                    Utilities.Run(Path.Combine(root, "autogen.sh"), null, null, root, Utilities.RunOptions.Default, envVars);

                    // Build for Linux
                    var toolchain = UnixToolchain.GetToolchainName(platform, TargetArchitecture.x64);
                    Utilities.Run(Path.Combine(root, "configure"), string.Format("--host={0}", toolchain), null, root, Utilities.RunOptions.Default, envVars);
                    SetupDirectory(buildDir, true);
                    Utilities.Run("cmake", "-G \"Unix Makefiles\" -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_BUILD_TYPE=Release ..", null, buildDir, Utilities.RunOptions.ConsoleLogOutput, envVars);
                    Utilities.Run("cmake", "--build .", null, buildDir, Utilities.RunOptions.ConsoleLogOutput, envVars);
                    var depsFolder = GetThirdPartyFolder(options, platform, TargetArchitecture.x64);
                    foreach (var file in binariesToCopyUnix)
                        Utilities.FileCopy(Path.Combine(buildDir, file.SrcFolder, file.Filename), Path.Combine(depsFolder, file.Filename));
                    break;
                }
                case TargetPlatform.PS4:
                {
                    BuildMsbuild(options, TargetPlatform.PS4, TargetArchitecture.x64);
                    break;
                }
                case TargetPlatform.PS5:
                {
                    BuildMsbuild(options, TargetPlatform.PS5, TargetArchitecture.x64);
                    break;
                }
                case TargetPlatform.XboxScarlett:
                {
                    BuildMsbuild(options, TargetPlatform.XboxScarlett, TargetArchitecture.x64);
                    break;
                }
                case TargetPlatform.Android:
                {
                    var oggRoot = Path.Combine(root, "ogg");
                    var oggBuildDir = Path.Combine(oggRoot, "build");
                    var buildDir = Path.Combine(root, "build");

                    // Get the source
                    CloneGitRepoFast(root, "https://github.com/xiph/vorbis.git");
                    CloneGitRepo(oggRoot, "https://github.com/xiph/ogg.git");
                    GitCheckout(oggRoot, "master", "4380566a44b8d5e85ad511c9c17eb04197863ec5");

                    // Build for Android
                    SetupDirectory(oggBuildDir, true);
                    RunCmake(oggBuildDir, platform, TargetArchitecture.ARM64, ".. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=\"../install\"");
                    Utilities.Run("cmake", "--build . --target install", null, oggBuildDir, Utilities.RunOptions.ConsoleLogOutput);
                    SetupDirectory(buildDir, true);
                    RunCmake(buildDir, platform, TargetArchitecture.ARM64, string.Format(".. -DCMAKE_BUILD_TYPE=Release  -DOGG_INCLUDE_DIR=\"{0}/install/include\" -DOGG_LIBRARY=\"{0}/install/lib\"", oggRoot));
                    BuildCmake(buildDir);
                    var depsFolder = GetThirdPartyFolder(options, platform, TargetArchitecture.ARM64);
                    foreach (var file in binariesToCopyUnix)
                        Utilities.FileCopy(Path.Combine(buildDir, file.SrcFolder, file.Filename), Path.Combine(depsFolder, file.Filename));
                    break;
                }
                case TargetPlatform.Switch:
                {
                    var oggRoot = Path.Combine(root, "ogg");
                    var oggBuildDir = Path.Combine(oggRoot, "build");
                    var buildDir = Path.Combine(root, "build");

                    // Get the source
                    CloneGitRepo(root, "https://github.com/xiph/vorbis.git");
                    GitCheckout(root, "master", "98eddc72d36e3421519d54b101c09b57e4d4d10d");
                    CloneGitRepo(oggRoot, "https://github.com/xiph/ogg.git");
                    GitCheckout(oggRoot, "master", "4380566a44b8d5e85ad511c9c17eb04197863ec5");
                    Utilities.DirectoryCopy(Path.Combine(GetBinariesFolder(options, platform), "ogg"), oggRoot, true, true);
                    Utilities.DirectoryCopy(Path.Combine(GetBinariesFolder(options, platform), "vorbis"), buildDir, true, true);

                    // Build for Switch
                    SetupDirectory(oggBuildDir, true);
                    RunCmake(oggBuildDir, platform, TargetArchitecture.ARM64, ".. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=\"../install\"");
                    Utilities.Run("cmake", "--build . --target install", null, oggBuildDir, Utilities.RunOptions.ConsoleLogOutput);
                    Utilities.FileCopy(Path.Combine(GetBinariesFolder(options, platform), "ogg", "include", "ogg", "config_types.h"), Path.Combine(oggRoot, "install", "include", "ogg", "config_types.h"));
                    SetupDirectory(buildDir, true);
                    RunCmake(buildDir, platform, TargetArchitecture.ARM64, string.Format(".. -DCMAKE_BUILD_TYPE=Release -DOGG_INCLUDE_DIR=\"{0}/install/include\" -DOGG_LIBRARY=\"{0}/install/lib\"", oggRoot));
                    BuildCmake(buildDir);
                    var depsFolder = GetThirdPartyFolder(options, platform, TargetArchitecture.ARM64);
                    foreach (var file in binariesToCopyUnix)
                        Utilities.FileCopy(Path.Combine(buildDir, file.SrcFolder, file.Filename), Path.Combine(depsFolder, file.Filename));
                    break;
                }
                case TargetPlatform.Mac:
                {
                    var oggRoot = Path.Combine(root, "ogg");
                    var oggBuildDir = Path.Combine(oggRoot, "build");
                    var buildDir = Path.Combine(root, "build");

                    // Get the source
                    CloneGitRepoFast(root, "https://github.com/xiph/vorbis.git");
                    CloneGitRepo(oggRoot, "https://github.com/xiph/ogg.git");
                    GitCheckout(oggRoot, "master", "4380566a44b8d5e85ad511c9c17eb04197863ec5");

                    // Build for Mac
                    foreach (var architecture in new[] { TargetArchitecture.x64, TargetArchitecture.ARM64 })
                    {
                        SetupDirectory(oggBuildDir, true);
                        RunCmake(oggBuildDir, platform, architecture, ".. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=\"../install\"");
                        Utilities.Run("cmake", "--build . --target install", null, oggBuildDir, Utilities.RunOptions.ConsoleLogOutput);
                        SetupDirectory(buildDir, true);
                        RunCmake(buildDir, platform, architecture, string.Format(".. -DCMAKE_BUILD_TYPE=Release  -DOGG_INCLUDE_DIR=\"{0}/install/include\" -DOGG_LIBRARY=\"{0}/install/lib\"", oggRoot));
                        BuildCmake(buildDir);
                        var depsFolder = GetThirdPartyFolder(options, platform, architecture);
                        foreach (var file in binariesToCopyUnix)
                            Utilities.FileCopy(Path.Combine(buildDir, file.SrcFolder, file.Filename), Path.Combine(depsFolder, file.Filename));
                    }
                    break;
                }
                case TargetPlatform.iOS:
                {
                    var oggRoot = Path.Combine(root, "ogg");
                    var oggBuildDir = Path.Combine(oggRoot, "build");
                    var buildDir = Path.Combine(root, "build");

                    // Get the source
                    CloneGitRepoFast(root, "https://github.com/xiph/vorbis.git");
                    CloneGitRepo(oggRoot, "https://github.com/xiph/ogg.git");
                    GitCheckout(oggRoot, "master", "4380566a44b8d5e85ad511c9c17eb04197863ec5");

                    // Build for Mac
                    SetupDirectory(oggBuildDir, true);
                    RunCmake(oggBuildDir, platform, TargetArchitecture.ARM64, ".. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=\"../install\"");
                    Utilities.Run("cmake", "--build . --target install", null, oggBuildDir, Utilities.RunOptions.ConsoleLogOutput);
                    SetupDirectory(buildDir, true);
                    RunCmake(buildDir, platform, TargetArchitecture.ARM64, string.Format(".. -DCMAKE_BUILD_TYPE=Release  -DOGG_INCLUDE_DIR=\"{0}/install/include\" -DOGG_LIBRARY=\"{0}/install/lib\"", oggRoot));
                    BuildCmake(buildDir);
                    var depsFolder = GetThirdPartyFolder(options, platform, TargetArchitecture.ARM64);
                    foreach (var file in binariesToCopyUnix)
                        Utilities.FileCopy(Path.Combine(buildDir, file.SrcFolder, file.Filename), Path.Combine(depsFolder, file.Filename));
                    break;
                }
                }
            }

            // Backup files
            if (hasSourcesReady)
                root = rootMsvcLib;
            var srcIncludePath = Path.Combine(root, "include", "vorbis");
            var dstIncludePath = Path.Combine(options.ThirdPartyFolder, "vorbis");
            foreach (var filename in filesToKeep)
            {
                var src = Path.Combine(dstIncludePath, filename);
                var dst = Path.Combine(options.IntermediateFolder, filename + ".tmp");
                Utilities.FileCopy(src, dst);
            }

            try
            {
                // Setup headers directory
                SetupDirectory(dstIncludePath, true);

                // Deploy header files and restore files
                Directory.GetFiles(srcIncludePath, "Makefile*").ToList().ForEach(File.Delete);
                Utilities.DirectoryCopy(srcIncludePath, dstIncludePath, true, true);
                Utilities.FileCopy(Path.Combine(root, "COPYING"), Path.Combine(dstIncludePath, "COPYING"));
            }
            finally
            {
                foreach (var filename in filesToKeep)
                {
                    var src = Path.Combine(options.IntermediateFolder, filename + ".tmp");
                    var dst = Path.Combine(dstIncludePath, filename);
                    Utilities.FileCopy(src, dst);
                }
            }
        }
    }
}
