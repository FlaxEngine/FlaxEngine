// Copyright (c) 2012-2020 Flax Engine. All rights reserved.

using System;
using System.IO;
using System.Text;
using Flax.Build;

namespace Flax.Deploy
{
    /// <summary>
    /// The automation and deployment utility.
    /// </summary>
    class Deployer
    {
        public static string PackageOutputPath;
        public static int VersionMajor;
        public static int VersionMinor;
        public static int VersionBuild;

        public static bool Run()
        {
            try
            {
                Initialize();

                if (Configuration.DeployEditor)
                {
                    BuildEditor();
                    Deployment.Editor.Package();
                }

                if (Configuration.DeployPlatforms)
                {
                    BuildPlatform(TargetPlatform.Linux, TargetArchitecture.x64);
                    BuildPlatform(TargetPlatform.UWP, TargetArchitecture.x64);
                    BuildPlatform(TargetPlatform.Windows, TargetArchitecture.x64);
                    BuildPlatform(TargetPlatform.Android, TargetArchitecture.ARM64);
                }
            }
            catch (Exception ex)
            {
                Log.Error("Build failed!");
                Log.Exception(ex);
                return true;
            }
            finally
            {
                Cleanup();
            }

            return false;
        }

        static void Initialize()
        {
            // Read the current engine version
            var engineVersion = EngineTarget.EngineVersion;
            VersionMajor = engineVersion.Major;
            VersionMinor = engineVersion.Minor;
            VersionBuild = engineVersion.Build;

            // Generate the engine build config
            var buildConfigHeader = new StringBuilder();
            {
                buildConfigHeader.AppendLine("#pragma once");
                buildConfigHeader.AppendLine();
                buildConfigHeader.AppendLine("#define COMPILE_WITH_DEV_ENV 0");
                buildConfigHeader.AppendLine("#define OFFICIAL_BUILD 1");
            }
            Utilities.WriteFileIfChanged(Path.Combine(Globals.EngineRoot, "Source/Engine/Core/Config.Gen.h"), buildConfigHeader.ToString());

            // Prepare the package output
            PackageOutputPath = Path.Combine(Globals.EngineRoot, string.Format("Package_{0}_{1:00}_{2:00000}", VersionMajor, VersionMinor, VersionBuild));
            Utilities.DirectoryDelete(PackageOutputPath);
            Directory.CreateDirectory(PackageOutputPath);

            Log.Info(string.Empty);
            Log.Info(string.Empty);
        }

        private static void Cleanup()
        {
#if false
            // Restore the generated config file (could resource with git but do it here just in case)
            using (var writer = new StreamWriter(Path.Combine(ProjectRoot, "Source/Engine/Core/Config.Gen.h")))
            {
                writer.WriteLine("#pragma once");
                writer.WriteLine();
            }
#endif
        }

        private static void BuildEditor()
        {
            FlaxBuild.Build(Globals.EngineRoot, "FlaxEditor", TargetPlatform.Windows, TargetArchitecture.x64, TargetConfiguration.Debug);
            FlaxBuild.Build(Globals.EngineRoot, "FlaxEditor", TargetPlatform.Windows, TargetArchitecture.x64, TargetConfiguration.Development);
            FlaxBuild.Build(Globals.EngineRoot, "FlaxEditor", TargetPlatform.Windows, TargetArchitecture.x64, TargetConfiguration.Release);
        }

        private static bool CannotBuildPlatform(TargetPlatform platform)
        {
            if (!Platform.BuildPlatform.CanBuildPlatform(platform))
            {
                Log.Warning(string.Format("Cannot build for {0} on {1}", platform, Platform.BuildPlatform.Target));
                return true;
            }

            return false;
        }

        private static void BuildPlatform(TargetPlatform platform, params TargetArchitecture[] architectures)
        {
            if (CannotBuildPlatform(platform))
                return;

            foreach (var architecture in architectures)
            {
                FlaxBuild.Build(Globals.EngineRoot, "FlaxGame", platform, architecture, TargetConfiguration.Debug);
                FlaxBuild.Build(Globals.EngineRoot, "FlaxGame", platform, architecture, TargetConfiguration.Development);
                FlaxBuild.Build(Globals.EngineRoot, "FlaxGame", platform, architecture, TargetConfiguration.Release);
            }

            Deployment.Platforms.Package(platform);
        }
    }
}
