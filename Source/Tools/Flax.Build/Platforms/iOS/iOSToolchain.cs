// Copyright (c) Wojciech Figat. All rights reserved.

using System.IO;
using System.Collections.Generic;
using Flax.Build.NativeCpp;

namespace Flax.Build
{
    partial class Configuration
    {
        /// <summary>
        /// Specifies the minimum iOS version to use (eg. 14).
        /// </summary>
        [CommandLine("iOSMinVer", "<version>", "Specifies the minimum iOS version to use (eg. 14).")]
        public static string iOSMinVer = "14";
    }
}

namespace Flax.Build.Platforms
{
    /// <summary>
    /// The build toolchain for all iOS systems.
    /// </summary>
    /// <seealso cref="UnixToolchain" />
    public sealed class iOSToolchain : AppleToolchain
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="iOSToolchain"/> class.
        /// </summary>
        /// <param name="platform">The platform.</param>
        /// <param name="architecture">The target architecture.</param>
        public iOSToolchain(iOSPlatform platform, TargetArchitecture architecture)
        : base(platform, architecture, "iPhoneOS")
        {
        }

        /// <inheritdoc />
        public override void SetupEnvironment(BuildOptions options)
        {
            base.SetupEnvironment(options);

            options.CompileEnv.PreprocessorDefinitions.Add("PLATFORM_IOS");

            // TODO: move this to the specific module configs (eg. Platform.Build.cs)
            options.LinkEnv.InputLibraries.Add("z");
            options.LinkEnv.InputLibraries.Add("bz2");
            options.LinkEnv.InputLibraries.Add("Foundation.framework");
            options.LinkEnv.InputLibraries.Add("CoreFoundation.framework");
            options.LinkEnv.InputLibraries.Add("CoreGraphics.framework");
            options.LinkEnv.InputLibraries.Add("CoreMedia.framework");
            options.LinkEnv.InputLibraries.Add("CoreVideo.framework");
            options.LinkEnv.InputLibraries.Add("SystemConfiguration.framework");
            options.LinkEnv.InputLibraries.Add("IOKit.framework");
            options.LinkEnv.InputLibraries.Add("UIKit.framework");
            options.LinkEnv.InputLibraries.Add("QuartzCore.framework");
            options.LinkEnv.InputLibraries.Add("AVFoundation.framework");
        }

        protected override void AddArgsCommon(BuildOptions options, List<string> args)
        {
            base.AddArgsCommon(options, args);

            args.Add("-miphoneos-version-min=" + Configuration.iOSMinVer);
        }

        public override bool CompileCSharp(ref CSharpOptions options)
        {
            switch (options.Action)
            {
            case CSharpOptions.ActionTypes.GetPlatformTools:
            {
                var arch = Flax.Build.Platforms.MacPlatform.BuildingForx64 ? "x64" : "arm64";
                options.PlatformToolsPath = Path.Combine(DotNetSdk.SelectVersionFolder(Path.Combine(DotNetSdk.Instance.RootPath, $"packs/Microsoft.NETCore.App.Runtime.AOT.osx-{arch}.Cross.ios-arm64")), "tools");
                return false;
            }
            case CSharpOptions.ActionTypes.MonoCompile:
            {
                var aotCompilerPath = Path.Combine(options.PlatformToolsPath, "mono-aot-cross");
                var clangPath = Path.Combine(ToolchainPath, "usr/bin/clang");

                var inputFile = options.InputFiles[0];
                var inputFileAsm = inputFile + ".s";
                var inputFileObj = inputFile + ".o";
                var outputFileDylib = options.OutputFiles[0];
                var inputFileFolder = Path.GetDirectoryName(inputFile);

                // Setup options
                var monoAotMode = "full";
                var monoDebugMode = options.EnableDebugSymbols ? "soft-debug" : "nodebug";
                var aotCompilerArgs = $"--aot={monoAotMode},asmonly,verbose,stats,print-skipped,{monoDebugMode} -O=float32";
                if (options.EnableDebugSymbols || options.EnableToolDebug)
                    aotCompilerArgs = "--debug " + aotCompilerArgs;
                var envVars = new Dictionary<string, string>();
                envVars["MONO_PATH"] = options.AssembliesPath + ":" + options.ClassLibraryPath;
                if (options.EnableToolDebug)
                {
                    envVars["MONO_LOG_LEVEL"] = "debug";
                }

                // Run cross-compiler compiler (outputs assembly code)
                int result = Utilities.Run(aotCompilerPath, $"{aotCompilerArgs} \"{inputFile}\"", null, inputFileFolder, Utilities.RunOptions.AppMustExist | Utilities.RunOptions.ConsoleLogOutput, envVars);
                if (result != 0)
                    return true;

                // Get build args for iOS
                var clangArgs = new List<string>();
                AddArgsCommon(null, clangArgs);
                var clangArgsText = string.Join(" ", clangArgs);

                // Build object file
                result = Utilities.Run(clangPath, $"\"{inputFileAsm}\" -c -o \"{inputFileObj}\" " + clangArgsText, null, inputFileFolder, Utilities.RunOptions.AppMustExist | Utilities.RunOptions.ConsoleLogOutput, envVars);
                if (result != 0)
                    return true;

                // Build dylib file
                result = Utilities.Run(clangPath, $"\"{inputFileObj}\" -dynamiclib -fPIC -o \"{outputFileDylib}\" " + clangArgsText, null, inputFileFolder, Utilities.RunOptions.AppMustExist | Utilities.RunOptions.ConsoleLogOutput, envVars);
                if (result != 0)
                    return true;

                // Clean intermediate results
                File.Delete(inputFileAsm);
                File.Delete(inputFileObj);

                // Fix rpath id
                result = Utilities.Run("install_name_tool", $"-id \"@rpath/{Path.GetFileName(outputFileDylib)}\" \"{outputFileDylib}\"", null, inputFileFolder, Utilities.RunOptions.ConsoleLogOutput, envVars);
                if (result != 0)
                    return true;

                return false;
            }
            }
            return base.CompileCSharp(ref options);
        }
    }
}
