// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;

namespace FlaxEditor.Content
{
    /// <summary>
    /// Asset item stored in a Json format file.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.AssetItem" />
    public class JsonAssetItem : AssetItem
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="JsonAssetItem"/> class.
        /// </summary>
        /// <param name="path">The path.</param>
        /// <param name="id">The identifier.</param>
        /// <param name="typeName">Name of the resource type.</param>
        public JsonAssetItem(string path, Guid id, string typeName)
        : base(path, typeName, ref id)
        {
        }

        /// <inheritdoc />
        public override ContentItemSearchFilter SearchFilter => ContentItemSearchFilter.Json;

        /// <inheritdoc />
        public override SpriteHandle DefaultThumbnail => Editor.Instance.Icons.Document64;

        /// <inheritdoc />
        protected override bool DrawShadow => false;
    }
}
