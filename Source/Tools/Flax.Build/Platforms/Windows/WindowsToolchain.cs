// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using Flax.Build.Graph;
using Flax.Build.NativeCpp;

namespace Flax.Build
{
    partial class Configuration
    {
        /// <summary>
        /// Specifies the minimum Windows version to use (eg. 10).
        /// </summary>
        [CommandLine("winMinVer", "<version>", "Specifies the minimum Windows version to use (eg. 10).")]
        public static string WindowsMinVer = "10";

        /// <summary>
        /// Specifies the minimum CPU architecture type to support (on x86/x64).
        /// </summary>
        [CommandLine("winCpuArch", "<arch>", "Specifies the minimum CPU architecture type to support (on x86/x64).")]
        public static CpuArchitecture WindowsCpuArch = CpuArchitecture.SSE4_2; // 99.78% support on PC according to Steam Hardware & Software Survey: September 2025 (https://store.steampowered.com/hwsurvey/)
    }
}

namespace Flax.Build.Platforms
{
    /// <summary>
    /// The Microsoft Windows toolchain implementation.
    /// </summary>
    /// <seealso cref="Toolchain" />
    /// <seealso cref="Flax.Build.Platforms.WindowsToolchainBase" />
    public sealed class WindowsToolchain : WindowsToolchainBase
    {
        private Version _minVersion;

        /// <summary>
        /// Initializes a new instance of the <see cref="WindowsToolchain"/> class.
        /// </summary>
        /// <param name="platform">The platform.</param>
        /// <param name="architecture">The target architecture.</param>
        public WindowsToolchain(WindowsPlatform platform, TargetArchitecture architecture)
        : base(platform, architecture, WindowsPlatformToolset.Latest, WindowsPlatformSDK.Latest)
        {
            // Select minimum Windows version
            if (!Utilities.ParseVersion(Configuration.WindowsMinVer, out _minVersion))
                _minVersion = new Version(7, 0);
        }

        /// <inheritdoc />
        public override void SetupEnvironment(BuildOptions options)
        {
            base.SetupEnvironment(options);

            options.CompileEnv.PreprocessorDefinitions.Add("PLATFORM_WINDOWS");

            int winVer;
            if (_minVersion.Major >= 10)
                winVer = 0x0A00; // Windows 10
            else if (_minVersion.Major == 8 && _minVersion.Minor >= 1)
                winVer = 0x0603; // Windows 8.1
            else if (_minVersion.Major == 8)
                winVer = 0x0602; // Windows 8
            else
                winVer = 0x0601; // Windows 7
            options.CompileEnv.PreprocessorDefinitions.Add($"WINVER=0x{winVer:X4}");

            options.LinkEnv.InputLibraries.Add("dwmapi.lib");
            options.LinkEnv.InputLibraries.Add("kernel32.lib");
            options.LinkEnv.InputLibraries.Add("user32.lib");
            options.LinkEnv.InputLibraries.Add("comdlg32.lib");
            options.LinkEnv.InputLibraries.Add("advapi32.lib");
            options.LinkEnv.InputLibraries.Add("shell32.lib");
            options.LinkEnv.InputLibraries.Add("ole32.lib");
            options.LinkEnv.InputLibraries.Add("oleaut32.lib");
            options.LinkEnv.InputLibraries.Add("delayimp.lib");

            options.CompileEnv.CpuArchitecture = Configuration.WindowsCpuArch;

            if (options.Architecture == TargetArchitecture.x64)
            {
                if (_minVersion.Major <= 7 && options.CompileEnv.CpuArchitecture == CpuArchitecture.AVX2)
                {
                    // Old Windows had lower support ratio for latest CPU features
                    options.CompileEnv.CpuArchitecture = CpuArchitecture.AVX;
                }
                if (_minVersion.Major >= 11 && options.CompileEnv.CpuArchitecture == CpuArchitecture.AVX)
                {
                    // Windows 11 has hard requirement on SSE4.2
                    options.CompileEnv.CpuArchitecture = CpuArchitecture.SSE4_2;
                }
            }
            else if (options.Architecture == TargetArchitecture.ARM64)
            {
                options.CompileEnv.PreprocessorDefinitions.Add("USE_SOFT_INTRINSICS");
                options.LinkEnv.InputLibraries.Add("softintrin.lib");
                if (options.CompileEnv.CpuArchitecture != CpuArchitecture.None)
                    options.CompileEnv.CpuArchitecture = CpuArchitecture.NEON;
            }
        }

