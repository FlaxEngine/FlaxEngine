// Copyright (c) Wojciech Figat. All rights reserved.

using Flax.Build;
using System.IO;

namespace Flax.Deps.Dependencies
{
    /// <summary>
    /// AMD GPU Services (AGS) library
    /// https://github.com/GPUOpen-LibrariesAndSDKs/AGS_SDK
    /// </summary>
    /// <seealso cref="Flax.Deps.Dependency" />
    class AGS : Dependency
    {
        /// <inheritdoc />
        public override TargetPlatform[] Platforms
        {
            get => new[] { TargetPlatform.Windows };
        }

        /// <inheritdoc />
        public override void Build(BuildOptions options)
        {
            var root = options.IntermediateFolder;
            var moduleFolder = Path.Combine(options.ThirdPartyFolder, "AGS");

            // Get the source
            CloneGitRepoFast(root, "https://github.com/GPUOpen-LibrariesAndSDKs/AGS_SDK.git");

            // Copy files
            foreach (var platform in options.Platforms)
            {
                BuildStarted(platform);
                var depsFolder = GetThirdPartyFolder(options, platform, TargetArchitecture.x64);
                Utilities.FileCopy(Path.Combine(root, "ags_lib/lib/amd_ags_x64.lib"), Path.Combine(depsFolder, "amd_ags_x64.lib"));
                Utilities.FileCopy(Path.Combine(root, "ags_lib/lib/amd_ags_x64.dll"), Path.Combine(depsFolder, "amd_ags_x64.dll"));
            }

            // Copy license and header files
            Utilities.FileCopy(Path.Combine(root, "LICENSE.txt"), Path.Combine(moduleFolder, "LICENSE.txt"));
            Utilities.FileCopy(Path.Combine(root, "ags_lib/inc/amd_ags.h"), Path.Combine(moduleFolder, "amd_ags.h"));
            Utilities.FileCopy(Path.Combine(root, "ags_lib/hlsl/ags_shader_intrinsics_dx11.hlsl"), Path.Combine(moduleFolder, "ags_shader_intrinsics_dx11.hlsl"));
            Utilities.FileCopy(Path.Combine(root, "ags_lib/hlsl/ags_shader_intrinsics_dx12.hlsl"), Path.Combine(moduleFolder, "ags_shader_intrinsics_dx12.hlsl"));
        }
    }
}
