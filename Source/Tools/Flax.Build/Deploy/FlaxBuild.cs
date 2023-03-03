// Copyright (c) 2012-2023 Flax Engine. All rights reserved.

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
            var flaxBuildTool = Path.Combine(Globals.EngineRoot, "Binaries/Tools/Flax.Build.exe");
            var format = "-build -buildtargets={0} -log -logfile= -perf -platform={1} -arch={2} -configuration={3}";
            if (buildPlatform == TargetPlatform.Linux)
            {
                format = format.Replace("-", "--");
            }
            var cmdLine = string.Format(format, target, platform, architecture, configuration);
            if (!string.IsNullOrEmpty(Configuration.Compiler))
                cmdLine += " -compiler=" + Configuration.Compiler;

            int result;
            if (buildPlatform == TargetPlatform.Windows)
            {
                result = Utilities.Run(flaxBuildTool, cmdLine, null, root);
            }
            else
            {
                result = Utilities.Run("mono", flaxBuildTool + " " + cmdLine, null, root);
            }

            if (result != 0)
            {
                throw new Exception(string.Format("Unable to build target {0}. Flax.Build failed. See log to learn more.", target));
            }
        }
    }
}
