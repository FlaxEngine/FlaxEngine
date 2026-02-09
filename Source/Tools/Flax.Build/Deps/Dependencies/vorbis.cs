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
        private struct Binary
        {
            public string Filename;
            public string SrcFolder;
            public string DstFilename;

            public Binary(string filename, string srcFolder, string dstFilename = null)
            {
                Filename = filename;
                SrcFolder = srcFolder;
                DstFilename = dstFilename;
            }
        }

        private bool hasSourcesReady;
        private string root;
        private string rootMsvcLib;
        private string _configuration = "Release";
        private List<string> vcxprojContentsWindows;
        private string[] vcxprojPathsWindows;

        private Binary[] vorbisBinariesToCopyWindows =
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
                binariesToCopy.AddRange(vorbisBinariesToCopyWindows.Select(x => new Binary(x.Filename, Path.Combine(buildDir, x.SrcFolder, buildPlatform, _configuration))));
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
                binariesToCopy.AddRange(binariesToCopyVorbis.Select(x => new Binary(x.Filename, Path.Combine(buildDir, x.SrcFolder, buildPlatform, _configuration))));
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
                binariesToCopy.AddRange(binariesToCopyVorbis.Select(x => new Binary(x.Filename, Path.Combine(buildDir, x.SrcFolder, buildPlatform, _configuration))));
                break;
            }
            case TargetPlatform.XboxOne:
                buildDir = Path.Combine(rootMsvcLib, "win32", "VS2010");
                vcxprojPaths = vcxprojPathsWindows;
                buildPlatform = "x64";
                PatchWindowsTargetPlatformVersion("10.0", "v143");
                binariesToCopy.AddRange(vorbisBinariesToCopyWindows.Select(x => new Binary(x.Filename, Path.Combine(buildDir, x.SrcFolder, buildPlatform, _configuration))));
                break;
            case TargetPlatform.XboxScarlett:
                buildDir = Path.Combine(rootMsvcLib, "win32", "VS2010");
                vcxprojPaths = vcxprojPathsWindows;
                buildPlatform = "x64";
                PatchWindowsTargetPlatformVersion("10.0", "v143");
                binariesToCopy.AddRange(vorbisBinariesToCopyWindows.Select(x => new Binary(x.Filename, Path.Combine(buildDir, x.SrcFolder, buildPlatform, _configuration))));
                break;
            default: throw new InvalidPlatformException(platform);
            }

            // Build
            foreach (var vcxprojPath in vcxprojPaths)
                Deploy.VCEnvironment.BuildSolution(vcxprojPath, _configuration, buildPlatform);

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
            var installDir = Path.Combine(root, "install");

            string ext;
            string oggConfig = $"-DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_BUILD_TYPE={_configuration} -DCMAKE_INSTALL_PREFIX=\"{installDir}\"";
            string vorbisConfig = $"-DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_BUILD_TYPE={_configuration} -DCMAKE_INSTALL_PREFIX=\"{installDir}\"";
            string liboggFilename = "libogg";
            Dictionary<string, string> envVars = new Dictionary<string, string>();
            (string, string)[] oggBinariesToCopy;
            Binary[] vorbisBinariesToCopy;
            switch (platform)
            {
            case TargetPlatform.Windows:
            case TargetPlatform.UWP:
            case TargetPlatform.XboxOne:
                oggConfig += " -DBUILD_SHARED_LIBS=OFF";
                vorbisConfig += " -DBUILD_SHARED_LIBS=OFF";
                ext = ".lib";
                liboggFilename = "ogg";
                break;
            case TargetPlatform.Linux:
                oggConfig += " -DCMAKE_POSITION_INDEPENDENT_CODE=ON";
                vorbisConfig += " -DCMAKE_POSITION_INDEPENDENT_CODE=ON";
                envVars = new Dictionary<string, string>
                {
                    { "CC", "clang-" + Configuration.LinuxClangMinVer },
                    { "CC_FOR_BUILD", "clang-" + Configuration.LinuxClangMinVer },
                    { "CXX", "clang++-" + Configuration.LinuxClangMinVer },
                    { "CMAKE_BUILD_PARALLEL_LEVEL", CmakeBuildParallel },
                };
                ext = ".a";
                break;
            case TargetPlatform.Mac:
                //oggConfig += $" -DOGG_INCLUDE_DIR=\"{oggRoot}/install/include\" -DOGG_LIBRARY=\"{oggRoot}/install/lib\"";
                ext = ".a";
                break;
            default: throw new InvalidPlatformException(platform);
            }

            switch (platform)
            {
            case TargetPlatform.Windows:
            case TargetPlatform.UWP:
            case TargetPlatform.XboxOne:
                oggBinariesToCopy =
                [
                    ("ogg.lib", "libogg_static.lib")
                ];
                vorbisBinariesToCopy =
                [
                    new Binary("vorbis.lib", "libvorbis", "libvorbis_static.lib"),
                    new Binary("vorbisfile.lib", "libvorbisfile", "libvorbisfile_static.lib")
                ];
                break;
            case TargetPlatform.Linux:
            case TargetPlatform.Mac:
                oggBinariesToCopy =
                [
                    ("libogg.a", "libogg.a")
                ];
                vorbisBinariesToCopy =
                [
                    new Binary("libvorbis.a", "lib"),
                    new Binary("libvorbisenc.a", "lib"),
                    new Binary("libvorbisfile.a", "lib")
                ];
                break;
            default: throw new InvalidPlatformException(platform);
            }

            vorbisConfig += $" -DOGG_INCLUDE_DIR=\"{Path.Combine(installDir, "include")}\" -DOGG_LIBRARY=\"{Path.Combine(installDir, "lib", liboggFilename + ext)}\"";

            var binariesToCopy = new List<(string, string)>();

            SetupDirectory(installDir, true);
            // Build ogg
            {
                SetupDirectory(oggBuildDir, true);
                RunCmake(oggRoot, platform, architecture, $"-B\"{oggBuildDir}\" " + oggConfig, envVars);
                if (platform == TargetPlatform.Windows)
                    Deploy.VCEnvironment.BuildSolution(Path.Combine(oggBuildDir, "ogg.sln"), _configuration, architecture.ToString());
                else
                    BuildCmake(oggBuildDir);
                Utilities.Run("cmake", $"--build . --config {_configuration} --target install", null, oggBuildDir, Utilities.RunOptions.DefaultTool);
            }
            // Build vorbis
            {
                SetupDirectory(vorbisBuildDir, true);
                RunCmake(vorbisRoot, platform, architecture, $"-B\"{vorbisBuildDir}\" " + vorbisConfig);
                if (platform == TargetPlatform.Windows)
                    Deploy.VCEnvironment.BuildSolution(Path.Combine(vorbisBuildDir, "vorbis.sln"), _configuration, architecture.ToString());
                else
                    BuildCmake(vorbisBuildDir);
                Utilities.Run("cmake", $"--build . --config {_configuration} --target install", null, vorbisBuildDir, Utilities.RunOptions.DefaultTool);
            }

            // Copy binaries
            foreach (var file in oggBinariesToCopy)
                binariesToCopy.Add((Path.Combine(installDir, "lib", file.Item1), file.Item2));
            foreach (var file in vorbisBinariesToCopy)
                binariesToCopy.Add((Path.Combine(installDir, "lib", file.Filename), file.DstFilename ?? file.Filename));

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
                foreach (var architecture in options.Architectures)
                {
                    BuildStarted(platform, architecture);
                    switch (platform)
                    {
                    case TargetPlatform.Windows:
                    {
                        BuildCmake(options, TargetPlatform.Windows, architecture);
                        break;
                    }
                    case TargetPlatform.UWP:
                    {
                        BuildMsbuild(options, TargetPlatform.UWP, architecture);
                        break;
                    }
                    case TargetPlatform.XboxOne:
                    {
                        BuildMsbuild(options, TargetPlatform.XboxOne, architecture);
                        break;
                    }
                    case TargetPlatform.Linux:
                    {
                        BuildCmake(options, TargetPlatform.Linux, architecture);
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
                        Utilities.Run("cmake", "--build . --config Release --target install", null, oggBuildDir, Utilities.RunOptions.ConsoleLogOutput);
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
                        SetupDirectory(oggRoot, false);
                        CloneGitRepo(root, "https://github.com/xiph/vorbis.git");
                        GitCheckout(root, "master", "98eddc72d36e3421519d54b101c09b57e4d4d10d");
                        CloneGitRepo(oggRoot, "https://github.com/xiph/ogg.git");
                        GitCheckout(oggRoot, "master", "4380566a44b8d5e85ad511c9c17eb04197863ec5");
                        Utilities.DirectoryCopy(Path.Combine(GetBinariesFolder(options, platform), "Data/ogg"), oggRoot, true, true);
                        Utilities.DirectoryCopy(Path.Combine(GetBinariesFolder(options, platform), "Data/vorbis"), buildDir, true, true);

                        // Build for Switch
                        SetupDirectory(oggBuildDir, true);
                        RunCmake(oggBuildDir, platform, TargetArchitecture.ARM64, ".. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=\"../install\"");
                        Utilities.Run("cmake", "--build . --config Release --target install", null, oggBuildDir, Utilities.RunOptions.ConsoleLogOutput);
                        Utilities.FileCopy(Path.Combine(GetBinariesFolder(options, platform), "Data/ogg", "include", "ogg", "config_types.h"), Path.Combine(oggRoot, "install", "include", "ogg", "config_types.h"));
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
                        BuildCmake(options, TargetPlatform.Mac, architecture);
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
                        Utilities.Run("cmake", "--build . --config Release --target install", null, oggBuildDir, Utilities.RunOptions.ConsoleLogOutput);
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
            }

            // Setup headers directory
            var installDir = Path.Combine(root, "install");
            var oggOut = Path.Combine(options.ThirdPartyFolder, "ogg");
            var vorbisOut = Path.Combine(options.ThirdPartyFolder, "vorbis");

            // Deploy header files
            Utilities.DirectoryCopy(Path.Combine(installDir, "include", "ogg"), oggOut, true, true);
            Utilities.DirectoryCopy(Path.Combine(installDir, "include", "vorbis"), vorbisOut, true, true);

            Utilities.FileCopy(Path.Combine(root, "libogg", "COPYING"), Path.Combine(oggOut, "COPYING"));
            Utilities.FileCopy(Path.Combine(root, "libvorbis", "COPYING"), Path.Combine(vorbisOut, "COPYING"));
        }
    }
}