        /// <inheritdoc />
        protected override void SetupCompileCppFilesArgs(TaskGraph graph, BuildOptions options, List<string> args)
        {
            base.SetupCompileCppFilesArgs(graph, options, args);

            switch (options.CompileEnv.CpuArchitecture)
            {
            case CpuArchitecture.AVX: args.Add("/arch:AVX"); break;
            case CpuArchitecture.AVX2: args.Add("/arch:AVX2"); break;
            case CpuArchitecture.AVX512: args.Add("/arch:AVX512"); break;
            case CpuArchitecture.SSE2: args.Add("/arch:SSE2"); break;
            case CpuArchitecture.SSE4_2: args.Add("/arch:SSE4.2"); break;
            }
        }

        /// <inheritdoc />
        public override void LinkFiles(TaskGraph graph, BuildOptions options, string outputFilePath)
        {
            // Compile and include resource file if need to
            if (options.Target.Win32ResourceFile != null &&
                !options.Target.IsPreBuilt &&
                (options.LinkEnv.Output == LinkerOutput.Executable || options.LinkEnv.Output == LinkerOutput.SharedLibrary))
            {
                var task = graph.Add<CompileCppTask>();
                var args = new List<string>();
                var sourceFile = options.Target.Win32ResourceFile;

                // Suppress Startup Banner
                args.Add("/nologo");

                // Language
                args.Add("/l 0x0409");

                // Add preprocessor definitions
                foreach (var definition in options.CompileEnv.PreprocessorDefinitions)
                    args.Add(string.Format("/D \"{0}\"", definition));
                args.Add(string.Format("/D \"ORIGINAL_FILENAME=\\\"{0}\\\"\"", Path.GetFileName(outputFilePath)));
                if (TryGetGameProductName(options, out var productName))
                {
                    if (!HasDefinition(options.CompileEnv.PreprocessorDefinitions, "PRODUCT_NAME"))
                        args.Add(string.Format("/D \"PRODUCT_NAME=\\\"{0}\\\"\"", productName));
                    if (!HasDefinition(options.CompileEnv.PreprocessorDefinitions, "PRODUCT_NAME_INTERNAL"))
                        args.Add(string.Format("/D \"PRODUCT_NAME_INTERNAL=\\\"{0}\\\"\"", productName));
                }
                else
                {
                    if (!HasDefinition(options.CompileEnv.PreprocessorDefinitions, "PRODUCT_NAME"))
                        args.Add(string.Format("/D \"PRODUCT_NAME=\\\"{0}\\\"\"", options.Target.ProjectName));
                    if (!HasDefinition(options.CompileEnv.PreprocessorDefinitions, "PRODUCT_NAME_INTERNAL"))
                        args.Add(string.Format("/D \"PRODUCT_NAME_INTERNAL=\\\"{0}\\\"\"", options.Target.Name));
                }

                // Add include paths
                foreach (var includePath in options.CompileEnv.IncludePaths)
                    AddIncludePath(args, includePath);

                // Add the resource file to the produced item list
                var outputFile = Path.Combine(options.IntermediateFolder, Path.GetFileName(outputFilePath) + ".res");
                args.Add(string.Format("/Fo\"{0}\"", outputFile));
                options.LinkEnv.InputFiles.Add(outputFile);

                // Request included files to exist
                task.PrerequisiteFiles.AddRange(IncludesCache.FindAllIncludedFiles(sourceFile));

                // Add the source file
                args.Add(string.Format("\"{0}\"", sourceFile));
                task.ProducedFiles.Add(outputFile);

                task.WorkingDirectory = options.WorkingDirectory;
                task.CommandPath = _resourceCompilerPath;
                task.CommandArguments = string.Join(" ", args);
                task.PrerequisiteFiles.Add(sourceFile);
                task.Cost = 1;
            }

            base.LinkFiles(graph, options, outputFilePath);
        }

