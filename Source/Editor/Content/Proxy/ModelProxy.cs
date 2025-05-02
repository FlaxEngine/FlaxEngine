// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Content.Thumbnails;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.Viewport.Previews;
using FlaxEditor.Windows;
using FlaxEditor.Windows.Assets;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Content
{
    /// <summary>
    /// A <see cref="Model"/> asset proxy object.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.BinaryAssetProxy" />
    public class ModelProxy : BinaryAssetProxy
    {
        private ModelPreview _preview;

        /// <inheritdoc />
        public override string Name => "Model";

        /// <inheritdoc />
        public override bool CanReimport(ContentItem item)
        {
            return true;
        }

        /// <inheritdoc />
        public override EditorWindow Open(Editor editor, ContentItem item)
        {
            return new ModelWindow(editor, item as AssetItem);
        }

        /// <inheritdoc />
        public override Color AccentColor => Color.FromRGB(0xe67e22);

        /// <inheritdoc />
        public override Type AssetType => typeof(Model);

        /// <inheritdoc />
        public override void OnContentWindowContextMenu(ContextMenu menu, ContentItem item)
        {
            base.OnContentWindowContextMenu(menu, item);

            menu.AddButton("Create collision data", () =>
            {
                var collisionDataProxy = (CollisionDataProxy)Editor.Instance.ContentDatabase.GetProxy<CollisionData>();
                var selection = Editor.Instance.Windows.ContentWin.View.Selection;
                if (selection.Count > 1)
                {
                    // Batch action
                    var items = selection.ToArray(); // Clone to prevent issue when iterating over and content window changes the selection
                    foreach (var contentItem in items)
                    {
                        if (contentItem is ModelItem modelItem)
                            collisionDataProxy.CreateCollisionDataFromModel(FlaxEngine.Content.LoadAsync<Model>(modelItem.ID), null, false);
                    }
                }
                else
                {
                    var model = FlaxEngine.Content.LoadAsync<Model>(((ModelItem)item).ID);
                    collisionDataProxy.CreateCollisionDataFromModel(model);
                }
            });
        }

        /// <inheritdoc />
        public override void OnThumbnailDrawPrepare(ThumbnailRequest request)
        {
            if (_preview == null)
            {
                _preview = new ModelPreview(false)
                {
                    ScaleToFit = false,
                };
                InitAssetPreview(_preview);
            }

            // TODO: disable streaming for asset during thumbnail rendering (and restore it after)
        }

        /// <inheritdoc />
        public override bool CanDrawThumbnail(ThumbnailRequest request)
        {
            return _preview.HasLoadedAssets && ThumbnailsModule.HasMinimumQuality((Model)request.Asset);
        }

        /// <inheritdoc />
        public override void OnThumbnailDrawBegin(ThumbnailRequest request, ContainerControl guiRoot, GPUContext context)
        {
            _preview.Model = (Model)request.Asset;
            _preview.Parent = guiRoot;
            _preview.SyncBackbufferSize();
            var bounds = _preview.Model.GetBox();
            var maxSize = Math.Max(0.001f, (float)bounds.Size.MaxValue);
            _preview.ViewportCamera.SetArcBallView(bounds);
            _preview.FarPlane = Mathf.Max(1000.0f, maxSize * 2 + 100.0f);

            _preview.Task.OnDraw();
        }

        /// <inheritdoc />
        public override void OnThumbnailDrawEnd(ThumbnailRequest request, ContainerControl guiRoot)
        {
            _preview.Model = null;
            _preview.Parent = null;
        }

        /// <inheritdoc />
        public override void Dispose()
        {
            if (_preview != null)
            {
                _preview.Dispose();
                _preview = null;
            }

            base.Dispose();
        }
    }
}
