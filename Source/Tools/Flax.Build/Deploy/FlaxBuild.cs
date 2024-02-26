// Copyright (c) 2012-2024 Flax Engine. All rights reserved.

using System;
using System.IO;
using Flax.Build;

namespace Flax.Deploy
{
    /// <summary>
    /// Flax.Build environment and tools.
    /// </summary>
    class FlaxBuild
    {
        public static void Build(string root, string target, TargetPlatform platform, TargetArchitecture architecture, TargetConfiguration configuration)
        {
            var buildPlatform = Platform.BuildPlatform.Target;
            var flaxBuildTool = Path.Combine(Globals.EngineRoot, buildPlatform == TargetPlatform.Windows ? "Binaries/Tools/Flax.Build.exe" : "Binaries/Tools/Flax.Build");
            var format = "-build -buildtargets={0} -log -logfile= -platform={1} -arch={2} -configuration={3}";
            var cmdLine = string.Format(format, target, platform, architecture, configuration);
            Configuration.PassArgs(ref cmdLine);

            Log.Info($"Building {target} for {platform} {architecture} {configuration}...");
            int result = Utilities.Run(flaxBuildTool, cmdLine, null, root);
            if (result != 0)
            {
                throw new Exception(string.Format("Unable to build target {0}. Flax.Build failed. See log to learn more.", target));
            }
        }
    }
}