        private static bool HasDefinition(IEnumerable<string> definitions, string name)
        {
            var prefix = name + "=";
            foreach (var definition in definitions)
            {
                if (definition.StartsWith(prefix, StringComparison.Ordinal))
                    return true;
            }
            return false;
        }

        private static bool TryGetGameProductName(BuildOptions options, out string finalName)
        {
            finalName = null;
            if (options.LinkEnv.Output != LinkerOutput.Executable)
                return false;
            if (!string.Equals(options.Target.ConfigurationName, "Game", StringComparison.OrdinalIgnoreCase))
                return false;

            // Prefer the workspace project (game), fallback to target project.
            ProjectInfo project = Globals.Project;
            if (project == null || string.Equals(project.Name, "Flax", StringComparison.OrdinalIgnoreCase))
            {
                if (options.Target is ProjectTarget projectTarget && projectTarget.Project != null)
                    project = projectTarget.Project;
            }
            if (project == null)
                return false;

            // Default values
            string productName = project.Name;
            string companyName = string.IsNullOrEmpty(project.Company) ? "MyCompany" : project.Company;
            string outputNameTemplate = "${PROJECT_NAME}";

            // Parse GameSettings.json
            var gameSettingsPath = Path.Combine(project.ProjectFolderPath, "Content", "GameSettings.json");
            var jsonProductName = GetJsonValue(gameSettingsPath, "ProductName");
            if (!string.IsNullOrEmpty(jsonProductName))
                productName = jsonProductName;
            var jsonCompanyName = GetJsonValue(gameSettingsPath, "CompanyName");
            if (!string.IsNullOrEmpty(jsonCompanyName))
                companyName = jsonCompanyName;

            // Parse Build Settings.json
            var buildSettingsPath = Path.Combine(project.ProjectFolderPath, "Content", "Settings", "Build Settings.json");
            var jsonOutputName = GetJsonValue(buildSettingsPath, "OutputName");
            if (!string.IsNullOrEmpty(jsonOutputName))
                outputNameTemplate = jsonOutputName;

            if (string.IsNullOrEmpty(outputNameTemplate))
                outputNameTemplate = "FlaxGame";

            // Token replacement matching EditorUtilities.cpp behavior
            string resolvedName = outputNameTemplate
                .Replace("${PROJECT_NAME}", productName, StringComparison.OrdinalIgnoreCase)
                .Replace("${COMPANY_NAME}", companyName, StringComparison.OrdinalIgnoreCase);

            // Strip invalid filename characters
            foreach (char c in Path.GetInvalidFileNameChars())
                resolvedName = resolvedName.Replace(c.ToString(), "");

            if (string.IsNullOrEmpty(resolvedName))
                return false;

            finalName = resolvedName;
            return true;
        }

        private static string GetJsonValue(string path, string propertyName)
        {
            if (!File.Exists(path))
                return null;
            var content = File.ReadAllText(path);

            // Search for "PropertyName": "Value" or "PropertyName":"Value"
            var search = $"\"{propertyName}\"";
            int keyIdx = content.IndexOf(search, StringComparison.Ordinal);
            if (keyIdx == -1)
                return null;

            int colonIdx = content.IndexOf(':', keyIdx + search.Length);
            if (colonIdx == -1)
                return null;

            int quoteStart = content.IndexOf('"', colonIdx + 1);
            if (quoteStart == -1)
                return null;

            int quoteEnd = content.IndexOf('"', quoteStart + 1);
            if (quoteEnd == -1)
                return null;

            return content.Substring(quoteStart + 1, quoteEnd - quoteStart - 1);
        }
    }
}
