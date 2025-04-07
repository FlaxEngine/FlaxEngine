// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Text;
using FlaxEngine;

namespace FlaxEditor.Content
{
    /// <summary>
    /// Represents binary asset item.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.AssetItem" />
    public class BinaryAssetItem : AssetItem
    {
        /// <summary>
        /// The type of the asset (the same as <see cref="AssetItem.TypeName"/> but cached as type reference).
        /// </summary>
        public readonly Type Type;

        /// <summary>
        /// Initializes a new instance of the <see cref="BinaryAssetItem"/> class.
        /// </summary>
        /// <param name="path">The asset path.</param>
        /// <param name="id">The asset identifier.</param>
        /// <param name="typeName">The asset type name identifier.</param>
        /// <param name="type">The asset type.</param>
        /// <param name="searchFilter">The asset type search filter type.</param>
        public BinaryAssetItem(string path, ref Guid id, string typeName, Type type, ContentItemSearchFilter searchFilter)
        : base(path, typeName, ref id)
        {
            Type = type;
            SearchFilter = searchFilter;
        }

        /// <summary>
        /// Gets the asset import path.
        /// </summary>
        /// <param name="importPath">The import path.</param>
        /// <returns>True if fails, otherwise false.</returns>
        public bool GetImportPath(out string importPath)
        {
            // TODO: add internal call to content backend with fast import asset metadata gather (without asset loading)

            var asset = FlaxEngine.Content.Load<BinaryAsset>(ID, 100);
            if (asset)
            {
                // Get meta from loaded asset
                importPath = asset.ImportPath;
                return string.IsNullOrEmpty(importPath);
            }

            importPath = string.Empty;
            return true;
        }

        internal void OnReimport(ref Guid id)
        {
            ID = id;
            OnReimport();
        }

        /// <inheritdoc />
        public override ContentItemSearchFilter SearchFilter { get; }

        /// <inheritdoc />
        public override bool IsOfType(Type type)
        {
            return type.IsAssignableFrom(Type);
        }
    }

    /// <summary>
    /// Implementation of <see cref="BinaryAssetItem"/> for <see cref="TextureBase"/> assets.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.BinaryAssetItem" />
    public class TextureAssetItem : BinaryAssetItem
    {
        /// <inheritdoc />
        public TextureAssetItem(string path, ref Guid id, string typeName, Type type)
        : base(path, ref id, typeName, type, ContentItemSearchFilter.Texture)
        {
        }

        /// <inheritdoc />
        protected override void OnBuildTooltipText(StringBuilder sb)
        {
            base.OnBuildTooltipText(sb);

            var asset = FlaxEngine.Content.Load<TextureBase>(ID, 100);
            if (asset)
            {
                sb.Append("Format: ").Append(asset.Format).AppendLine();
                sb.Append("Size: ").Append(asset.Width).Append('x').Append(asset.Height);
                if (asset.ArraySize != 1)
                    sb.Append('[').Append(asset.ArraySize).Append(']');
                sb.AppendLine();
                sb.Append("Mip Levels: ").Append(asset.MipLevels).AppendLine();
            }
        }
    }

    /// <summary>
    /// Implementation of <see cref="BinaryAssetItem"/> for <see cref="Model"/> assets.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.BinaryAssetItem" />
    public class ModelItem : BinaryAssetItem
    {
        /// <inheritdoc />
        public ModelItem(string path, ref Guid id, string typeName, Type type)
        : base(path, ref id, typeName, type, ContentItemSearchFilter.Model)
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
            return new StaticModel { Model = FlaxEngine.Content.LoadAsync<Model>(ID) };
        }

        /// <inheritdoc />
        protected override void OnBuildTooltipText(StringBuilder sb)
        {
            base.OnBuildTooltipText(sb);

            var asset = FlaxEngine.Content.Load<Model>(ID, 100);
            if (asset)
            {
                var lods = asset.LODs;
                int triangleCount = 0, vertexCount = 0;
                for (int lodIndex = 0; lodIndex < lods.Length; lodIndex++)
                {
                    var lod = lods[lodIndex];
                    for (int meshIndex = 0; meshIndex < lod.Meshes.Length; meshIndex++)
                    {
                        var mesh = lod.Meshes[meshIndex];
                        triangleCount += mesh.TriangleCount;
                        vertexCount += mesh.VertexCount;
                    }
                }
                sb.Append("LODs: ").Append(lods.Length).AppendLine();
                sb.Append("Triangles: ").Append(triangleCount.ToString("N0")).AppendLine();
                sb.Append("Vertices: ").Append(vertexCount.ToString("N0")).AppendLine();
            }
        }
    }

    /// <summary>
    /// Implementation of <see cref="BinaryAssetItem"/> for <see cref="SkinnedModel"/> assets.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.BinaryAssetItem" />
    public class SkinnedModeItem : BinaryAssetItem
    {
        /// <inheritdoc />
        public SkinnedModeItem(string path, ref Guid id, string typeName, Type type)
        : base(path, ref id, typeName, type, ContentItemSearchFilter.Model)
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
            return new AnimatedModel { SkinnedModel = FlaxEngine.Content.LoadAsync<SkinnedModel>(ID) };
        }

        /// <inheritdoc />
        protected override void OnBuildTooltipText(StringBuilder sb)
        {
            base.OnBuildTooltipText(sb);

            var asset = FlaxEngine.Content.Load<SkinnedModel>(ID, 100);
            if (asset)
            {
                var lods = asset.LODs;
                int triangleCount = 0, vertexCount = 0;
                for (int lodIndex = 0; lodIndex < lods.Length; lodIndex++)
                {
                    var lod = lods[lodIndex];
                    for (int meshIndex = 0; meshIndex < lod.Meshes.Length; meshIndex++)
                    {
                        var mesh = lod.Meshes[meshIndex];
                        triangleCount += mesh.TriangleCount;
                        vertexCount += mesh.VertexCount;
                    }
                }
                sb.Append("LODs: ").Append(lods.Length).AppendLine();
                sb.Append("Triangles: ").Append(triangleCount.ToString("N0")).AppendLine();
                sb.Append("Vertices: ").Append(vertexCount.ToString("N0")).AppendLine();
                sb.Append("Skeleton Nodes: ").Append(asset.Nodes.Length).AppendLine();
                sb.Append("Blend Shapes: ").Append(asset.BlendShapes.Length).AppendLine();
            }
        }
    }
}
