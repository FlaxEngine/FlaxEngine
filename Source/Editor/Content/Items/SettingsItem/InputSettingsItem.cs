// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;

namespace FlaxEditor.Content
{
    /// <summary>
    /// Content item that contains <see cref="FlaxEditor.Content.Settings.InputSettings"/> data.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.JsonAssetItem" />
    public sealed class InputSettingsItem : JsonAssetItem
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="InputSettingsItem"/> class.
        /// </summary>
        /// <param name="path">The asset path.</param>
        /// <param name="id">The asset identifier.</param>
        /// <param name="typeName"></param>
        public InputSettingsItem(string path, Guid id, string typeName)
        : base(path, id, typeName)
        {
        }

        /// <inheritdoc />
        public override SpriteHandle DefaultThumbnail => Editor.Instance.Icons.InputSettings;
    }
}
