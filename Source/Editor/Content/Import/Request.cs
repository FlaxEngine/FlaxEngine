// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.Content.Import
{
    /// <summary>
    /// The content item import request data container.
    /// </summary>
    [HideInEditor]
    public struct Request
    {
        /// <summary>
        /// The input item path (folder or file).
        /// </summary>
        public string InputPath;

        /// <summary>
        /// The output path (folder or file).
        /// </summary>
        public string OutputPath;

        /// <summary>
        /// Flag set to true for the assets handled by the engine internally.
        /// </summary>
        public bool IsInBuilt;

        /// <summary>
        /// Flag used to skip showing import settings dialog to used. Can be used for importing assets from code by plugins.
        /// </summary>
        public bool SkipSettingsDialog;

        /// <summary>
        /// The custom settings object.
        /// </summary>
        public object Settings;
    }
}
