// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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

        private Binary[] binariesToCopyWindows =
        {
            new Binary("libvorbis_static.lib", "libvorbis"),
            new Binary("libvorbisfile_static.lib", "libvorbisfile"),
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

            var packagePath = Path.Combine(root, "package.zip");
            configurationMsvc = "Release";

            // Get the additional source (ogg dependency)
            Downloader.DownloadFileFromUrlToPath("http://downloads.xiph.org/releases/ogg/libogg-1.3.3.zip", packagePath);
            using (ZipArchive archive = ZipFile.Open(packagePath, ZipArchiveMode.Read))
            {
                archive.ExtractToDirectory(root);
                Directory.Move(Path.Combine(root, archive.Entries.First().FullName), Path.Combine(root, "libogg"));
            }

            // Get the source
            File.Delete(packagePath);
            Downloader.DownloadFileFromUrlToPath("http://downloads.xiph.org/releases/vorbis/libvorbis-1.3.6.zip", packagePath);
            using (ZipArchive archive = ZipFile.Open(packagePath, ZipArchiveMode.Read))
            {
                archive.ExtractToDirectory(root);
                rootMsvcLib = Path.Combine(root, archive.Entries.First().FullName);
            }

            // Patch Windows projects
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
        }

        private void BuildMsbuild(BuildOptions options, TargetPlatform platform, TargetArchitecture architecture)
        {
            GetSources();

            string buildPlatform, buildDir;
            string[] vcxprojPaths;
            Binary[] binariesToCopy;
            switch (platform)
            {
            case TargetPlatform.Windows:
                buildDir = Path.Combine(rootMsvcLib, "win32", "VS2010");
                binariesToCopy = binariesToCopyWindows;
                vcxprojPaths = vcxprojPathsWindows;
                PatchWindowsTargetPlatformVersion("8.1", "v140");
                switch (architecture)
                {
                case TargetArchitecture.x86:
                    buildPlatform = "Win32";
                    break;
                case TargetArchitecture.x64:
                    buildPlatform = "x64";
                    break;
                default: throw new InvalidArchitectureException(architecture);
                }

                break;
            case TargetPlatform.XboxOne:
                buildDir = Path.Combine(rootMsvcLib, "win32", "VS2010");
                binariesToCopy = binariesToCopyWindows;
                vcxprojPaths = vcxprojPathsWindows;
                buildPlatform = "x64";
                PatchWindowsTargetPlatformVersion("10.0.17763.0", "v141");
                break;
            case TargetPlatform.UWP:
                buildDir = Path.Combine(rootMsvcLib, "win32", "VS2010");
                binariesToCopy = binariesToCopyWindows;
                vcxprojPaths = vcxprojPathsWindows;
                PatchWindowsTargetPlatformVersion("10.0.17763.0", "v141");
                switch (architecture)
                {
                case TargetArchitecture.x86:
                    buildPlatform = "Win32";
                    break;
                case TargetArchitecture.x64:
                    buildPlatform = "x64";
                    break;
                case TargetArchitecture.ARM:
                    buildPlatform = "ARM";
                    break;
                default: throw new InvalidArchitectureException(architecture);
                }

                break;
            case TargetPlatform.PS4:
                buildDir = Path.Combine(rootMsvcLib, "PS4");
                binariesToCopy = new[]
                {
                    new Binary("libvorbis.a", "libvorbis"),
                };
                vcxprojPaths = new[]
                {
                    Path.Combine(buildDir, "libvorbis", "libvorbis_static.vcxproj"),
                };
                buildPlatform = "ORBIS";
                Utilities.DirectoryCopy(
                                        Path.Combine(GetBinariesFolder(options, platform), "Data", "vorbis"),
                                        buildDir, true, true);
                Utilities.FileCopy(
                                   Path.Combine(GetBinariesFolder(options, platform), "Data", "ogg", "ogg", "config_types.h"),
                                   Path.Combine(root, "libogg", "include", "ogg", "config_types.h"));
                break;
            case TargetPlatform.XboxScarlett:
                buildDir = Path.Combine(rootMsvcLib, "win32", "VS2010");
                binariesToCopy = binariesToCopyWindows;
                vcxprojPaths = vcxprojPathsWindows;
                buildPlatform = "x64";
                PatchWindowsTargetPlatformVersion("10.0.19041.0", "v142");
                break;
            default: throw new InvalidPlatformException(platform);
            }

            // Build
            foreach (var vcxprojPath in vcxprojPaths)
                Deploy.VCEnvironment.BuildSolution(vcxprojPath, configurationMsvc, buildPlatform);

            // Copy binaries
            var depsFolder = GetThirdPartyFolder(options, platform, architecture);
            foreach (var filename in binariesToCopy)
                Utilities.FileCopy(Path.Combine(buildDir, filename.SrcFolder, buildPlatform, configurationMsvc, filename.Filename), Path.Combine(depsFolder, filename.Filename));
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
                switch (platform)
                {
                case TargetPlatform.Windows:
                {
                    BuildMsbuild(options, TargetPlatform.Windows, TargetArchitecture.x64);
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
                    var toolchain = UnixToolchain.GetToolchainName(TargetPlatform.Linux, TargetArchitecture.x64);
                    Utilities.Run(Path.Combine(root, "configure"), string.Format("--host={0}", toolchain), null, root, Utilities.RunOptions.Default, envVars);
                    SetupDirectory(buildDir, true);
                    Utilities.Run("cmake", "-G \"Unix Makefiles\" -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_BUILD_TYPE=Release ..", null, buildDir, Utilities.RunOptions.None, envVars);
                    Utilities.Run("cmake", "--build .", null, buildDir, Utilities.RunOptions.None, envVars);
                    var depsFolder = GetThirdPartyFolder(options, TargetPlatform.Linux, TargetArchitecture.x64);
                    foreach (var file in binariesToCopyUnix)
                        Utilities.FileCopy(Path.Combine(buildDir, file.SrcFolder, file.Filename), Path.Combine(depsFolder, file.Filename));
                    break;
                }
                case TargetPlatform.PS4:
                {
                    BuildMsbuild(options, TargetPlatform.PS4, TargetArchitecture.x64);
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
                    CloneGitRepoFast(oggRoot, "https://github.com/xiph/ogg.git");

                    // Build for Android
                    SetupDirectory(oggBuildDir, true);
                    RunCmake(oggBuildDir, TargetPlatform.Android, TargetArchitecture.ARM64, ".. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=\"../install\"");
                    Utilities.Run("cmake", "--build . --target install", null, oggBuildDir, Utilities.RunOptions.None);
                    SetupDirectory(buildDir, true);
                    RunCmake(buildDir, TargetPlatform.Android, TargetArchitecture.ARM64, string.Format(".. -DCMAKE_BUILD_TYPE=Release  -DOGG_INCLUDE_DIR=\"{0}/install/include\" -DOGG_LIBRARY=\"{0}/install/lib\"", oggRoot));
                    Utilities.Run("cmake", "--build .", null, buildDir, Utilities.RunOptions.None);
                    var depsFolder = GetThirdPartyFolder(options, TargetPlatform.Android, TargetArchitecture.ARM64);
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

            // Setup headers directory
            SetupDirectory(dstIncludePath, true);

            // Deploy header files and restore files
            Directory.GetFiles(srcIncludePath, "Makefile*").ToList().ForEach(File.Delete);
            Utilities.DirectoryCopy(srcIncludePath, dstIncludePath, true, true);
            Utilities.FileCopy(Path.Combine(root, "COPYING"), Path.Combine(dstIncludePath, "COPYING"));
            foreach (var filename in filesToKeep)
            {
                var src = Path.Combine(options.IntermediateFolder, filename + ".tmp");
                var dst = Path.Combine(dstIncludePath, filename);
                Utilities.FileCopy(src, dst);
            }
        }
    }
}
