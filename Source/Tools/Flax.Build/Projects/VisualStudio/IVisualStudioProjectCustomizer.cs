// Copyright (c) Wojciech Figat. All rights reserved.

using System.Text;

namespace Flax.Build.Projects.VisualStudio
{
    /// <summary>
    /// Interface for objects that can customize generated Visual Studio project files (eg. insert debugger customization or custom extension settings).
    /// </summary>
    public interface IVisualStudioProjectCustomizer
    {
        /// <summary>
        /// Writes the Visual Studio project begin custom data.
        /// </summary>
        /// <param name="project">The project.</param>
        /// <param name="platform">The platform.</param>
        /// <param name="vcProjectFileContent">Contents of the VC project file.</param>
        /// <param name="vcFiltersFileContent">Contents of the VC filters file.</param>
        /// <param name="vcUserFileContent">Contents of the VC user file.</param>
        void WriteVisualStudioBegin(VisualStudioProject project, Platform platform, StringBuilder vcProjectFileContent, StringBuilder vcFiltersFileContent, StringBuilder vcUserFileContent);

        /// <summary>
        /// Writes the Visual Studio project build configuration properties.
        /// </summary>
        /// <param name="project">The project.</param>
        /// <param name="platform">The platform.</param>
        /// <param name="toolchain">The toolchain.</param>
        /// <param name="configuration">The project configuration.</param>
        /// <param name="vcProjectFileContent">Content of the VC project file.</param>
        /// <param name="vcFiltersFileContent">Content of the VC filters file.</param>
        /// <param name="vcUserFileContent">Contents of the VC user file.</param>
        void WriteVisualStudioBuildProperties(VisualStudioProject project, Platform platform, Toolchain toolchain, Project.ConfigurationData configuration, StringBuilder vcProjectFileContent, StringBuilder vcFiltersFileContent, StringBuilder vcUserFileContent);

        /// <summary>
        /// Writes the Visual Studio project end custom data.
        /// </summary>
        /// <param name="project">The project.</param>
        /// <param name="platform">The platform.</param>
        /// <param name="vcProjectFileContent">Contents of the VC project file.</param>
        /// <param name="vcFiltersFileContent">Contents of the VC filters file.</param>
        /// <param name="vcUserFileContent">Contents of the VC user file.</param>
        void WriteVisualStudioEnd(VisualStudioProject project, Platform platform, StringBuilder vcProjectFileContent, StringBuilder vcFiltersFileContent, StringBuilder vcUserFileContent);
    }
}
