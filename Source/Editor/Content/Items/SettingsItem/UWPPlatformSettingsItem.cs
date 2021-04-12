// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;

namespace FlaxEditor.Content
{
    /// <summary>
    /// Content item that contains <see cref="FlaxEditor.Content.Settings.UWPPlatformSettings"/> data.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.JsonAssetItem" />
    public sealed class UWPPlatformSettingsItem : JsonAssetItem
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="UWPPlatformSettingsItem"/> class.
        /// </summary>
        /// <param name="path">The asset path.</param>
        /// <param name="id">The asset identifier.</param>
        /// <param name="typeName">Name of the resource type.</param>
        public UWPPlatformSettingsItem(string path, Guid id, string typeName)
        : base(path, id, typeName)
        {
        }

        /// <inheritdoc />
        public override SpriteHandle DefaultThumbnail => Editor.Instance.Icons.UWPSettings;
    }
}
