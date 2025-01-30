// Copyright (c) 2012-2024 Flax Engine. All rights reserved.

#define USE_STD

using System;
using System.IO;
using System.Text;
using Flax.Build;

namespace Flax.Build
{
    public static partial class Configuration
    {
        /// <summary>
        /// Compresses deployed files.
        /// </summary>
        [CommandLine("deployDontCompress", "Skips compressing deployed files, and keeps files.")]
        public static bool DontCompress = false;

        /// <summary>
        /// Package deployment output path.
        /// </summary>
        [CommandLine("deployOutput", "Package deployment output path.")]
        public static string DeployOutput;

        /// <summary>
        /// Builds and packages the editor.
        /// </summary>
        [CommandLine("deployEditor", "Builds and packages the editor.")]
        public static bool DeployEditor;

        /// <summary>
        /// Builds and packages the platforms data.
        /// </summary>
        [CommandLine("deployPlatforms", "Builds and packages the platforms data.")]
        public static bool DeployPlatforms;

        /// <summary>
        /// Certificate file path for binaries signing. Or sign identity for Apple platforms.
        /// </summary>
        [CommandLine("deployCert", "Certificate file path for binaries signing. Or sign identity for Apple platforms.")]
        public static string DeployCert;

        /// <summary>
        /// Certificate file password for binaries signing.
        /// </summary>
        [CommandLine("deployCertPass", "Certificate file password for binaries signing.")]
        public static string DeployCertPass;

        /// <summary>
        /// Apple keychain profile name to use for app notarize action (installed locally).
        /// </summary>
        [CommandLine("deployKeychainProfile", "Apple keychain profile name to use for app notarize action (installed locally).")]
        public static string DeployKeychainProfile;
    }
}

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
        public static TargetConfiguration[] Configurations;

        public static bool Run()
        {
            try
            {
                Initialize();

                if (Configuration.DeployPlatforms)
                {
                    if (Configuration.BuildPlatforms == null || Configuration.BuildPlatforms.Length == 0)
                    {
                        BuildPlatform(TargetPlatform.Linux, TargetArchitecture.x64);
                        BuildPlatform(TargetPlatform.Windows, TargetArchitecture.x64);
                        BuildPlatform(TargetPlatform.Android, TargetArchitecture.ARM64);
                        if (Platform.BuildTargetPlatform == TargetPlatform.Mac)
                        {
                            BuildPlatform(TargetPlatform.Mac, Platform.BuildTargetArchitecture);
                            BuildPlatform(TargetPlatform.iOS, TargetArchitecture.ARM64);
                        }
                    }
                    else
                    {
                        var architectures = Configuration.BuildArchitectures == null || Configuration.BuildArchitectures.Length == 0 ? Globals.AllArchitectures : Configuration.BuildArchitectures;
                        foreach (var platform in Configuration.BuildPlatforms)
                        {
                            BuildPlatform(platform, architectures);
                        }
                    }

                    if (Platform.BuildTargetPlatform == TargetPlatform.Windows)
                    {
                        Log.Info("Compressing game debug symbols files...");
                        var gamePackageZipPath = Path.Combine(Deployer.PackageOutputPath, "GameDebugSymbols.zip");
                        Utilities.FileDelete(gamePackageZipPath);
#if USE_STD
                        System.IO.Compression.ZipFile.CreateFromDirectory(Path.Combine(Deployer.PackageOutputPath, "GameDebugSymbols"), gamePackageZipPath, System.IO.Compression.CompressionLevel.Optimal, false);
#else
                        using (var zip = new Ionic.Zip.ZipFile())
                        {
                            zip.AddDirectory(Path.Combine(Deployer.PackageOutputPath, "GameDebugSymbols"));
                            zip.CompressionLevel = Ionic.Zlib.CompressionLevel.BestCompression;
                            zip.Comment = string.Format("Flax Game {0}.{1}.{2}\nDate: {3}", Deployer.VersionMajor, Deployer.VersionMinor, Deployer.VersionBuild, DateTime.UtcNow);
                            zip.Save(gamePackageZipPath);
                        }
#endif
                        Log.Info("Compressed game debug symbols package size: " + Utilities.GetFileSize(gamePackageZipPath));
                    }
                    Utilities.DirectoryDelete(Path.Combine(Deployer.PackageOutputPath, "GameDebugSymbols"));
                }

                if (Configuration.DeployEditor)
                {
                    BuildEditor();
                    Deployment.Editor.Package();
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
            Configurations = Configuration.BuildConfigurations != null ? Configuration.BuildConfigurations : new[] { TargetConfiguration.Debug, TargetConfiguration.Development, TargetConfiguration.Release };

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
            if (string.IsNullOrEmpty(Configuration.DeployOutput))
                PackageOutputPath = Path.Combine(Globals.EngineRoot, string.Format("Package_{0}_{1:00}_{2:00000}", VersionMajor, VersionMinor, VersionBuild));
            else
                PackageOutputPath = Configuration.DeployOutput;
            if (!Directory.Exists(PackageOutputPath))
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
            var targetPlatform = Platform.BuildPlatform.Target;
            foreach (var configuration in Configurations)
            {
                var arch = targetPlatform == TargetPlatform.Mac ? Platform.BuildTargetArchitecture : TargetArchitecture.x64;
                FlaxBuild.Build(Globals.EngineRoot, "FlaxEditor", targetPlatform, arch, configuration);
            }
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
                if (Platform.IsPlatformSupported(platform, architecture))
                {
                    Log.Info(string.Empty);
                    Log.Info($"Build {platform} {architecture} platform");
                    Log.Info(string.Empty);

                    foreach (var configuration in Configurations)
                    {
                        FlaxBuild.Build(Globals.EngineRoot, "FlaxGame", platform, architecture, configuration);
                    }
                }
            }

            Deployment.Platforms.Package(platform);
        }
    }
}
