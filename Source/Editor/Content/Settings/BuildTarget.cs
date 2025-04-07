// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;

namespace FlaxEditor.Content.Settings
{
    /// <summary>
    /// A <see cref="GameCooker"/> game building target with configuration properties.
    /// </summary>
    [Serializable, HideInEditor]
    public class BuildTarget
    {
        /// <summary>
        /// The name of the target.
        /// </summary>
        [EditorOrder(10), Tooltip("Name of the target")]
        public string Name;

        /// <summary>
        /// The output folder path.
        /// </summary>
        [EditorOrder(20), Tooltip("Output folder path")]
        public string Output;

        /// <summary>
        /// The target platform.
        /// </summary>
        [EditorOrder(30), Tooltip("Target platform")]
        public BuildPlatform Platform;

        /// <summary>
        /// The configuration mode.
        /// </summary>
        [EditorOrder(30), Tooltip("Configuration build mode")]
        public BuildConfiguration Mode;

        /// <summary>
        /// The list of custom defines passed to the build tool when compiling project scripts. Can be used in build scripts for configuration (Configuration.CustomDefines).
        /// </summary>
        [EditorOrder(90), Tooltip("The list of custom defines passed to the build tool when compiling project scripts. Can be used in build scripts for configuration (Configuration.CustomDefines).")]
        public string[] CustomDefines;

        /// <summary>
        /// The pre-build action command line.
        /// </summary>
        [EditorOrder(100)]
        public string PreBuildAction;

        /// <summary>
        /// The post-build action command line.
        /// </summary>
        [EditorOrder(110)]
        public string PostBuildAction;
    }
}
