// Copyright (c) 2012-2024 Flax Engine. All rights reserved.

using System.Collections.Generic;
using System.IO;
using Flax.Build;

namespace Flax.Deploy
{
    partial class Deployment
    {
        public class Platforms
        {
            internal static List<TargetPlatform> PackagedPlatforms;

            public static void Package(TargetPlatform platform)
            {
                if (PackagedPlatforms == null)
                    PackagedPlatforms = new List<TargetPlatform>();
                PackagedPlatforms.Add(platform);
                var platformsRoot = Path.Combine(Globals.EngineRoot, "Source", "Platforms");

                Log.Info(string.Empty);
                Log.Info($"Deploy {platform} platform files");
                Log.Info(string.Empty);

                string platformName = platform.ToString();
                string src = Path.Combine(platformsRoot, platformName);
                string dst = Path.Combine(Deployer.PackageOutputPath, platformName);
                Utilities.DirectoryDelete(dst);

                // Deploy debug files for crashes debugging
                foreach (var configuration in new[] { TargetConfiguration.Debug, TargetConfiguration.Development, TargetConfiguration.Release  })
                {
                    if (platform == TargetPlatform.Windows)
                    {
                        var dstDebug = Path.Combine(Deployer.PackageOutputPath, $"GameDebugSymbols/{platform}/{configuration}");
                        Directory.CreateDirectory(dstDebug);
                        var binaries = Path.Combine(src, "Binaries", "Game", "x64", configuration.ToString());
                        DeployFiles(binaries, dstDebug, "*.pdb");
                    }
                }

                // Deploy files
                {
                    DeployFolder(platformsRoot, Deployer.PackageOutputPath, platformName);

                    // For Linux don't deploy engine libs used by C++ scripting linking (engine source required)
                    if (platform == TargetPlatform.Linux)
                    {
                        Utilities.FileDelete(Path.Combine(dst, "Binaries", "Game", "x64", "Debug", "FlaxGame.a"));
                        Utilities.FileDelete(Path.Combine(dst, "Binaries", "Game", "x64", "Development", "FlaxGame.a"));
                        Utilities.FileDelete(Path.Combine(dst, "Binaries", "Game", "x64", "Release", "FlaxGame.a"));
                        Utilities.FileDelete(Path.Combine(dst, "Binaries", "Game", "x64", "Debug", "FlaxEngine.a"));
                        Utilities.FileDelete(Path.Combine(dst, "Binaries", "Game", "x64", "Development", "FlaxEngine.a"));
                        Utilities.FileDelete(Path.Combine(dst, "Binaries", "Game", "x64", "Release", "FlaxEngine.a"));
                    }

                    // Sign binaries
                    if (platform == TargetPlatform.Windows && !string.IsNullOrEmpty(Configuration.DeployCert))
                    {
                        var binaries = Path.Combine(dst, "Binaries", "Game", "x64", "Debug");
                        CodeSign(Path.Combine(binaries, "FlaxGame.exe"));
                        CodeSign(Path.Combine(binaries, "FlaxEngine.dll"));
                        CodeSign(Path.Combine(binaries, "FlaxEngine.CSharp.dll"));

                        binaries = Path.Combine(dst, "Binaries", "Game", "x64", "Development");
                        CodeSign(Path.Combine(binaries, "FlaxGame.exe"));
                        CodeSign(Path.Combine(binaries, "FlaxEngine.dll"));
                        CodeSign(Path.Combine(binaries, "FlaxEngine.CSharp.dll"));

                        binaries = Path.Combine(dst, "Binaries", "Game", "x64", "Release");
                        CodeSign(Path.Combine(binaries, "FlaxGame.exe"));
                        CodeSign(Path.Combine(binaries, "FlaxEngine.dll"));
                        CodeSign(Path.Combine(binaries, "FlaxEngine.CSharp.dll"));
                    }
                    else if (platform == TargetPlatform.Mac)
                    {
                        var binaries = Path.Combine(dst, "Binaries", "Game", "arm64", "Debug");
                        CodeSign(Path.Combine(binaries, "FlaxGame"));
                        CodeSign(Path.Combine(binaries, "FlaxEngine.dylib"));

                        binaries = Path.Combine(dst, "Binaries", "Game", "arm64", "Development");
                        CodeSign(Path.Combine(binaries, "FlaxGame"));
                        CodeSign(Path.Combine(binaries, "FlaxEngine.dylib"));

                        binaries = Path.Combine(dst, "Binaries", "Game", "arm64", "Release");
                        CodeSign(Path.Combine(binaries, "FlaxGame"));
                        CodeSign(Path.Combine(binaries, "FlaxEngine.dylib"));
                    }

                    // Don't distribute engine deps
                    Utilities.DirectoryDelete(Path.Combine(dst, "Binaries", "ThirdParty"));

                    var filesToRemove = new List<string>();
                    filesToRemove.AddRange(Directory.GetFiles(dst, ".gitignore", SearchOption.AllDirectories));
                    filesToRemove.AddRange(Directory.GetFiles(dst, "*.pdb", SearchOption.AllDirectories));
                    filesToRemove.AddRange(Directory.GetFiles(dst, "*.ilk", SearchOption.AllDirectories));
                    filesToRemove.AddRange(Directory.GetFiles(dst, "*.exp", SearchOption.AllDirectories));
                    filesToRemove.ForEach(File.Delete);
                }

                // Compress
                if (!Configuration.DontCompress)
                {
                    Log.Info("Compressing platform files...");

                    var packageZipPath = Path.Combine(Deployer.PackageOutputPath, platformName + ".zip");
                    Utilities.FileDelete(packageZipPath);
#if false
                    using (var zip = new Ionic.Zip.ZipFile())
                    {
                        zip.AddDirectory(dst);

                        zip.CompressionLevel = Ionic.Zlib.CompressionLevel.BestCompression;
                        zip.ParallelDeflateThreshold = -1;
                        zip.Comment = string.Format("Flax Engine {0}.{1}.{2}\nPlatform: {4}\nDate: {3}", Deployer.VersionMajor, Deployer.VersionMinor, Deployer.VersionBuild, System.DateTime.UtcNow, platformName);

                        zip.Save(packageZipPath);
                    }
#else
                    System.IO.Compression.ZipFile.CreateFromDirectory(dst, packageZipPath);
#endif

                    Log.Info(string.Format("Compressed {0} package size: {1}", platformName, Utilities.GetFileSize(packageZipPath)));

                    // Remove files (only zip package is used)
                    Utilities.DirectoryDelete(dst);
                }

                Log.Info(string.Empty);
            }
        }
    }
}
