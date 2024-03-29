// Copyright (c) 2012-2024 Flax Engine. All rights reserved.

#define USE_STD

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Text;
using Flax.Build;
using Flax.Build.Platforms;

namespace Flax.Deploy
{
    partial class Deployment
    {
        private static void CodeSign(string file)
        {
            if (string.IsNullOrEmpty(Configuration.DeployCert))
                return;
            Log.Info("Code signing file: " + file);
            switch (Platform.BuildTargetPlatform)
            {
            case TargetPlatform.Windows:
                VCEnvironment.CodeSign(file, Configuration.DeployCert, Configuration.DeployCertPass);
                break;
            case TargetPlatform.Mac:
                MacPlatform.CodeSign(file, Configuration.DeployCert);
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

                    var buildToolExe = Platform.BuildTargetPlatform == TargetPlatform.Windows ? "Flax.Build.exe" : "Flax.Build";
                    DeployFile(src, dst, buildToolExe);
                    CodeSign(Path.Combine(dst, buildToolExe));
                    var buildToolDll = "Flax.Build.dll";
                    DeployFile(src, dst, buildToolDll);
                    CodeSign(Path.Combine(dst, buildToolDll));
                    DeployFile(src, dst, "Flax.Build.xml", true);
                    DeployFile(src, dst, "Flax.Build.pdb");
                    DeployFile(src, dst, "Flax.Build.deps.json");
                    DeployFile(src, dst, "Flax.Build.runtimeconfig.json");
                    DeployFile(src, dst, "Ionic.Zip.Reduced.dll");
                    DeployFile(src, dst, "Mono.Cecil.dll");
                    DeployFile(src, dst, "Microsoft.CodeAnalysis.dll");
                    DeployFile(src, dst, "Microsoft.CodeAnalysis.CSharp.dll");
                    DeployFile(src, dst, "Microsoft.VisualStudio.Setup.Configuration.Interop.dll");
                }

                // Deploy content
                DeployFolder(RootPath, OutputPath, "Content");

                // Deploy DotNet deps
                {
                    var subDir = "Source/Platforms/DotNet";
                    DeployFile(RootPath, OutputPath, subDir, "Newtonsoft.Json.dll");
                    DeployFile(RootPath, OutputPath, subDir, "Newtonsoft.Json.xml");
                    DeployFile(RootPath, OutputPath, Path.Combine(subDir, "AOT"), "Newtonsoft.Json.dll");
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

                // When deploying Editor with Platforms at once then bundle them inside it
                if (Configuration.DeployPlatforms && Platforms.PackagedPlatforms != null)
                {
                    foreach (var platform in Platforms.PackagedPlatforms)
                    {
                        Log.Info(string.Empty);
                        Log.Info($"Bunding {platform} platform with Editor");
                        Log.Info(string.Empty);

                        string platformName = platform.ToString();
                        string platformFiles = Path.Combine(Deployer.PackageOutputPath, platformName);
                        string platformData = Path.Combine(OutputPath, "Source", "Platforms", platformName);
                        if (Directory.Exists(platformFiles))
                        {
                            // Copy deployed files
                            Utilities.DirectoryCopy(platformFiles, platformData);
                        }
                        else
                        {
                            // Extract deployed files
                            var packageZipPath = Path.Combine(Deployer.PackageOutputPath, platformName + ".zip");
                            Log.Verbose(packageZipPath + " -> " + platformData);
                            System.IO.Compression.ZipFile.ExtractToDirectory(packageZipPath, platformData, true);
                        }
                    }
                }

                // Package Editor into macOS app
                if (Platform.BuildTargetPlatform == TargetPlatform.Mac)
                {
                    var arch = Platform.BuildTargetArchitecture;
                    Log.Info(string.Empty);
                    Log.Info("Creating macOS app...");
                    var appPath = Path.Combine(Deployer.PackageOutputPath, "FlaxEditor.app");
                    var appContentsPath = Path.Combine(appPath, "Contents");
                    var appBinariesPath = Path.Combine(appContentsPath, "MacOS");
                    Utilities.DirectoryDelete(appPath);
                    Directory.CreateDirectory(appPath);
                    Directory.CreateDirectory(appContentsPath);
                    Directory.CreateDirectory(appBinariesPath);

                    // Copy icons set
                    Directory.CreateDirectory(Path.Combine(appContentsPath, "Resources"));
                    Utilities.FileCopy(Path.Combine(Globals.EngineRoot, "Source/Platforms/Mac/Default.icns"), Path.Combine(appContentsPath, "Resources/icon.icns"));

                    // Create PkgInfo file
                    File.WriteAllText(Path.Combine(appContentsPath, "PkgInfo"), "APPL???", Encoding.ASCII);

                    // Create Info.plist
                    var plist = File.ReadAllText(Path.Combine(Globals.EngineRoot, "Source/Platforms/Mac/Default.plist"));
                    var flaxProject = EngineTarget.EngineProject;
                    plist = plist.Replace("{Version}", flaxProject.Version.ToString());
                    plist = plist.Replace("{Copyright}", flaxProject.Copyright);
                    plist = plist.Replace("{Executable}", "FlaxEditor");
                    plist = plist.Replace("{Arch}", arch == TargetArchitecture.ARM64 ? "arm64" : "x86_64");
                    File.WriteAllText(Path.Combine(appContentsPath, "Info.plist"), plist, Encoding.ASCII);

                    // Copy output editor files
                    Utilities.DirectoryCopy(OutputPath, appContentsPath);

                    // Copy native binaries for app execution
                    var defaultEditorConfig = "Development";
                    var ediotrBinariesPath = Path.Combine(appContentsPath, "Binaries/Editor/Mac", defaultEditorConfig);
                    Utilities.DirectoryCopy(ediotrBinariesPath, appBinariesPath, true, true);

                    // Sign app resources
                    CodeSign(appPath);

                    // Build a disk image
                    var dmgPath = Path.Combine(Deployer.PackageOutputPath, "FlaxEditor.dmg");
                    Log.Info(string.Empty);
                    Log.Info("Building disk image...");
                    if (File.Exists(dmgPath))
                        File.Delete(dmgPath);
                    Utilities.Run("hdiutil", $"create -srcFolder \"{appPath}\" -o \"{dmgPath}\"", null, null, Utilities.RunOptions.Default | Utilities.RunOptions.ThrowExceptionOnError);
                    CodeSign(dmgPath);
                    Log.Info("Output disk image size: " + Utilities.GetFileSize(dmgPath));

                    // Notarize disk image
                    if (!string.IsNullOrEmpty(Configuration.DeployKeychainProfile))
                    {
                        Log.Info(string.Empty);
                        Log.Info("Notarizing disk image...");
                        Utilities.Run("xcrun", $"notarytool submit \"{dmgPath}\" --wait --keychain-profile \"{Configuration.DeployKeychainProfile}\"", null, null, Utilities.RunOptions.Default | Utilities.RunOptions.ThrowExceptionOnError);
                        Utilities.Run("xcrun", $"stapler staple \"{dmgPath}\"", null, null, Utilities.RunOptions.Default | Utilities.RunOptions.ThrowExceptionOnError);
                        Log.Info("App notarized for macOS distribution!");
                    }
                }

                // Compress
                if (Configuration.DontCompress)
                    return;
                Log.Info(string.Empty);
                Log.Info("Compressing editor files...");
                string editorPackageZipPath;
                var unix = Platform.BuildTargetPlatform == TargetPlatform.Linux || Platform.BuildTargetPlatform == TargetPlatform.Mac;
                if (unix)
                {
                    var zipEofPath = UnixPlatform.Which("zip");
                    unix = File.Exists(zipEofPath);
                    if (!unix)
                        Log.Verbose("Using .NET compressing");
                }
                if (unix)
                {
                    // Use system tool (preserves executable file attributes and link files)
                    editorPackageZipPath = Path.Combine(Deployer.PackageOutputPath, $"FlaxEditor{Enum.GetName(typeof(TargetPlatform), Platform.BuildTargetPlatform)}.zip");
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
                var binariesSubDir = Path.Combine(Platform.GetEditorBinaryDirectory(), configuration.ToString());
                var src = Path.Combine(RootPath, binariesSubDir);
                var dst = Path.Combine(OutputPath, binariesSubDir);
                Directory.CreateDirectory(dst);

                DeployFile(src, dst, "FlaxEditor.Build.json");
                DeployFile(src, dst, "FlaxEngine.CSharp.runtimeconfig.json");
                DeployFile(src, dst, "FlaxEngine.CSharp.pdb");
                DeployFile(src, dst, "FlaxEngine.CSharp.xml");
                DeployFile(src, dst, "Newtonsoft.Json.pdb");

                if (Platform.BuildTargetPlatform == TargetPlatform.Windows)
                {
                    var dstDebug = Path.Combine(Deployer.PackageOutputPath, "EditorDebugSymbols/Win64/" + configuration);
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
                    DeployFile(src, dst, "FlaxEditor.lib");
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
                    // Deploy binaries
                    DeployFile(src, dst, "FlaxEditor");
                    DeployFiles(src, dst, "*.dll");
                    DeployFiles(src, dst, "*.so");
                    DeployFile(src, dst, "Logo.png");

                    // Optimize package size
                    Utilities.Run("strip", "FlaxEditor", null, dst, Utilities.RunOptions.None);
                    Utilities.Run("strip", "libFlaxEditor.so", null, dst, Utilities.RunOptions.None);
                }
                else if (Platform.BuildTargetPlatform == TargetPlatform.Mac)
                {
                    // Deploy binaries
                    DeployFile(src, dst, "FlaxEditor");
                    DeployFile(src, dst, "MoltenVK_icd.json");
                    DeployFiles(src, dst, "*.dll");
                    DeployFiles(src, dst, "*.dylib");

                    // Optimize package size
                    Utilities.Run("strip", "FlaxEditor", null, dst, Utilities.RunOptions.None);
                    Utilities.Run("strip", "FlaxEngine.dylib", null, dst, Utilities.RunOptions.None);
                    Utilities.Run("strip", "libMoltenVK.dylib", null, dst, Utilities.RunOptions.None);

                    // Sign binaries
                    CodeSign(Path.Combine(dst, "FlaxEditor"));
                    CodeSign(Path.Combine(dst, "FlaxEngine.dylib"));
                    CodeSign(Path.Combine(dst, "libMoltenVK.dylib"));
                }
            }
        }
    }
}
