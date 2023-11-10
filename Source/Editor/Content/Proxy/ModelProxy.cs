// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

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
            var createPrefabButton = menu.AddButton("Create Prefab");
            createPrefabButton.CloseMenuOnClick = false;
            createPrefabButton.Clicked += () => OnCreatePrefab(createPrefabButton, item);
        }

        private void OnCreatePrefab(ContextMenuButton button, ContentItem item)
        {
            var popup = new ContextMenuBase
            {
                Size = new Float2(170, 100),
                ClipChildren = false,
                CullChildren = false,
            };
            popup.Show(button, new Float2(button.Width, 0));

            var centerModelsLabel = new Label
            {
                Parent = popup,
                AnchorPreset = AnchorPresets.TopLeft,
                Text = "Center Models",
                HorizontalAlignment = TextAlignment.Near,
            };
            centerModelsLabel.LocalX += 10;
            centerModelsLabel.LocalY += 10;

            var centerModelsCb = new CheckBox()
            {
                Parent = popup,
                AnchorPreset = AnchorPresets.TopLeft,
            };
            centerModelsCb.LocalX += 145;
            centerModelsCb.LocalY += 10;

            var origPosLabel = new Label
            {
                Parent = popup,
                AnchorPreset = AnchorPresets.TopLeft,
                Text = "Use original position",
                HorizontalAlignment = TextAlignment.Near,
            };
            origPosLabel.LocalX += 10;
            origPosLabel.LocalY += 35;

            var origPosCb = new CheckBox
            {
                Parent = popup,
                AnchorPreset = AnchorPresets.TopLeft,
            };
            origPosCb.LocalY += 35;
            origPosCb.LocalX += 145;

            var submitButton = new Button
            {
                Parent = popup,
                AnchorPreset = AnchorPresets.TopLeft,
                Text = "Create",
                Width = 70,
            };
            submitButton.LocalX += 10;
            submitButton.LocalY += 65;
            submitButton.Clicked += () =>
            {
                CreatePrefab(item, centerModelsCb.Checked, origPosCb.Checked);
                centerModelsCb.Checked = false;
                origPosCb.Checked = false;
                popup.Hide();
                button.ParentContextMenu.Hide();
            };

            var cancelButton = new Button
            {
                Parent = popup,
                AnchorPreset = AnchorPresets.TopLeft,
                Text = "Cancel",
                Width = 70,
            };
            cancelButton.LocalX += 90;
            cancelButton.LocalY += 65;
            cancelButton.Clicked += () =>
            {
                centerModelsCb.Checked = false;
                origPosCb.Checked = false;
                popup.Hide();
                button.ParentContextMenu.Hide();
            };
        }

        private void CreatePrefab(ContentItem item, bool centerModels, bool useOrigPos)
        {
            var selection = Editor.Instance.Windows.ContentWin.View.Selection;
            if (selection.Count > 1)
            {
                var items = selection.ToArray();
                var parentActor = new EmptyActor();
                parentActor.Name = "Root";
                Vector3 addedPositions = Vector3.Zero;
                int positionsCount = 0;
                foreach (var contentItem in items)
                {
                    if (contentItem is not ModelItem modelItem)
                        continue;
                    var model = FlaxEngine.Content.LoadAsync<Model>(modelItem.ID);
                    model.WaitForLoaded();
                    var staticModel = parentActor.AddChild<StaticModel>();
                    staticModel.Name = modelItem.ShortName;
                    staticModel.Model = model;
                    if (useOrigPos)
                    {
                        var transform = model.OriginalTransform;
                        staticModel.Position = transform.Translation;
                    }
                    addedPositions += staticModel.Position;
                    positionsCount += 1;
                }
                if (centerModels)
                {
                    Vector3 avgPosition = addedPositions / positionsCount;
                    foreach (var child in parentActor.Children)
                        child.Position -= avgPosition;
                }
                Editor.Instance.Prefabs.CreatePrefab(parentActor);
            }
            else
            {
                var model = FlaxEngine.Content.LoadAsync<Model>(((ModelItem)item).ID);
                model.WaitForLoaded();
                var staticModel = new StaticModel
                {
                    Name = item.ShortName,
                    Model = model
                };
                if (useOrigPos)
                {
                    var transform = model.OriginalTransform;
                    staticModel.Position = transform.Translation;
                }
                if (centerModels)
                    staticModel.Position -= staticModel.Box.Center;
                Editor.Instance.Prefabs.CreatePrefab(staticModel);
            }
        }

        /// <inheritdoc />
        public override void OnThumbnailDrawPrepare(ThumbnailRequest request)
        {
            if (_preview == null)
            {
                _preview = new ModelPreview(false);
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
