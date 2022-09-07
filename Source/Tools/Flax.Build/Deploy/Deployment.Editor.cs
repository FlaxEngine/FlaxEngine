// Copyright (c) 2012-2020 Flax Engine. All rights reserved.

#define USE_STD

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using Flax.Build;

namespace Flax.Deploy
{
    partial class Deployment
    {
        private static void CodeSign(string file)
        {
            if (string.IsNullOrEmpty(Configuration.DeployCert))
                return;
            switch (Platform.BuildTargetPlatform)
            {
            case TargetPlatform.Windows:
                VCEnvironment.CodeSign(file, Configuration.DeployCert, Configuration.DeployCertPass);
                break;
            }
        }

        public class Editor
        {
            private static string RootPath;
            private static string OutputPath;

            public static void Package()
            {
                // Prepare
                RootPath = Globals.EngineRoot;
                OutputPath = Path.Combine(Deployer.PackageOutputPath, "Editor");
                Utilities.DirectoryDelete(OutputPath);
                Directory.CreateDirectory(OutputPath);
                Log.Info(string.Empty);
                Log.Info("Deploy editor files");
                Log.Info(string.Empty);

                // Deploy binaries
                foreach (var configuration in Deployer.Configurations)
                    DeployEditorBinaries(configuration);
                {
                    var binariesSubDir = "Binaries/Tools";
                    var src = Path.Combine(RootPath, binariesSubDir);
                    var dst = Path.Combine(OutputPath, binariesSubDir);

                    DeployFile(src, dst, "Flax.Build.exe");
                    CodeSign(Path.Combine(dst, "Flax.Build.exe"));
                    DeployFile(src, dst, "Flax.Build.xml");
                    DeployFile(src, dst, "Ionic.Zip.Reduced.dll");
                    DeployFile(src, dst, "Newtonsoft.Json.dll");
                }

                // Deploy content
                DeployFolder(RootPath, OutputPath, "Content");

                // Deploy Mono runtime data files
                switch (Platform.BuildTargetPlatform)
                {
                case TargetPlatform.Windows:
                    DeployFolder(RootPath, OutputPath, "Source/Platforms/Editor/Windows/Mono");
                    break;
                case TargetPlatform.Linux:
                    DeployFolder(RootPath, OutputPath, "Source/Platforms/Editor/Linux/Mono");
                    break;
                case TargetPlatform.Mac:
                    DeployFolder(RootPath, OutputPath, "Source/Platforms/Editor/Mac/Mono");
                    break;
                default: throw new InvalidPlatformException(Platform.BuildTargetPlatform);
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
                        if (!module.Deploy)
                            continue;
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
                string editorPackageZipPath;
                if (Platform.BuildTargetPlatform == TargetPlatform.Linux)
                {
                    // Use system tool (preserves executable file attributes and link files)
                    editorPackageZipPath = Path.Combine(Deployer.PackageOutputPath, "FlaxEditorLinux.zip");
                    Utilities.FileDelete(editorPackageZipPath);
                    Utilities.Run("zip", "Editor.zip -r .", null, OutputPath, Utilities.RunOptions.ThrowExceptionOnError);
                    File.Move(Path.Combine(OutputPath, "Editor.zip"), editorPackageZipPath);
                }
                else if (Platform.BuildTargetPlatform == TargetPlatform.Mac)
                {
                    // Use system tool (preserves executable file attributes and link files)
                    editorPackageZipPath = Path.Combine(Deployer.PackageOutputPath, "FlaxEditorMac.zip");
                    Utilities.FileDelete(editorPackageZipPath);
                    Utilities.Run("zip", "Editor.zip -r .", null, OutputPath, Utilities.RunOptions.ThrowExceptionOnError);
                    File.Move(Path.Combine(OutputPath, "Editor.zip"), editorPackageZipPath);
                }
                else
                {
                    editorPackageZipPath = Path.Combine(Deployer.PackageOutputPath, "Editor.zip");
                    Utilities.FileDelete(editorPackageZipPath);
#if USE_STD
                    System.IO.Compression.ZipFile.CreateFromDirectory(OutputPath, editorPackageZipPath, System.IO.Compression.CompressionLevel.Optimal, false);
#else
                    using (var zip = new Ionic.Zip.ZipFile())
                    {
                        zip.AddDirectory(OutputPath);
                        zip.CompressionLevel = Ionic.Zlib.CompressionLevel.BestCompression;
                        zip.Comment = string.Format("Flax Editor {0}.{1}.{2}\nDate: {3}", Deployer.VersionMajor, Deployer.VersionMinor, Deployer.VersionBuild, DateTime.UtcNow);
                        zip.Save(editorPackageZipPath);
                    }
#endif
                }
                Log.Info("Compressed editor package size: " + Utilities.GetFileSize(editorPackageZipPath));

                if (Platform.BuildTargetPlatform == TargetPlatform.Windows)
                {
                    Log.Info("Compressing editor debug symbols files...");
                    editorPackageZipPath = Path.Combine(Deployer.PackageOutputPath, "EditorDebugSymbols.zip");
                    Utilities.FileDelete(editorPackageZipPath);
#if USE_STD
                    System.IO.Compression.ZipFile.CreateFromDirectory(Path.Combine(Deployer.PackageOutputPath, "EditorDebugSymbols"), editorPackageZipPath, System.IO.Compression.CompressionLevel.Optimal, false);
#else
                    using (var zip = new Ionic.Zip.ZipFile())
                    {
                        zip.AddDirectory(Path.Combine(Deployer.PackageOutputPath, "EditorDebugSymbols"));
                        zip.CompressionLevel = Ionic.Zlib.CompressionLevel.BestCompression;
                        zip.Comment = string.Format("Flax Editor {0}.{1}.{2}\nDate: {3}", Deployer.VersionMajor, Deployer.VersionMinor, Deployer.VersionBuild, DateTime.UtcNow);
                        zip.Save(editorPackageZipPath);
                    }
#endif
                    Log.Info("Compressed editor debug symbols package size: " + Utilities.GetFileSize(editorPackageZipPath));
                }

                // Cleanup
                Utilities.DirectoryDelete(OutputPath);
                Utilities.DirectoryDelete(Path.Combine(Deployer.PackageOutputPath, "EditorDebugSymbols"));
            }

            private static void DeployEditorBinaries(TargetConfiguration configuration)
            {
                if (Platform.BuildTargetPlatform == TargetPlatform.Windows)
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
                    CodeSign(Path.Combine(dst, editorExeName));
                    DeployFile(src, dst, "FlaxEditor.Build.json");
                    DeployFile(src, dst, "FlaxEditor.lib");
                    DeployFile(src, dst, "FlaxEngine.CSharp.pdb");
                    DeployFile(src, dst, "FlaxEngine.CSharp.xml");
                    DeployFile(src, dst, "Newtonsoft.Json.pdb");
                    DeployFiles(src, dst, "*.dll");
                    CodeSign(Path.Combine(dst, "FlaxEngine.CSharp.dll"));

                    // Deploy debug symbols files
                    DeployFiles(src, dstDebug, "*.pdb");
                    File.Delete(Path.Combine(dstDebug, "FlaxEngine.CSharp.pdb"));
                    File.Delete(Path.Combine(dstDebug, "Newtonsoft.Json.pdb"));
                    if (configuration == TargetConfiguration.Development)
                    {
                        // Deploy Editor executable debug file for Development configuration to improve crash reporting
                        DeployFile(src, dst, "FlaxEditor.pdb");
                    }
                }
                else if (Platform.BuildTargetPlatform == TargetPlatform.Linux)
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
                    DeployFile(src, dst, "Logo.png");

                    // Optimize package size
                    Utilities.Run("strip", "FlaxEditor", null, dst, Utilities.RunOptions.None);
                    Utilities.Run("strip", "libFlaxEditor.so", null, dst, Utilities.RunOptions.None);
                    Utilities.Run("strip", "libmonosgen-2.0.so", null, dst, Utilities.RunOptions.None);
                    Utilities.Run("ln", "-s libmonosgen-2.0.so libmonosgen-2.0.so.1", null, dst, Utilities.RunOptions.None);
                    Utilities.Run("ln", "-s libmonosgen-2.0.so libmonosgen-2.0.so.1.0.0", null, dst, Utilities.RunOptions.None);
                }
                else if (Platform.BuildTargetPlatform == TargetPlatform.Mac)
                {
                    var binariesSubDir = "Binaries/Editor/Mac/" + configuration;
                    var src = Path.Combine(RootPath, binariesSubDir);
                    var dst = Path.Combine(OutputPath, binariesSubDir);
                    Directory.CreateDirectory(dst);

                    // Deploy binaries
                    DeployFile(src, dst, "FlaxEditor");
                    DeployFile(src, dst, "FlaxEditor.Build.json");
                    DeployFile(src, dst, "FlaxEngine.CSharp.pdb");
                    DeployFile(src, dst, "FlaxEngine.CSharp.xml");
                    DeployFile(src, dst, "Newtonsoft.Json.pdb");
                    DeployFile(src, dst, "MoltenVK_icd.json");
                    DeployFiles(src, dst, "*.dll");
                    DeployFiles(src, dst, "*.dylib");

                    // Optimize package size
                    Utilities.Run("strip", "FlaxEditor", null, dst, Utilities.RunOptions.None);
                    Utilities.Run("strip", "FlaxEditor.dylib", null, dst, Utilities.RunOptions.None);
                    Utilities.Run("strip", "libmonosgen-2.0.1.dylib", null, dst, Utilities.RunOptions.None);
                    Utilities.Run("strip", "libMoltenVK.dylib", null, dst, Utilities.RunOptions.None);
                }
                else
                {
                    throw new NotImplementedException();
                }
            }
        }
    }
}
