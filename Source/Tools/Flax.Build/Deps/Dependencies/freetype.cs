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
    /// FreeType is a freely available software library to render fonts.
    /// </summary>
    /// <seealso cref="Flax.Deps.Dependency" />
    class freetype : Dependency
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

        private void PatchWindowsTargetPlatformVersion(string vcxprojPath, string vcxprojContents, string windowsTargetPlatformVersion, string platformToolset)
        {
            // Fix the MSVC project settings for Windows
            var contents = vcxprojContents.Replace("$(DefaultPlatformToolset)", string.Format("{0}", platformToolset));
            contents = contents.Replace("</TargetName>", string.Format("</TargetName><WindowsTargetPlatformVersion>{0}</WindowsTargetPlatformVersion>", windowsTargetPlatformVersion));
            File.WriteAllText(vcxprojPath, contents);
        }

        /// <inheritdoc />
        public override void Build(BuildOptions options)
        {
            var root = options.IntermediateFolder;
            var packagePath = Path.Combine(root, "package.zip");
            var filesToKeep = new[]
            {
                "freetype.Build.cs",
            };

            // Get the source
            Downloader.DownloadFileFromUrlToPath("https://sourceforge.net/projects/freetype/files/freetype2/2.10.0/ft2100.zip/download", packagePath);
            using (ZipArchive archive = ZipFile.Open(packagePath, ZipArchiveMode.Read))
            {
                archive.ExtractToDirectory(root);
                root = Path.Combine(root, archive.Entries.First().FullName);
            }

            var configurationMsvc = "Release Static";
            var binariesToCopyMsvc = new[]
            {
                "freetype.lib",
                "freetype.pdb",
            };
            var vsSolutionPath = Path.Combine(root, "builds", "windows", "vc2010", "freetype.sln");
            var vcxprojPath = Path.Combine(root, "builds", "windows", "vc2010", "freetype.vcxproj");
            var vcxprojContents = File.ReadAllText(vcxprojPath);
            var libraryFileName = "libfreetype.a";
            vcxprojContents = vcxprojContents.Replace("<RuntimeLibrary>MultiThreaded</RuntimeLibrary>", "<RuntimeLibrary>MultiThreadedDLL</RuntimeLibrary>");
            vcxprojContents = vcxprojContents.Replace("<RuntimeLibrary>MultiThreadedDebug</RuntimeLibrary>", "<RuntimeLibrary>MultiThreadedDebugDLL</RuntimeLibrary>");

            foreach (var platform in options.Platforms)
            {
                switch (platform)
                {
                case TargetPlatform.Windows:
                {
                    // Fix the MSVC project settings for Windows
                    PatchWindowsTargetPlatformVersion(vcxprojPath, vcxprojContents, "8.1", "140");

                    // Build for Win64
                    Deploy.VCEnvironment.BuildSolution(vsSolutionPath, configurationMsvc, "x64");
                    var depsFolder = GetThirdPartyFolder(options, platform, TargetArchitecture.x64);
                    foreach (var filename in binariesToCopyMsvc)
                        Utilities.FileCopy(Path.Combine(root, "objs", "x64", configurationMsvc, filename), Path.Combine(depsFolder, filename));

                    break;
                }
                case TargetPlatform.UWP:
                {
                    // Fix the MSVC project settings for UWP
                    PatchWindowsTargetPlatformVersion(vcxprojPath, vcxprojContents, "10.0.17763.0", "v141");

                    // Build for UWP x64
                    Deploy.VCEnvironment.BuildSolution(vsSolutionPath, configurationMsvc, "x64");
                    var depsFolder = GetThirdPartyFolder(options, platform, TargetArchitecture.x64);
                    foreach (var filename in binariesToCopyMsvc)
                        Utilities.FileCopy(Path.Combine(root, "objs", "x64", configurationMsvc, filename), Path.Combine(depsFolder, filename));

                    break;
                }
                case TargetPlatform.XboxOne:
                {
                    // Fix the MSVC project settings for Xbox One
                    PatchWindowsTargetPlatformVersion(vcxprojPath, vcxprojContents, "10.0.17763.0", "v141");

                    // Build for Xbox One x64
                    Deploy.VCEnvironment.BuildSolution(vsSolutionPath, configurationMsvc, "x64");
                    var depsFolder = GetThirdPartyFolder(options, platform, TargetArchitecture.x64);
                    foreach (var filename in binariesToCopyMsvc)
                        Utilities.FileCopy(Path.Combine(root, "objs", "x64", configurationMsvc, filename), Path.Combine(depsFolder, filename));

                    break;
                }
                case TargetPlatform.Linux:
                {
                    var envVars = new Dictionary<string, string>
                    {
                        { "CC", "clang-7" },
                        { "CC_FOR_BUILD", "clang-7" }
                    };
                    var buildDir = Path.Combine(root, "build");

                    // Fix scripts
                    Utilities.Run("sed", "-i -e \'s/\r$//\' autogen.sh", null, root, Utilities.RunOptions.None, envVars);
                    Utilities.Run("sed", "-i -e \'s/\r$//\' configure", null, root, Utilities.RunOptions.None, envVars);
                    Utilities.Run("chmod", "+x autogen.sh", null, root, Utilities.RunOptions.None);
                    Utilities.Run("chmod", "+x configure", null, root, Utilities.RunOptions.None);

                    Utilities.Run(Path.Combine(root, "autogen.sh"), null, null, root, Utilities.RunOptions.Default, envVars);

                    // Disable using libpng even if it's found on the system
                    var cmakeFile = Path.Combine(root, "CMakeLists.txt");
                    File.WriteAllText(cmakeFile,
                                      File.ReadAllText(cmakeFile)
                                          .Replace("find_package(PNG)", "")
                                          .Replace("find_package(ZLIB)", "")
                                          .Replace("find_package(BZip2)", "")
                                     );

                    // Build for Linux
                    SetupDirectory(buildDir, true);
                    var toolchain = UnixToolchain.GetToolchainName(platform, TargetArchitecture.x64);
                    Utilities.Run("cmake", string.Format("-G \"Unix Makefiles\" -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DFT_WITH_BZIP2=OFF -DFT_WITH_ZLIB=OFF -DFT_WITH_PNG=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER_TARGET={0} ..", toolchain), null, buildDir, Utilities.RunOptions.None, envVars);
                    Utilities.Run("cmake", "--build .", null, buildDir, Utilities.RunOptions.None, envVars);
                    var depsFolder = GetThirdPartyFolder(options, platform, TargetArchitecture.x64);
                    Utilities.FileCopy(Path.Combine(buildDir, libraryFileName), Path.Combine(depsFolder, libraryFileName));

                    break;
                }
                case TargetPlatform.PS4:
                {
                    // Get the build data files
                    Utilities.DirectoryCopy(
                                            Path.Combine(GetBinariesFolder(options, platform), "Data", "freetype"),
                                            Path.Combine(root, "builds", "PS4"), false, true);

                    // Build for PS4
                    var solutionPath = Path.Combine(root, "builds", "PS4", "freetype.sln");
                    Deploy.VCEnvironment.BuildSolution(solutionPath, "Release", "ORBIS");
                    var depsFolder = GetThirdPartyFolder(options, platform, TargetArchitecture.x64);
                    Utilities.FileCopy(Path.Combine(root, "lib", "PS4", libraryFileName), Path.Combine(depsFolder, libraryFileName));

                    break;
                }
                case TargetPlatform.XboxScarlett:
                {
                    // Fix the MSVC project settings for Xbox Scarlett
                    PatchWindowsTargetPlatformVersion(vcxprojPath, vcxprojContents, "10.0.19041.0", "v142");

                    // Build for Xbox Scarlett
                    Deploy.VCEnvironment.BuildSolution(vsSolutionPath, configurationMsvc, "x64");
                    var depsFolder = GetThirdPartyFolder(options, platform, TargetArchitecture.x64);
                    foreach (var filename in binariesToCopyMsvc)
                        Utilities.FileCopy(Path.Combine(root, "objs", "x64", configurationMsvc, filename), Path.Combine(depsFolder, filename));

                    break;
                }
                case TargetPlatform.Android:
                {
                    var buildDir = Path.Combine(root, "build");

                    // Disable using libpng even if it's found on the system
                    var cmakeFile = Path.Combine(root, "CMakeLists.txt");
                    File.WriteAllText(cmakeFile,
                                      File.ReadAllText(cmakeFile)
                                          .Replace("find_package(PNG)", "")
                                          .Replace("find_package(ZLIB)", "")
                                          .Replace("find_package(BZip2)", "")
                                     );

                    // Build for Android
                    SetupDirectory(buildDir, true);
                    RunCmake(buildDir, TargetPlatform.Android, TargetArchitecture.ARM64, ".. -DFT_WITH_BZIP2=OFF -DFT_WITH_ZLIB=OFF -DFT_WITH_PNG=OFF -DCMAKE_BUILD_TYPE=Release");
                    Utilities.Run("cmake", "--build .", null, buildDir, Utilities.RunOptions.None);
                    var depsFolder = GetThirdPartyFolder(options, platform, TargetArchitecture.ARM64);
                    Utilities.FileCopy(Path.Combine(buildDir, libraryFileName), Path.Combine(depsFolder, libraryFileName));
                    break;
                }
                }
            }

            // Backup files
            var srcIncludePath = Path.Combine(root, "include", "freetype");
            var dstIncludePath = Path.Combine(options.ThirdPartyFolder, "freetype");
            foreach (var filename in filesToKeep)
            {
                var src = Path.Combine(dstIncludePath, filename);
                var dst = Path.Combine(options.IntermediateFolder, filename + ".tmp");
                Utilities.FileCopy(src, dst);
            }

            // Setup headers directory
            SetupDirectory(dstIncludePath, true);

            // Deploy header files and restore files
            Utilities.DirectoryCopy(srcIncludePath, dstIncludePath, true, true);
            Utilities.FileCopy(Path.Combine(root, "include", "ft2build.h"), Path.Combine(dstIncludePath, "ft2build.h"));
            Utilities.FileCopy(Path.Combine(root, "docs", "LICENSE.TXT"), Path.Combine(dstIncludePath, "LICENSE.TXT"));
            foreach (var filename in filesToKeep)
            {
                var src = Path.Combine(options.IntermediateFolder, filename + ".tmp");
                var dst = Path.Combine(dstIncludePath, filename);
                Utilities.FileCopy(src, dst);
            }
        }
    }
}
