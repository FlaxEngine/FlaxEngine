// Copyright (c) 2012-2020 Flax Engine. All rights reserved.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using Flax.Build;
using Ionic.Zip;
using Ionic.Zlib;

namespace Flax.Deploy
{
    partial class Deployment
    {
        public class Editor
        {
            private static string RootPath;
            private static string OutputPath;

            public static void Package()
            {
                // Prepare
                RootPath = Globals.EngineRoot;
                OutputPath = Path.Combine(Deployer.PackageOutputPath, "Editor");
                Directory.CreateDirectory(OutputPath);
                Log.Info(string.Empty);
                Log.Info("Deploy editor files");
                Log.Info(string.Empty);

                // Deploy binaries
                DeployEditorBinaries(TargetConfiguration.Debug);
                DeployEditorBinaries(TargetConfiguration.Development);
                DeployEditorBinaries(TargetConfiguration.Release);
                {
                    var binariesSubDir = "Binaries/Tools";
                    var src = Path.Combine(RootPath, binariesSubDir);
                    var dst = Path.Combine(OutputPath, binariesSubDir);

                    DeployFile(src, dst, "Flax.Build.exe");
                    DeployFile(src, dst, "Flax.Build.xml");
                    DeployFile(src, dst, "Ionic.Zip.Reduced.dll");
                    DeployFile(src, dst, "Newtonsoft.Json.dll");
                }

                // Deploy content
                DeployFolder(RootPath, OutputPath, "Content");

                // Deploy Mono runtime data files
                if (Platform.BuildPlatform.Target == TargetPlatform.Windows)
                {
                    DeployFolder(RootPath, OutputPath, "Source/Platforms/Editor/Windows/Mono");
                }
                else if (Platform.BuildPlatform.Target == TargetPlatform.Linux)
                {
                    DeployFolder(RootPath, OutputPath, "Source/Platforms/Editor/Linux/Mono");
                }
                else
                {
                    throw new NotImplementedException();
                }

                // Deploy DotNet deps
                {
                    var subDir = "Source/Platforms/DotNet";
                    DeployFile(RootPath, OutputPath, subDir, "Newtonsoft.Json.dll");
                    DeployFile(RootPath, OutputPath, subDir, "Newtonsoft.Json.xml");
                }

                // Deploy sources
                {
                    // Modules public files
                    var rules = Builder.GenerateRulesAssembly();
                    var files = new List<string>();
                    foreach (var module in rules.Modules)
                    {
                        module.GetFilesToDeploy(files);
                        files.Add(module.FilePath);
                        foreach (var file in files)
                        {
                            var src = Path.GetDirectoryName(file);
                            var dst = Path.Combine(OutputPath, Utilities.MakePathRelativeTo(src, RootPath));
                            var filename = Path.GetFileName(file);
                            DeployFile(src, dst, filename);
                        }
                        files.Clear();
                    }

                    // Shader includes
                    var shaders = Directory.GetFiles(Path.Combine(RootPath, "Source/Shaders"), "*.hlsl", SearchOption.AllDirectories);
                    foreach (var shader in shaders)
                    {
                        var localPath = Utilities.MakePathRelativeTo(shader, RootPath);
                        DeployFile(shader, Path.Combine(OutputPath, localPath));
                    }

                    // Custom engine files
                    DeployFile(RootPath, OutputPath, "Source/ThirdParty", "concurrentqueue.h");
                    DeployFile(RootPath, OutputPath, "Source", ".editorconfig");
                    DeployFile(RootPath, OutputPath, "Source", "flax.natvis");
                    DeployFile(RootPath, OutputPath, "Source", "FlaxEditor.Build.cs");
                    DeployFile(RootPath, OutputPath, "Source", "FlaxEngine.Gen.h");
                    DeployFile(RootPath, OutputPath, "Source", "FlaxGame.Build.cs");

                    // Mark deployed sources as already prebuilt
                    Utilities.ReplaceInFile(Path.Combine(OutputPath, "Source/FlaxEditor.Build.cs"), "IsPreBuilt = false;", "IsPreBuilt = true;");
                    Utilities.ReplaceInFile(Path.Combine(OutputPath, "Source/FlaxGame.Build.cs"), "IsPreBuilt = false;", "IsPreBuilt = true;");
                }

                // Deploy project
                DeployFile(RootPath, OutputPath, "Flax.flaxproj");

                // Compress
                Log.Info(string.Empty);
                Log.Info("Compressing editor files...");
                var editorPackageZipPath = Path.Combine(Deployer.PackageOutputPath, "Editor.zip");
                using (ZipFile zip = new ZipFile())
                {
                    zip.AddDirectory(OutputPath);

                    zip.CompressionLevel = CompressionLevel.BestCompression;
                    zip.Comment = string.Format("Flax Editor {0}.{1}.{2}\nDate: {3}", Deployer.VersionMajor, Deployer.VersionMinor, Deployer.VersionBuild, DateTime.UtcNow);

                    zip.Save(editorPackageZipPath);
                }
                Log.Info("Compressed editor package size: " + Utilities.GetFileSize(editorPackageZipPath));

                if (Platform.BuildPlatform.Target == TargetPlatform.Windows)
                {
                    Log.Info("Compressing editor debug symbols files...");
                    editorPackageZipPath = Path.Combine(Deployer.PackageOutputPath, "EditorDebugSymbols.zip");
                    using (ZipFile zip = new ZipFile())
                    {
                        zip.AddDirectory(Path.Combine(Deployer.PackageOutputPath, "EditorDebugSymbols"));

                        zip.CompressionLevel = CompressionLevel.BestCompression;
                        zip.Comment = string.Format("Flax Editor {0}.{1}.{2}\nDate: {3}", Deployer.VersionMajor, Deployer.VersionMinor, Deployer.VersionBuild, DateTime.UtcNow);

                        zip.Save(editorPackageZipPath);
                    }
                    Log.Info("Compressed editor debug symbols package size: " + Utilities.GetFileSize(editorPackageZipPath));
                }

                // Cleanup
                Utilities.DirectoryDelete(OutputPath);
                Utilities.DirectoryDelete(Path.Combine(Deployer.PackageOutputPath, "EditorDebugSymbols"));
            }

