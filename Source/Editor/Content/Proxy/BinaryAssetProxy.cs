// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;
using FlaxEngine.Utilities;

namespace FlaxEditor.Content
{
    /// <summary>
    /// Base class for all binary asset proxy objects used to manage <see cref="BinaryAssetItem"/>.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.AssetProxy" />
    public abstract class BinaryAssetProxy : AssetProxy
    {
        /// <summary>
        /// The binary asset files extension.
        /// </summary>
        public static readonly string Extension = "flax";

        /// <inheritdoc />
        public override bool IsProxyFor(ContentItem item)
        {
            return item is BinaryAssetItem binaryAssetItem && TypeName == binaryAssetItem.TypeName;
        }

        /// <inheritdoc />
        public override string FileExtension => Extension;

        /// <inheritdoc />
        public override string TypeName => AssetType.FullName;

        /// <inheritdoc />
        public override bool IsProxyFor<T>()
        {
            return typeof(T) == AssetType;
        }

        /// <summary>
        /// Gets the type of the asset.
        /// </summary>
        public abstract Type AssetType { get; }

        /// <inheritdoc />
        public override AssetItem ConstructItem(string path, string typeName, ref Guid id)
        {
            var type = TypeUtils.GetType(typeName).Type;

            if (typeof(TextureBase).IsAssignableFrom(type))
                return new TextureAssetItem(path, ref id, typeName, type);
            if (typeof(Model).IsAssignableFrom(type))
                return new ModelItem(path, ref id, typeName, type);
            if (typeof(SkinnedModel).IsAssignableFrom(type))
                return new SkinnedModeItem(path, ref id, typeName, type);

            ContentItemSearchFilter searchFilter;
            if (typeof(MaterialBase).IsAssignableFrom(type))
                searchFilter = ContentItemSearchFilter.Material;
            else if (typeof(Prefab).IsAssignableFrom(type))
                searchFilter = ContentItemSearchFilter.Prefab;
            else if (typeof(SceneAsset).IsAssignableFrom(type))
                searchFilter = ContentItemSearchFilter.Scene;
            else if (typeof(Animation).IsAssignableFrom(type))
                searchFilter = ContentItemSearchFilter.Animation;
            else if (typeof(ParticleEmitter).IsAssignableFrom(type))
                searchFilter = ContentItemSearchFilter.Particles;
            else
                searchFilter = ContentItemSearchFilter.Other;
            return new BinaryAssetItem(path, ref id, typeName, type, searchFilter);
        }
    }
}
