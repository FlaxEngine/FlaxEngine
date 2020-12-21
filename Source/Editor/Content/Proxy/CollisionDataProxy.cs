// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using System;
using System.Threading.Tasks;
using FlaxEditor.Windows;
using FlaxEditor.Windows.Assets;
using FlaxEngine;

namespace FlaxEditor.Content
{
    /// <summary>
    /// A <see cref="CollisionData"/> asset proxy object.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.BinaryAssetProxy" />
    public class CollisionDataProxy : BinaryAssetProxy
    {
        /// <inheritdoc />
        public override string Name => "Collision Data";

        /// <inheritdoc />
        public override EditorWindow Open(Editor editor, ContentItem item)
        {
            return new CollisionDataWindow(editor, item as AssetItem);
        }

        /// <inheritdoc />
        public override Color AccentColor => Color.FromRGB(0x2c3e50);

        /// <inheritdoc />
        public override Type AssetType => typeof(CollisionData);

        /// <inheritdoc />
        public override bool CanCreate(ContentFolder targetLocation)
        {
            return targetLocation.CanHaveAssets;
        }

        /// <inheritdoc />
        public override void Create(string outputPath, object arg)
        {
            if (Editor.CreateAsset(Editor.NewAssetType.CollisionData, outputPath))
                throw new Exception("Failed to create new asset.");
        }

        /// <summary>
        /// Create collision data from model.
        /// </summary>
        /// <param name="model">The associated model.</param>
        public void CreateCollisionDataFromModel(Model model)
        {
            Action<ContentItem> created = contentItem =>
            {
                var ai = (AssetItem)contentItem;
                var cd = FlaxEngine.Content.LoadAsync<CollisionData>(ai.ID);
                if (cd == null || cd.WaitForLoaded())
                {
                    Editor.LogError("Failed to load created collision data.");
                    return;
                }

                Task.Run(() =>
                {
                    Editor.CookMeshCollision(ai.Path, CollisionDataType.TriangleMesh, model);
                }); 
            };
            Editor.Instance.Windows.ContentWin.NewItem(this, null, created);
        }
    }
}
