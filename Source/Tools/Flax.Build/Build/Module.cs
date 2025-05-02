// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using Flax.Build.NativeCpp;

namespace Flax.Build
{
    /// <summary>
    /// Defines a module that can be compiled and used by other modules and targets.
    /// </summary>
    public class Module
    {
        /// <summary>
        /// The module name.
        /// </summary>
        public string Name;

        /// <summary>
        /// The module file path.
        /// </summary>
        public string FilePath;

        /// <summary>
        /// The path of the folder that contains this module file.
        /// </summary>
        public string FolderPath;

        /// <summary>
        /// The name for the output binary module. Can be used to merge multiple native modules into single library. If set to null or <see cref="string.Empty"/> the module won't be using scripting API features.
        /// </summary>
        public string BinaryModuleName;

        /// <summary>
        /// True if module has native code to build. Can be used for C#-only modules.
        /// </summary>
        public bool BuildNativeCode = true;

        /// <summary>
        /// True if module has C# code to build. Can be used for native modules without C# bindings nor code.
        /// </summary>
        public bool BuildCSharp = true;

        /// <summary>
        /// True if module can be deployed.
        /// </summary>
        public bool Deploy = true;

        /// <summary>
        /// Custom module tags. Used by plugins and external tools.
        /// </summary>
        public Dictionary<string, string> Tags = new Dictionary<string, string>();

        /// <summary>
        /// Initializes a new instance of the <see cref="Module"/> class.
        /// </summary>
        public Module()
        {
            var type = GetType();
            Name = BinaryModuleName = type.Name;
        }

        /// <summary>
        /// Initializes the module properties.
        /// </summary>
        public virtual void Init()
        {
        }

        /// <summary>
        /// Setups the module build options. Can be used to specify module dependencies, external includes and other settings.
        /// </summary>
        /// <param name="options">The module build options.</param>
        public virtual void Setup(BuildOptions options)
        {
            options.ScriptingAPI.Defines.Add(GetCSharpBuildDefine(options.Configuration));
            options.ScriptingAPI.Defines.Add(GetCSharpPlatformDefine(options.Platform.Target));
            if ((options.Platform != null && !options.Platform.HasDynamicCodeExecutionSupport) || Configuration.AOTMode != DotNetAOTModes.None)
            {
                options.ScriptingAPI.Defines.Add("USE_AOT");
            }
        }

        internal static string GetCSharpBuildDefine(TargetConfiguration configuration)
        {
            switch (configuration)
            {
            case TargetConfiguration.Debug: return "BUILD_DEBUG";
            case TargetConfiguration.Development: return "BUILD_DEVELOPMENT";
            case TargetConfiguration.Release: return "BUILD_RELEASE";
            default: throw new Exception();
            }
        }

        internal static string GetCSharpPlatformDefine(TargetPlatform platform)
        {
            switch (platform)
            {
            case TargetPlatform.Windows: return "PLATFORM_WINDOWS";
            case TargetPlatform.XboxOne: return "PLATFORM_XBOX_ONE";
            case TargetPlatform.UWP: return "PLATFORM_UWP";
            case TargetPlatform.Linux: return "PLATFORM_LINUX";
            case TargetPlatform.PS4: return "PLATFORM_PS4";
            case TargetPlatform.PS5: return "PLATFORM_PS5";
            case TargetPlatform.XboxScarlett: return "PLATFORM_XBOX_SCARLETT";
            case TargetPlatform.Android: return "PLATFORM_ANDROID";
            case TargetPlatform.Switch: return "PLATFORM_SWITCH";
            case TargetPlatform.Mac: return "PLATFORM_MAC";
            case TargetPlatform.iOS: return "PLATFORM_IOS";
            default: throw new InvalidPlatformException(platform);
            }
        }

        /// <summary>
        /// Setups the module building environment. Allows to modify compiler and linker options.
        /// </summary>
        /// <param name="options">The module build options.</param>
        public virtual void SetupEnvironment(BuildOptions options)
        {
            options.CompileEnv.PreprocessorDefinitions.AddRange(options.PublicDefinitions);
            options.CompileEnv.PreprocessorDefinitions.AddRange(options.PrivateDefinitions);

            options.CompileEnv.IncludePaths.AddRange(options.PublicIncludePaths);
            options.CompileEnv.IncludePaths.AddRange(options.PrivateIncludePaths);
        }

        /// <summary>
        /// Gets the files to deploy.
        /// </summary>
        /// <param name="files">The output files list.</param>
        public virtual void GetFilesToDeploy(List<string> files)
        {
            // By default deploy all C++ header files
            files.AddRange(Directory.GetFiles(FolderPath, "*.h", SearchOption.AllDirectories));
        }

        /// <summary>
        /// Adds the file to the build sources if exists.
        /// </summary>
        /// <param name="options">The options.</param>
        /// <param name="path">The source file path.</param>
        protected void AddSourceFileIfExists(BuildOptions options, string path)
        {
            if (File.Exists(path))
                options.SourceFiles.Add(path);
        }
    }
}
