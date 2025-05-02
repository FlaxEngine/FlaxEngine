// Copyright (c) Wojciech Figat. All rights reserved.

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
        /// Asset icon.
        /// </summary>
        protected SpriteHandle _thumbnail;

        /// <summary>
        /// Initializes a new instance of the <see cref="JsonAssetItem"/> class.
        /// </summary>
        /// <param name="path">The path.</param>
        /// <param name="id">The identifier.</param>
        /// <param name="typeName">Name of the resource type.</param>
        public JsonAssetItem(string path, Guid id, string typeName)
        : base(path, typeName, ref id)
        {
            _thumbnail = Editor.Instance.Icons.Json128;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="JsonAssetItem"/> class.
        /// </summary>
        /// <param name="path">The path.</param>
        /// <param name="id">The identifier.</param>
        /// <param name="typeName">Name of the resource type.</param>
        /// <param name="thumbnail">Asset icon.</param>
        public JsonAssetItem(string path, Guid id, string typeName, SpriteHandle thumbnail)
        : base(path, typeName, ref id)
        {
            _thumbnail = thumbnail;
        }

        /// <inheritdoc />
        public override ContentItemSearchFilter SearchFilter => ContentItemSearchFilter.Json;

        /// <inheritdoc />
        public override SpriteHandle DefaultThumbnail => _thumbnail;

        /// <inheritdoc />
        protected override bool DrawShadow => false;
    }
}
