// Copyright (c) Wojciech Figat. All rights reserved.

namespace Flax.Build
{
    /// <summary>
    /// The build target that builds the game project modules for standalone game.
    /// </summary>
    /// <seealso cref="Flax.Build.ProjectTarget" />
    public abstract class GameProjectTarget : ProjectTarget
    {
        /// <inheritdoc />
        public override void Init()
        {
            base.Init();

            // Setup target properties
            OutputType = TargetOutputType.Library;
            ConfigurationName = "Game";

            /*// Building game target into single executable
            LinkType = TargetLinkType.Monolithic;
            OutputType = TargetOutputType.Executable;
            Modules.Add("Main");*/
        }
    }

    /// <summary>
    /// The build target that builds the game project modules for editor.
    /// </summary>
    /// <seealso cref="Flax.Build.ProjectTarget" />
    public abstract class GameProjectEditorTarget : ProjectTarget
    {
        /// <inheritdoc />
        public override void Init()
        {
            base.Init();

            // Setup target properties
            IsEditor = true;
            OutputType = TargetOutputType.Library;
            Platforms = new[]
            {
                TargetPlatform.Windows,
                TargetPlatform.Linux,
                TargetPlatform.Mac,
            };
            Architectures = new[]
            {
                TargetArchitecture.x64,
                TargetArchitecture.ARM64,
            };
            ConfigurationName = "Editor";
            GlobalDefinitions.Add("USE_EDITOR");
        }
    }
}
