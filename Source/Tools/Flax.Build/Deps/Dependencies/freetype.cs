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
            if (!File.Exists(packagePath))
                Downloader.DownloadFileFromUrlToPath("https://sourceforge.net/projects/freetype/files/freetype2/2.13.2/ft2132.zip/download", packagePath);
            using (ZipArchive archive = ZipFile.Open(packagePath, ZipArchiveMode.Read))
            {
                var newRoot = Path.Combine(root, archive.Entries.First().FullName);
                if (!Directory.Exists(newRoot))
                    archive.ExtractToDirectory(root);
                root = newRoot;
            }
            var buildDir = Path.Combine(root, "build");

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
            vcxprojContents = vcxprojContents.Replace("<<PlatformToolset>v142</PlatformToolset>", "<PlatformToolset>v143</PlatformToolset>");
            Utilities.ReplaceInFile(Path.Combine(root, "include", "freetype", "config", "ftoption.h"), "#define FT_CONFIG_OPTION_ENVIRONMENT_PROPERTIES", "");
            var msvcProps = new Dictionary<string, string>
            {
                { "WindowsTargetPlatformVersion", "10.0" },
                { "PlatformToolset", "v143" },
                //{ "RuntimeLibrary", "MultiThreadedDLL" }
            };

            foreach (var platform in options.Platforms)
            {
                BuildStarted(platform);
                switch (platform)
                {
                case TargetPlatform.Windows:
                {
                    // Patch the RuntimeLibrary value
                    File.WriteAllText(vcxprojPath, vcxprojContents);

                    // Build for Windows
                    foreach (var architecture in new[] { TargetArchitecture.x64, TargetArchitecture.ARM64 })
                    {
                        Deploy.VCEnvironment.BuildSolution(vsSolutionPath, configurationMsvc, architecture.ToString(), msvcProps);
                        var depsFolder = GetThirdPartyFolder(options, platform, architecture);
                        foreach (var filename in binariesToCopyMsvc)
                            Utilities.FileCopy(Path.Combine(root, "objs", architecture.ToString(), configurationMsvc, filename), Path.Combine(depsFolder, filename));
                    }
                    break;
                }
                case TargetPlatform.Linux:
                {
                    var envVars = new Dictionary<string, string>
                    {
                        { "CC", "clang-7" },
                        { "CC_FOR_BUILD", "clang-7" }
                    };

                    // Fix scripts
                    Utilities.Run("sed", "-i -e \'s/\r$//\' autogen.sh", null, root, Utilities.RunOptions.ThrowExceptionOnError, envVars);
                    Utilities.Run("sed", "-i -e \'s/\r$//\' configure", null, root, Utilities.RunOptions.ThrowExceptionOnError, envVars);
                    Utilities.Run("chmod", "+x autogen.sh", null, root, Utilities.RunOptions.ThrowExceptionOnError);
                    Utilities.Run("chmod", "+x configure", null, root, Utilities.RunOptions.ThrowExceptionOnError);

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
                    Utilities.Run("cmake", string.Format("-G \"Unix Makefiles\" -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DFT_WITH_BZIP2=OFF -DFT_WITH_ZLIB=OFF -DFT_WITH_PNG=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER_TARGET={0} ..", toolchain), null, buildDir, Utilities.RunOptions.ThrowExceptionOnError, envVars);
                    Utilities.Run("cmake", "--build .", null, buildDir, Utilities.RunOptions.ThrowExceptionOnError, envVars);
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
                case TargetPlatform.PS5:
                {
                    // Get the build data files
                    Utilities.DirectoryCopy(
                                            Path.Combine(GetBinariesFolder(options, platform), "Data", "freetype"),
                                            Path.Combine(root, "builds", "PS5"), false, true);
                    Utilities.ReplaceInFile(Path.Combine(root, "include\\freetype\\config\\ftstdlib.h"), "#define ft_getenv  getenv", "char* ft_getenv(const char* n);");

                    // Build for PS5
                    var solutionPath = Path.Combine(root, "builds", "PS5", "freetype.sln");
                    Deploy.VCEnvironment.BuildSolution(solutionPath, "Release", "PROSPERO");
                    var depsFolder = GetThirdPartyFolder(options, platform, TargetArchitecture.x64);
                    Utilities.FileCopy(Path.Combine(root, "lib", "PS5", libraryFileName), Path.Combine(depsFolder, libraryFileName));

                    break;
                }
                case TargetPlatform.XboxOne:
                {
                    // Build for Xbox One x64
                    Deploy.VCEnvironment.BuildSolution(vsSolutionPath, configurationMsvc, "x64", msvcProps);
                    var depsFolder = GetThirdPartyFolder(options, platform, TargetArchitecture.x64);
                    foreach (var filename in binariesToCopyMsvc)
                        Utilities.FileCopy(Path.Combine(root, "objs", "x64", configurationMsvc, filename), Path.Combine(depsFolder, filename));

                    break;
                }
                case TargetPlatform.XboxScarlett:
                {
                    // Build for Xbox Scarlett
                    Deploy.VCEnvironment.BuildSolution(vsSolutionPath, configurationMsvc, "x64", msvcProps);
                    var depsFolder = GetThirdPartyFolder(options, platform, TargetArchitecture.x64);
                    foreach (var filename in binariesToCopyMsvc)
                        Utilities.FileCopy(Path.Combine(root, "objs", "x64", configurationMsvc, filename), Path.Combine(depsFolder, filename));

                    break;
                }
                case TargetPlatform.Android:
                {
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
                    BuildCmake(buildDir);
                    var depsFolder = GetThirdPartyFolder(options, platform, TargetArchitecture.ARM64);
                    Utilities.FileCopy(Path.Combine(buildDir, libraryFileName), Path.Combine(depsFolder, libraryFileName));
                    break;
                }
                case TargetPlatform.Switch:
                {
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
                    // Fix archive creation issue due to missing ar tool
                    Utilities.ReplaceInFile(Path.Combine(root, "builds/cmake/iOS.cmake"), "set(CMAKE_SYSTEM_NAME Darwin)", "set(CMAKE_SYSTEM_NAME Darwin)\nset(CMAKE_AR ar CACHE FILEPATH \"\" FORCE)");

                    // Fix freetype toolchain rejecting min iPhone version
                    Utilities.ReplaceInFile(Path.Combine(root, "builds/cmake/iOS.cmake"), "set(CMAKE_OSX_DEPLOYMENT_TARGET \"\"", "set(CMAKE_OSX_DEPLOYMENT_TARGET \"${CMAKE_OSX_DEPLOYMENT_TARGET}\"");

                    // Build for iOS
                    SetupDirectory(buildDir, true);
                    RunCmake(buildDir, platform, TargetArchitecture.ARM64, ".. -DIOS_PLATFORM=OS -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_BUILD_TYPE=Release -DFT_WITH_BZIP2=OFF -DFT_WITH_ZLIB=OFF -DFT_WITH_PNG=OFF");
                    BuildCmake(buildDir);
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

            try
            {
                // Setup headers directory
                SetupDirectory(dstIncludePath, true);

                // Deploy header files and restore files
                Utilities.DirectoryCopy(srcIncludePath, dstIncludePath, true, true);
                Utilities.FileCopy(Path.Combine(root, "include", "ft2build.h"), Path.Combine(dstIncludePath, "ft2build.h"));
                Utilities.FileCopy(Path.Combine(root, "LICENSE.TXT"), Path.Combine(dstIncludePath, "LICENSE.TXT"));
                Utilities.FileCopy(Path.Combine(root, "docs", "FTL.TXT"), Path.Combine(dstIncludePath, "FTL.TXT"));
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
