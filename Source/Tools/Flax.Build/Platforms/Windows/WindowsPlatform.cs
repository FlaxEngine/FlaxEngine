// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System.IO;
using System.Linq;
using System.Text;
using Flax.Build.Projects;
using Flax.Build.Projects.VisualStudio;

namespace Flax.Build.Platforms
{
    /// <summary>
    /// The Microsoft Windows platform implementation.
    /// </summary>
    /// <seealso cref="Platform" />
    /// <seealso cref="Flax.Build.Platforms.WindowsPlatformBase" />
    public sealed class WindowsPlatform : WindowsPlatformBase, IVisualStudioProjectCustomizer, IProjectCustomizer
    {
        /// <inheritdoc />
        public override TargetPlatform Target => TargetPlatform.Windows;

        /// <summary>
        /// Initializes a new instance of the <see cref="WindowsPlatform"/> class.
        /// </summary>
        public WindowsPlatform()
        {
            if (Platform.BuildTargetPlatform != TargetPlatform.Windows)
                return;

            // Need Windows SDK (any)
            if (GetSDKs().Count == 0)
            {
                Log.Warning("Missing Windows SDK. Cannot build for Windows platform.");
                _hasRequiredSDKsInstalled = false;
                return;
            }

            // Need v140+ toolset
            if (!GetToolsets().ContainsKey(WindowsPlatformToolset.v140) &&
                !GetToolsets().ContainsKey(WindowsPlatformToolset.v141) &&
                !GetToolsets().ContainsKey(WindowsPlatformToolset.v142))
            {
                Log.Warning("Missing MSVC toolset v140 or later (VS 2015 or later C++ build tools). Cannot build for Windows platform.");
                _hasRequiredSDKsInstalled = false;
            }
        }

        /// <inheritdoc />
        protected override Toolchain CreateToolchain(TargetArchitecture architecture)
        {
            return new WindowsToolchain(this, architecture);
        }

        /// <inheritdoc />
        public override bool CanBuildPlatform(TargetPlatform platform)
        {
            switch (platform)
            {
            case TargetPlatform.Windows: return GetSDKs().Count != 0;
            case TargetPlatform.XboxOne:
            case TargetPlatform.UWP: return GetSDKs().FirstOrDefault(x => x.Key != WindowsPlatformSDK.v8_1).Value != null;
            case TargetPlatform.PS4: return Sdk.HasValid("PS4Sdk");
            case TargetPlatform.XboxScarlett: return GetSDKs().ContainsKey(WindowsPlatformSDK.v10_0_19041_0) && Sdk.HasValid("GDK");
            case TargetPlatform.Android: return AndroidSdk.Instance.IsValid && AndroidNdk.Instance.IsValid;
            default: return false;
            }
        }

        /// <inheritdoc />
        void IVisualStudioProjectCustomizer.WriteVisualStudioBegin(VisualStudioProject project, Platform platform, StringBuilder vcProjectFileContent, StringBuilder vcFiltersFileContent, StringBuilder vcUserFileContent)
        {
        }

        /// <inheritdoc />
        void IVisualStudioProjectCustomizer.WriteVisualStudioBuildProperties(VisualStudioProject project, Platform platform, Toolchain toolchain, Project.ConfigurationData configuration, StringBuilder vcProjectFileContent, StringBuilder vcFiltersFileContent, StringBuilder vcUserFileContent)
        {
            // Override the debugger to use Editor if target doesn't output executable file
            var outputType = project.OutputType ?? configuration.Target.OutputType;
            if (outputType != TargetOutputType.Executable && configuration.Name.StartsWith("Editor."))
            {
                var editorFolder = configuration.Architecture == TargetArchitecture.x64 ? "Win64" : "Win32";
                vcUserFileContent.AppendLine(string.Format("  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='{0}'\">", configuration.Name));
                vcUserFileContent.AppendLine(string.Format("    <LocalDebuggerCommand>{0}\\FlaxEditor.exe</LocalDebuggerCommand>", Path.Combine(Globals.EngineRoot, "Binaries", "Editor", editorFolder, configuration.ConfigurationName)));
                vcUserFileContent.AppendLine("    <LocalDebuggerCommandArguments>-project \"$(SolutionDir)\" -skipCompile</LocalDebuggerCommandArguments>");
                vcUserFileContent.AppendLine("    <LocalDebuggerWorkingDirectory>$(SolutionDir)</LocalDebuggerWorkingDirectory>");
                vcUserFileContent.AppendLine("    <DebuggerFlavor>WindowsLocalDebugger</DebuggerFlavor>");
                vcUserFileContent.AppendLine("    </PropertyGroup>");
            }
        }

        /// <inheritdoc />
        void IVisualStudioProjectCustomizer.WriteVisualStudioEnd(VisualStudioProject project, Platform platform, StringBuilder vcProjectFileContent, StringBuilder vcFiltersFileContent, StringBuilder vcUserFileContent)
        {
        }

        /// <inheritdoc />
        void IProjectCustomizer.GetSolutionArchitectureName(TargetArchitecture architecture, ref string name)
        {
            switch (architecture)
            {
            case TargetArchitecture.x86:
                name = "Win32";
                break;
            case TargetArchitecture.x64:
                name = "Win64";
                break;
            }
        }
    }
}
