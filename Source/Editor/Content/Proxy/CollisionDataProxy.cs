// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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
        /// <param name="created">The action to call once the collision data gets created (or reused from existing).</param>
        public void CreateCollisionDataFromModel(Model model, Action<CollisionData> created = null)
        {
            // Check if there already is collision data for that model to reuse
            var modelItem = (AssetItem)Editor.Instance.ContentDatabase.Find(model.ID);
            if (modelItem?.ParentFolder != null)
            {
                foreach (var child in modelItem.ParentFolder.Children)
                {
                    if (child is BinaryAssetItem b && b.IsOfType<CollisionData>())
                    {
                        var collisionData = FlaxEngine.Content.Load<CollisionData>(b.ID);
                        if (collisionData && collisionData.Options.Model == model.ID)
                        {
                            Editor.Instance.Windows.ContentWin.Select(b);
                            if (created != null)
                                FlaxEngine.Scripting.InvokeOnUpdate(() => created(collisionData));
                            return;
                        }
                    }
                }
            }

            // Create new item so user can name it and then generate collision for it in async
            Action<ContentItem> create = contentItem =>
            {
                var assetItem = (AssetItem)contentItem;
                var collisionData = FlaxEngine.Content.LoadAsync<CollisionData>(assetItem.ID);
                if (collisionData == null || collisionData.WaitForLoaded())
                {
                    Editor.LogError("Failed to load created collision data.");
                    return;
                }
                Task.Run(() =>
                {
                    Editor.CookMeshCollision(assetItem.Path, CollisionDataType.TriangleMesh, model);
                    if (created != null)
                        FlaxEngine.Scripting.InvokeOnUpdate(() => created(collisionData));
                });
            };
            Editor.Instance.Windows.ContentWin.NewItem(this, null, create);
        }
    }
}
