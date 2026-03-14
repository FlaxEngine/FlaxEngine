// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.IO;
using Flax.Build;

namespace Flax.Deps.Dependencies
{
    /// <summary>
    /// GPU compressed texture interchange system from Binomial LLC that supports two intermediate file formats.
    /// </summary>
    /// <seealso cref="Flax.Deps.Dependency" />
    class basis_universal : Dependency
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
                        TargetPlatform.Web,
                    };
                case TargetPlatform.Linux:
                    return new[]
                    {
                        TargetPlatform.Linux,
                    };
                default:
                    return new TargetPlatform[0];
                }
            }
        }

        /// <inheritdoc />
        public override TargetArchitecture[] Architectures
        {
            get
            {
                switch (BuildPlatform)
                {
                case TargetPlatform.Windows:
                    return new[]
                    {
                        TargetArchitecture.x86,
                        TargetArchitecture.x64,
                        TargetArchitecture.ARM64,
                    };
                case TargetPlatform.Linux:
                    return new[]
                    {
                        TargetArchitecture.x64,
                    };
                default:
                    return new TargetArchitecture[0];
                }
            }
        }

        /// <inheritdoc />
        public override void Build(BuildOptions options)
        {
            var root = options.IntermediateFolder;
            var buildDir = Path.Combine(root, "build");
            var configuration = "Release";
            var defines = new Dictionary<string, string>()
            {
                // Disable unused formats and features
                { "BASISD_SUPPORT_ATC", "0" },
                { "BASISD_SUPPORT_FXT1", "0" },
                { "BASISD_SUPPORT_PVRTC1", "0" },
                { "BASISD_SUPPORT_PVRTC2", "0" },
                { "BASISD_SUPPORT_ETC2_EAC_A8", "0" },
                { "BASISD_SUPPORT_ETC2_EAC_RG11", "0" },
                { "BASISD_SUPPORT_BC7", "0" },
            };

            // Get the source
            var commit = "2c4f52f705f697660cc7a9d481d9d496f85b9959"; // 'final snapshot of v2.0 before v2.1'
            CloneGitRepo(root, "https://github.com/BinomialLLC/basis_universal.git", commit);

            // Disable KTX2 support
            var headerPath = Path.Combine(root, "transcoder/basisu_transcoder.h");
            //Utilities.ReplaceInFile(headerPath, "#define BASISD_SUPPORT_KTX2 1", "#define BASISD_SUPPORT_KTX2 0");
            Utilities.ReplaceInFile(headerPath, "#define BASISD_SUPPORT_KTX2_ZSTD 1", "#define BASISD_SUPPORT_KTX2_ZSTD 0");

            // Fix build for Web with Emscripten
            var cmakeListsPath = Path.Combine(root, "CMakeLists.txt");
            Utilities.ReplaceInFile(cmakeListsPath, "if (CMAKE_SYSTEM_NAME STREQUAL \"WASI\")", "if (CMAKE_SYSTEM_NAME STREQUAL \"WASI\" OR BASISU_BUILD_WASM)");
            Utilities.ReplaceInFile(cmakeListsPath, "set(BASISU_STATIC OFF CACHE BOOL \"\" FORCE)", "");

            foreach (var platform in options.Platforms)
            {
                foreach (var architecture in options.Architectures)
                {
                    if (!BuildStarted(platform, architecture))
                        continue;

                    var cmakeArgs = $"-DCMAKE_BUILD_TYPE={configuration} -DBASISU_STATIC=ON -DBASISU_EXAMPLES=OFF -DBASISU_ZSTD=OFF -DBASISU_OPENCL=OFF";
                    cmakeArgs += " -DCMAKE_CXX_FLAGS=\"";
                    if (platform == TargetPlatform.Windows)
                    {
                        foreach (var define in defines)
                            cmakeArgs += $"/D{define.Key}={define.Value} ";
                    }
                    else
                    {
                        foreach (var define in defines)
                            cmakeArgs += $"-D{define.Key}={define.Value} ";
                    }
                    if (platform == TargetPlatform.Web && Configuration.WebThreads)
                        cmakeArgs += "-pthread ";
                    cmakeArgs += "\"";

                    var depsFolder = GetThirdPartyFolder(options, platform, architecture);
                    SetupDirectory(buildDir, true);
                    switch (platform)
                    {
                    case TargetPlatform.Windows:
                    {
                        cmakeArgs = ".. -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL " + cmakeArgs;
                        RunCmake(buildDir, platform, architecture, cmakeArgs);
                        BuildCmake(buildDir, configuration, options:Utilities.RunOptions.ConsoleLogOutput);
                        CopyLib(platform, Path.Combine(buildDir, configuration), depsFolder, "basisu_encoder"); 
                        break;
                    }
                    case TargetPlatform.Web:
                    {
                        cmakeArgs = ".. -DBASISU_BUILD_WASM=ON -DBASISU_WASM_THREADING=OFF " + cmakeArgs;
                        RunCmake(buildDir, platform, architecture, cmakeArgs);
                        BuildCmake(buildDir, configuration, options:Utilities.RunOptions.ConsoleLogOutput);
                        CopyLib(platform, buildDir, depsFolder, "basisu_encoder");
                        break;
                    }
                    case TargetPlatform.Linux:
                    {
                        cmakeArgs = ".. " + cmakeArgs;
                        RunCmake(buildDir, platform, architecture, cmakeArgs);
                        BuildCmake(buildDir, configuration, options:Utilities.RunOptions.ConsoleLogOutput);
                        CopyLib(platform, buildDir, depsFolder, "basisu_encoder");
                        break;
                    }
                    }
                }
            }

            // Copy headers and license
            SetupDirectory(Path.Combine(options.ThirdPartyFolder, "basis_universal", "encoder"), true);
            SetupDirectory(Path.Combine(options.ThirdPartyFolder, "basis_universal", "transcoder"), true);
            Utilities.DirectoryCopy(Path.Combine(root, "encoder"), Path.Combine(options.ThirdPartyFolder, "basis_universal", "encoder"), false, true, "*.h");
            Utilities.DirectoryCopy(Path.Combine(root, "transcoder"), Path.Combine(options.ThirdPartyFolder, "basis_universal", "transcoder"), false, true, "*.h");
            Utilities.FileCopy(Path.Combine(root, "LICENSE"), Path.Combine(options.ThirdPartyFolder, "basis_universal", "LICENSE"));

            // Fix C++17 usage
            Utilities.ReplaceInFile(Path.Combine(options.ThirdPartyFolder, "basis_universal/transcoder/basisu.h"), "if constexpr", "if IF_CONSTEXPR");
            Utilities.ReplaceInFile(Path.Combine(options.ThirdPartyFolder, "basis_universal/transcoder/basisu_containers.h"), "if constexpr", "if IF_CONSTEXPR");
            Utilities.ReplaceInFile(Path.Combine(options.ThirdPartyFolder, "basis_universal/encoder/basisu_math.h"), "static_assert(", "// static_assert(");
        }
    }
}
