// Copyright (c) 2012-2020 Flax Engine. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using Flax.Build;
using Ionic.Zip;
using Ionic.Zlib;

namespace Flax.Deploy
{
    partial class Deployment
    {
        public class Platforms
        {
            public static void Package(TargetPlatform platform)
            {
                var platformsRoot = Path.Combine(Globals.EngineRoot, "Source", "Platforms");

                Log.Info(string.Empty);
                Log.Info($"Deploy {platform} platform files");
                Log.Info(string.Empty);

                string platformName = platform.ToString();
                string src = Path.Combine(platformsRoot, platformName);
                string dst = Path.Combine(Deployer.PackageOutputPath, platformName);
                Utilities.DirectoryDelete(dst);

                // Deploy files
                {
                    DeployFolder(platformsRoot, Deployer.PackageOutputPath, platformName);

                    // For Linux don't deploy engine libs used by C++ scripting linking (engine source required)
                    if (platform == TargetPlatform.Linux)
                    {
                        File.Delete(Path.Combine(dst, "Binaries", "Game", "x64", "Debug", "FlaxGame.a"));
                        File.Delete(Path.Combine(dst, "Binaries", "Game", "x64", "Development", "FlaxGame.a"));
                        File.Delete(Path.Combine(dst, "Binaries", "Game", "x64", "Release", "FlaxGame.a"));
                    }

                    // Sign binaries
                    if (platform == TargetPlatform.Windows && !string.IsNullOrEmpty(Configuration.DeployCert))
                    {
                        var binaries = Path.Combine(dst, "Binaries", "Game", "x64", "Debug");
                        CodeSign(Path.Combine(binaries, "FlaxGame.exe"));
                        CodeSign(Path.Combine(binaries, "FlaxEngine.CSharp.dll"));

                        binaries = Path.Combine(dst, "Binaries", "Game", "x64", "Development");
                        CodeSign(Path.Combine(binaries, "FlaxGame.exe"));
                        CodeSign(Path.Combine(binaries, "FlaxEngine.CSharp.dll"));

                        binaries = Path.Combine(dst, "Binaries", "Game", "x64", "Release");
                        CodeSign(Path.Combine(binaries, "FlaxGame.exe"));
                        CodeSign(Path.Combine(binaries, "FlaxEngine.CSharp.dll"));
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
                {
                    Log.Info("Compressing platform files...");

                    var packageZipPath = Path.Combine(Deployer.PackageOutputPath, platformName + ".zip");
                    Utilities.FileDelete(packageZipPath);
                    using (ZipFile zip = new ZipFile())
                    {
                        zip.AddDirectory(dst);

                        zip.CompressionLevel = CompressionLevel.BestCompression;
                        zip.Comment = string.Format("Flax Engine {0}.{1}.{2}\nPlatform: {4}\nDate: {3}", Deployer.VersionMajor, Deployer.VersionMinor, Deployer.VersionBuild, DateTime.UtcNow, platformName);

                        zip.Save(packageZipPath);
                    }

                    Log.Info(string.Format("Compressed {0} package size: {1}", platformName, Utilities.GetFileSize(packageZipPath)));
                }

                // Remove files (only zip package is used)
                Utilities.DirectoryDelete(dst);

                Log.Info(string.Empty);
            }
        }
    }
}
