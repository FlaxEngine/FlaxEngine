// Copyright (c) Wojciech Figat. All rights reserved.

namespace Flax.Build.Projects
{
    /// <summary>
    /// The project files format.
    /// </summary>
    public enum ProjectFormat
    {
        /// <summary>
        /// The custom project format (external).
        /// </summary>
        Custom,

        /// <summary>
        /// Visual Studio (auto select the newest version installed on the system).
        /// </summary>
        VisualStudio,

        /// <summary>
        /// Visual Studio 2015.
        /// </summary>
        VisualStudio2015,

        /// <summary>
        /// Visual Studio 2017.
        /// </summary>
        VisualStudio2017,

        /// <summary>
        /// Visual Studio 2019.
        /// </summary>
        VisualStudio2019,

        /// <summary>
        /// Visual Studio 2022.
        /// </summary>
        VisualStudio2022,

        /// <summary>
        /// Visual Studio Code.
        /// </summary>
        VisualStudioCode,

        /// <summary>
        /// XCode.
        /// </summary>
        XCode,
    }
}
