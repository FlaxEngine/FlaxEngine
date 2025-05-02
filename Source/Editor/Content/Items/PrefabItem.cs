// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;

namespace FlaxEditor.Content
{
    /// <summary>
    /// Content item that contains <see cref="FlaxEngine.Prefab"/> data.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.JsonAssetItem" />
    public sealed class PrefabItem : JsonAssetItem
    {
        private string _cachedTypeDescription = null;

        /// <summary>
        /// Initializes a new instance of the <see cref="PrefabItem"/> class.
        /// </summary>
        /// <param name="path">The asset path.</param>
        /// <param name="id">The asset identifier.</param>
        public PrefabItem(string path, Guid id)
        : base(path, id, PrefabProxy.AssetTypename)
        {
        }

        /// <inheritdoc />
        public override bool OnEditorDrag(object context)
        {
            return true;
        }

        /// <inheritdoc />
        public override Actor OnEditorDrop(object context)
        {
            return PrefabManager.SpawnPrefab(FlaxEngine.Content.LoadAsync<Prefab>(ID), null);
        }

        /// <inheritdoc />
        public override ContentItemType ItemType => ContentItemType.Asset;

        /// <inheritdoc />
        public override ContentItemSearchFilter SearchFilter => ContentItemSearchFilter.Prefab;

        /// <inheritdoc />
        public override SpriteHandle DefaultThumbnail => SpriteHandle.Invalid;

        /// <inheritdoc />
        public override string TypeDescription
        {
            get
            {
                if (_cachedTypeDescription == null)
                {
                    _cachedTypeDescription = "Prefab";
                    var prefab = FlaxEngine.Content.Load<Prefab>(ID);
                    if (prefab)
                    {
                        Actor root = prefab.GetDefaultInstance();
                        if (root is UIControl or UICanvas)
                            _cachedTypeDescription = "Widget";
                    }
                }
                return _cachedTypeDescription;
            }
        }

        /// <inheritdoc />
        public override bool IsOfType(Type type)
        {
            return type.IsAssignableFrom(typeof(Prefab));
        }
    }
}
