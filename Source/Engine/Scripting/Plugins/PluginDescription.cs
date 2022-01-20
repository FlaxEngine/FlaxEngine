// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    /// <summary>
    /// The engine plugin description container.
    /// </summary>
    public struct PluginDescription
    {
        /// <summary>
        /// The name of the plugin.
        /// </summary>
        public string Name;

        /// <summary>
        /// The version of the plugin.
        /// </summary>
        public Version Version;

        /// <summary>
        /// The name of the author of the plugin.
        /// </summary>
        public string Author;

        /// <summary>
        /// The plugin author website URL.
        /// </summary>
        public string AuthorUrl;

        /// <summary>
        /// The homepage URL for the plugin.
        /// </summary>
        public string HomepageUrl;

        /// <summary>
        /// The plugin repository URL (for open-source plugins).
        /// </summary>
        public string RepositoryUrl;

        /// <summary>
        /// The plugin description.
        /// </summary>
        public string Description;

        /// <summary>
        /// The plugin category.
        /// </summary>
        public string Category;

        /// <summary>
        /// True if plugin is during Beta tests (before release).
        /// </summary>
        public bool IsBeta;

        /// <summary>
        /// True if plugin is during Alpha tests (early version).
        /// </summary>
        public bool IsAlpha;
    }
}
