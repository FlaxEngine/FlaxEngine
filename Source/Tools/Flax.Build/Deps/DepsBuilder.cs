// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.IO;
using System.Linq;
using System.Text;
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
            // Fix error 'IBM437' is not a supported encoding name from Ionic.Zip.ZipFile
            Encoding.RegisterProvider(CodePagesEncodingProvider.Instance);

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
            platforms = platforms.Where(buildPlatform.CanBuildPlatform).ToArray();
            var architectures = Globals.AllArchitectures;
            if (Configuration.BuildArchitectures != null && Configuration.BuildArchitectures.Length != 0)
                architectures = Configuration.BuildArchitectures;
            architectures = architectures.Where(buildPlatform.CanBuildArchitecture).ToArray();
            Log.Verbose($"Building deps for platforms {string.Join(',', platforms)}, {string.Join(',', architectures)}:");
            foreach (var platform in platforms)
            {
                foreach (var architecture in architectures)
                {
                    Log.Verbose($" - {platform} ({architecture})");

                    if (Platform.IsPlatformSupported(platform, architecture))
                        SetupDepsOutputFolder(options, platform, architecture);
                }
            }

            // Get all deps
            var dependencies = Builder.BuildTypes.Where(x => x.IsSubclassOf(typeof(Dependency))).Select(Activator.CreateInstance).Cast<Dependency>().ToArray();
            if (dependencies.Length == 0)
                Log.Warning("No dependencies found!");
            for (var i = 0; i < dependencies.Length; i++)
            {
                var dependency = dependencies[i];
                var name = dependency.GetType().Name;
                if (depsToBuild.Length > 0 || !dependency.BuildByDefault)
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

                options.Architectures = architectures.Intersect(dependency.Architectures).ToArray();
                if (options.Architectures.Length == 0)
                {
                    Log.Info(string.Format("Skipping {0} ({1}/{2})", name, i + 1, dependencies.Length));
                    Log.Verbose("Architecture not used on any of the build platforms.");
                    continue;
                }

                Log.Info(string.Format("Building {0} ({1}/{2})", name, i + 1, dependencies.Length));

                options.IntermediateFolder = Path.Combine(Environment.CurrentDirectory, "Cache", "Intermediate", "Deps", name).Replace('\\', '/');
                if (!Configuration.ReBuildDeps && Directory.Exists(options.IntermediateFolder))
                {
                    Log.Verbose(string.Format("{0} is up-to-date. Skipping build.", name));
                    continue;
                }

                var forceEmpty = false; //Configuration.ReBuildDeps;
                Dependency.SetupDirectory(options.IntermediateFolder, forceEmpty);

                dependency.Build(options);
            }

            Log.Info("Done!");

            return false;
        }
    }
}
