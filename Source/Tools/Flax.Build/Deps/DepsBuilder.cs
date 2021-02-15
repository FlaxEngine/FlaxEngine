// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.IO;
using System.Linq;
using Flax.Build;

namespace Flax.Deps
{
    /// <summary>
    /// Dependency packages building tool.
    /// </summary>
    class DepsBuilder
    {
        private static void SetupDepsOutputFolder(Dependency.BuildOptions options, TargetPlatform platform, TargetArchitecture architecture)
        {
            var path = Dependency.GetThirdPartyFolder(options, platform, architecture);
            if (!Directory.Exists(path))
                Directory.CreateDirectory(path);
        }

        public static bool Run()
        {
            // Setup
            var buildPlatform = Platform.BuildPlatform;
            var options = new Dependency.BuildOptions
            {
                PlatformsFolder = Path.Combine(Globals.EngineRoot, "Source", "Platforms").Replace('\\', '/'),
                ThirdPartyFolder = Path.Combine(Globals.EngineRoot, "Source", "ThirdParty").Replace('\\', '/'),
            };
            var depsToBuild = string.IsNullOrEmpty(Configuration.DepsToBuild) ? new string[0] : Configuration.DepsToBuild.Trim().ToLower().Split(',');

            // Pick platforms for build
            var platforms = Globals.AllPlatforms;
            if (Configuration.BuildPlatforms != null && Configuration.BuildPlatforms.Length != 0)
                platforms = Configuration.BuildPlatforms;
            platforms = platforms.Where(x => buildPlatform.CanBuildPlatform(x)).ToArray();
            Log.Verbose("Building deps for platforms:");
            foreach (var platform in platforms)
            {
                Log.Verbose(" - " + platform);

                if (Platform.IsPlatformSupported(platform, TargetArchitecture.x64))
                    SetupDepsOutputFolder(options, platform, TargetArchitecture.x64);
                if (Platform.IsPlatformSupported(platform, TargetArchitecture.x86))
                    SetupDepsOutputFolder(options, platform, TargetArchitecture.x86);
                if (Platform.IsPlatformSupported(platform, TargetArchitecture.ARM))
                    SetupDepsOutputFolder(options, platform, TargetArchitecture.ARM);
                if (Platform.IsPlatformSupported(platform, TargetArchitecture.ARM64))
                    SetupDepsOutputFolder(options, platform, TargetArchitecture.ARM64);
            }

            // Get all deps
            var dependencies = Builder.BuildTypes.Where(x => x.IsSubclassOf(typeof(Dependency))).Select(Activator.CreateInstance).Cast<Dependency>().ToArray();
            if (dependencies.Length == 0)
                Log.Warning("No dependencies found!");
            for (var i = 0; i < dependencies.Length; i++)
            {
                var dependency = dependencies[i];
                var name = dependency.GetType().Name;
                if (depsToBuild.Length > 0)
                {
                    if (!depsToBuild.Contains(name.ToLower()))
                    {
                        Log.Info(string.Format("Skipping {0} ({1}/{2})", name, i + 1, dependencies.Length));
                        Log.Verbose("Not selected for build.");
                        continue;
                    }
                }

                options.Platforms = platforms.Intersect(dependency.Platforms).ToArray();
                if (options.Platforms.Length == 0)
                {
                    Log.Info(string.Format("Skipping {0} ({1}/{2})", name, i + 1, dependencies.Length));
                    Log.Verbose("Not used on any of the build platforms.");
                    continue;
                }

                Log.Info(string.Format("Building {0} ({1}/{2})", name, i + 1, dependencies.Length));

                options.IntermediateFolder = Path.Combine(Environment.CurrentDirectory, "Cache", "Intermediate", "Deps", name).Replace('\\', '/');
                if (!Configuration.ReBuildDeps && Directory.Exists(options.IntermediateFolder))
                {
                    Log.Verbose(string.Format("{0} is up-to-date. Skipping build.", name));
                    continue;
                }

                Dependency.SetupDirectory(options.IntermediateFolder, true);

                dependency.Build(options);
            }

            Log.Info("Done!");

            return false;
        }
    }
}
