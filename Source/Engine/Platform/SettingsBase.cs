// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

namespace FlaxEditor.Content.Settings
{
    /// <summary>
    /// The base class for all the settings assets.
    /// </summary>
    public abstract class SettingsBase
    {
    }

    partial class GraphicsSettings
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="GraphicsSettings"/>.
        /// </summary>
        public GraphicsSettings()
        {
            // Initialize PostFx settings with default options (C# structs don't support it)
            PostProcessSettings = FlaxEngine.PostProcessSettings.Default;
        }
    }
}