            private static void DeployEditorBinaries(TargetConfiguration configuration)
            {
                if (Platform.BuildPlatform.Target == TargetPlatform.Windows)
                {
                    var binariesSubDir = "Binaries/Editor/Win64/" + configuration;
                    var src = Path.Combine(RootPath, binariesSubDir);
                    var dst = Path.Combine(OutputPath, binariesSubDir);
                    var dstDebug = Path.Combine(Deployer.PackageOutputPath, "EditorDebugSymbols/Win64/" + configuration);
                    Directory.CreateDirectory(dst);
                    Directory.CreateDirectory(dstDebug);

                    // Validate that build editor app has a valid version number
                    var editorExeName = "FlaxEditor.exe";
                    var version = FileVersionInfo.GetVersionInfo(Path.Combine(src, editorExeName));
                    if (version.FileMajorPart != Deployer.VersionMajor || version.FileMinorPart != Deployer.VersionMinor || version.FileBuildPart != Deployer.VersionBuild)
                    {
                        throw new InvalidDataException("Invalid engine build number. Output " + editorExeName + " has not matching version number.");
                    }

                    // Deploy binaries
                    DeployFile(src, dst, editorExeName);
                    DeployFile(src, dst, "FlaxEditor.Build.json");
                    DeployFile(src, dst, "FlaxEditor.lib");
                    DeployFile(src, dst, "FlaxEngine.CSharp.pdb");
                    DeployFile(src, dst, "FlaxEngine.CSharp.xml");
                    DeployFile(src, dst, "Newtonsoft.Json.pdb");
                    DeployFiles(src, dst, "*.dll");

                    // Deploy debug symbols files
                    DeployFiles(src, dstDebug, "*.pdb");
                    File.Delete(Path.Combine(dstDebug, "FlaxEngine.CSharp.pdb"));
                    File.Delete(Path.Combine(dstDebug, "Newtonsoft.Json.pdb"));
                }
                else if (Platform.BuildPlatform.Target == TargetPlatform.Linux)
                {
                    var binariesSubDir = "Binaries/Editor/Linux/" + configuration;
                    var src = Path.Combine(RootPath, binariesSubDir);
                    var dst = Path.Combine(OutputPath, binariesSubDir);
                    Directory.CreateDirectory(dst);

                    // Deploy binaries
                    DeployFile(src, dst, "FlaxEditor");
                    DeployFile(src, dst, "FlaxEditor.Build.json");
                    DeployFile(src, dst, "FlaxEngine.CSharp.pdb");
                    DeployFile(src, dst, "FlaxEngine.CSharp.xml");
                    DeployFile(src, dst, "Newtonsoft.Json.pdb");
                    DeployFiles(src, dst, "*.dll");
                    DeployFiles(src, dst, "*.so");
                    DeployFile(src, dst, "libmonosgen-2.0.so.1");
                    DeployFile(src, dst, "libmonosgen-2.0.so.1.0.0");
                    DeployFile(src, dst, "Logo.png");
                }
                else
                {
                    throw new NotImplementedException();
                }
            }
        }
    }
}
