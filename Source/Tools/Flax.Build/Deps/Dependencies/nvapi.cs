// Copyright (c) Wojciech Figat. All rights reserved.

using Flax.Build;
using System.IO;

namespace Flax.Deps.Dependencies
{
    /// <summary>
    /// NVAPI is NVIDIA's core software development kit that allows direct access to NVIDIA GPUs and drivers on supported platforms.
    /// https://github.com/NVIDIA/nvapi
    /// </summary>
    /// <seealso cref="Flax.Deps.Dependency" />
    class nvapi : Dependency
    {
        /// <inheritdoc />
        public override TargetPlatform[] Platforms
        {
            get => new[] { TargetPlatform.Windows };
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
                        TargetArchitecture.x64
                    };
                default: return new TargetArchitecture[0];
                }
            }
        }

        /// <inheritdoc />
        public override void Build(BuildOptions options)
        {
            var root = options.IntermediateFolder;
            var moduleFolder = Path.Combine(options.ThirdPartyFolder, "nvapi");

            // Get the source
            CloneGitRepoFast(root, "https://github.com/NVIDIA/nvapi.git");

            // Copy files
            foreach (var platform in options.Platforms)
            {
                BuildStarted(platform, TargetArchitecture.x64);
                var depsFolder = GetThirdPartyFolder(options, platform, TargetArchitecture.x64);
                Utilities.FileCopy(Path.Combine(root, "amd64/nvapi64.lib"), Path.Combine(depsFolder, "nvapi64.lib"));
            }

            // Copy license and header files
            Utilities.FileCopy(Path.Combine(root, "License.txt"), Path.Combine(moduleFolder, "License.txt"));
            var files = new[]
            {
                "nvHLSLExtns.h",
                "nvHLSLExtnsInternal.h",
                "nvapi.h",
                "nvapi_lite_common.h",
                "nvapi_lite_d3dext.h",
                "nvapi_lite_salstart.h",
                "nvapi_lite_salend.h",
                "nvapi_lite_sli.h",
                "nvapi_lite_stereo.h",
                "nvapi_lite_surround.h",
            };
            foreach (var file in files)
                Utilities.FileCopy(Path.Combine(root, file), Path.Combine(moduleFolder, file));
        }
    }
}
