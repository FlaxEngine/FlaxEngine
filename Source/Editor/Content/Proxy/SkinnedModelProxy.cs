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
    /// A <see cref="SkinnedModel"/> asset proxy object.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.BinaryAssetProxy" />
    public class SkinnedModelProxy : BinaryAssetProxy
    {
        private AnimatedModelPreview _preview;

        /// <inheritdoc />
        public override string Name => "Skinned Model";

        /// <inheritdoc />
        public override bool CanReimport(ContentItem item)
        {
            return true;
        }

        /// <inheritdoc />
        public override EditorWindow Open(Editor editor, ContentItem item)
        {
            return new SkinnedModelWindow(editor, item as AssetItem);
        }

        /// <inheritdoc />
        public override Color AccentColor => Color.FromRGB(0xB30031);

        /// <inheritdoc />
        public override Type AssetType => typeof(SkinnedModel);

        /// <inheritdoc />
        public override void OnContentWindowContextMenu(ContextMenu menu, ContentItem item)
        {
            base.OnContentWindowContextMenu(menu, item);

            if (item is BinaryAssetItem binaryAssetItem)
            {
                var button = menu.AddButton("Create Animation Graph", CreateAnimationGraphClicked);
                button.Tag = binaryAssetItem;
            }
        }

        private void CreateAnimationGraphClicked(ContextMenuButton obj)
        {
            var binaryAssetItem = (BinaryAssetItem)obj.Tag;
            CreateAnimationGraph(binaryAssetItem);
        }

        /// <summary>
        /// Creates the animation graph from the given particle emitter.
        /// </summary>
        /// <param name="skinnedModelItem">The skinned model item to use as the base model for the animation graph.</param>
        public static void CreateAnimationGraph(BinaryAssetItem skinnedModelItem)
        {
            var animationGraphName = skinnedModelItem.ShortName + " Graph";
            var animationGraphProxy = Editor.Instance.ContentDatabase.GetProxy<AnimationGraph>();
            Editor.Instance.Windows.ContentWin.NewItem(animationGraphProxy, null, item => OnAnimationGraphCreated(item, skinnedModelItem), animationGraphName);
        }

        private static void OnAnimationGraphCreated(ContentItem item, BinaryAssetItem skinnedModelItem)
        {
            var skinnedModel = FlaxEngine.Content.Load<SkinnedModel>(skinnedModelItem.ID);
            if (skinnedModel == null)
            {
                Editor.LogError("Failed to load base skinned model.");
                return;
            }

            // Hack the animation graph window to modify the base model of the animation graph.
            // TODO: implement it without window logic (load AnimGraphSurface and set AnimationGraphWindow.BaseModelId to model)
            AnimationGraphWindow win = new AnimationGraphWindow(Editor.Instance, item as AssetItem);
            win.Show();

            // Ensure the window knows the asset is loaded so we can save it later.
            win.Asset.WaitForLoaded();
            win.Update(0); // Call Update() to refresh the loaded flag.

            win.SetBaseModel(skinnedModel);
            win.Surface.MarkAsEdited();
            win.Save();
            win.Close();
        }

        /// <inheritdoc />
        public override void OnThumbnailDrawPrepare(ThumbnailRequest request)
        {
            if (_preview == null)
            {
                _preview = new AnimatedModelPreview(false);
                InitAssetPreview(_preview);
            }

            // TODO: disable streaming for asset during thumbnail rendering (and restore it after)
        }

        /// <inheritdoc />
        public override bool CanDrawThumbnail(ThumbnailRequest request)
        {
            return _preview.HasLoadedAssets && ThumbnailsModule.HasMinimumQuality((SkinnedModel)request.Asset);
        }

        /// <inheritdoc />
        public override void OnThumbnailDrawBegin(ThumbnailRequest request, ContainerControl guiRoot, GPUContext context)
        {
            _preview.SkinnedModel = (SkinnedModel)request.Asset;
            _preview.Parent = guiRoot;
            _preview.SyncBackbufferSize();

            _preview.Task.OnDraw();
        }

        /// <inheritdoc />
        public override void OnThumbnailDrawEnd(ThumbnailRequest request, ContainerControl guiRoot)
        {
            _preview.SkinnedModel = null;
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
