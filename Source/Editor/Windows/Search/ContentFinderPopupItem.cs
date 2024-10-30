// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using FlaxEditor.Content;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.SceneGraph;
using FlaxEngine;
using FlaxEngine.GUI;
using System;
using System.Linq;

// TODO: Sort of each one of these classes into their own file or rename the file to something more generic

namespace FlaxEditor.Windows.Search
{
    /// <summary>
    /// The base class for all items displayed in classes inheriting <see cref="SearchableEditorPopup"/>.
    /// </summary>
    [HideInEditor]
    internal class PopupItemBase : ContainerControl
    {
        protected SearchableEditorPopup _finder;
        protected Image _icon;
        protected Label _nameLabel;

        /// <summary>
        /// The item name.
        /// </summary>
        public string Name;

        /// <summary>
        /// The item object reference.
        /// </summary>
        public object Item;

        /// <summary>
        /// The context menu.
        /// </summary>
        public ContextMenu Cm;

        /// <summary>
        /// Initializes a new instance of the <see cref="PopupItemBase"/> class.
        /// </summary>
        /// <param name="name">The name.</param>
        /// <param name="item">The item.</param>
        /// <param name="finder">The finder.</param>
        /// <param name="width">The item width.</param>
        /// <param name="height">The item height.</param> 
        public PopupItemBase(string name, object item, SearchableEditorPopup finder, float width, float height)
        {
            Size = new Float2(width, height);
            Name = name;
            Item = item;
            //Parent = finder;
            _finder = finder;

            var logoSize = 15.0f;
            var icon = new Image
            {
                Size = new Float2(logoSize),
                Location = new Float2(5, (height - logoSize) / 2)
            };
            _icon = icon;

            _nameLabel = AddChild<Label>();
            _nameLabel.Height = 25;
            _nameLabel.Location = new Float2(icon.X + icon.Width + 5, (height - _nameLabel.Height) / 2);
            _nameLabel.Text = Name;
            _nameLabel.HorizontalAlignment = TextAlignment.Near;

            // TODO: Either remove this or figure out what it does (preferably both)

            //var typeLabel = AddChild<Label>();
            //typeLabel.Height = 25;
            //typeLabel.Location = new Float2((height - _nameLabel.Height) / 2, X + width - typeLabel.Width - 17);
            //typeLabel.HorizontalAlignment = TextAlignment.Far;
            ////typeLabel.Text = Type;
            //typeLabel.Text = "ASDASDASDASD";
            //typeLabel.TextColor = Style.Current.ForegroundGrey;
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left)
            {
                _finder.Hide();
                Select();
            }

            return base.OnMouseUp(location, button);
            //return true;
        }

        /// <inheritdoc />
        public override void OnMouseEnter(Float2 location)
        {
            base.OnMouseEnter(location);

            var root = RootWindow;
            if (root != null)
            {
                root.Cursor = CursorType.Hand;
            }

            _finder.SelectedItem = this;
            _finder.Hand = true;
        }

        /// <summary>
        /// Select the current SearchItem.
        /// </summary>
        public virtual void Select()
        {
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            Item = null;
            _finder = null;
            _icon = null;

            base.OnDestroy();
        }
    }

    /// <summary>
    /// The <see cref="PopupItemBase"/> for assets. Supports using content item thumbnail.
    /// </summary>
    /// <seealso cref="FlaxEditor.Windows.Search.PopupItemBase" />
    /// <seealso cref="FlaxEditor.Content.IContentItemOwner" />
    internal class ContentFinderPopupItem : PopupItemBase, IContentItemOwner
    {
        private string Type;

        private ContentItem _asset;

        /// <inheritdoc />
        public ContentFinderPopupItem(string name, string type, ContentItem item, SearchableEditorPopup finder, float width, float height)
        : base(name, item, finder, width, height)
        {
            _asset = item;
            _asset.AddReference(this);
            Type = type;
        }

        /// <inheritdoc />
        protected override bool ShowTooltip => true;

        public override void Select()
        {
            Editor.Instance.ContentFinding.Open(Item);
        }

        /// <inheritdoc />
        public override bool OnShowTooltip(out string text, out Float2 location, out Rectangle area)
        {
            if (string.IsNullOrEmpty(TooltipText) && Item is ContentItem contentItem)
            {
                contentItem.UpdateTooltipText();
                TooltipText = contentItem.TooltipText;
            }
            return base.OnShowTooltip(out text, out location, out area);
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (base.OnMouseDown(location, button))
                return true;
            if (button == MouseButton.Right && Item is ContentItem)
                return true;
            return false;
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (base.OnMouseUp(location, button))
                return true;
            if (button == MouseButton.Right && Item is ContentItem contentItem)
            {
                // Show context menu
                var proxy = Editor.Instance.ContentDatabase.GetProxy(contentItem);
                ContextMenuButton b;
                var cm = new ContextMenu { Tag = contentItem };
                b = cm.AddButton("Open", () => Editor.Instance.ContentFinding.Open(Item));
                cm.AddSeparator();
                cm.AddButton(Utilities.Constants.ShowInExplorer, () => FileSystem.ShowFileExplorer(System.IO.Path.GetDirectoryName(contentItem.Path)));
                cm.AddButton("Show in Content window", () => Editor.Instance.Windows.ContentWin.Select(contentItem, true));
                b.Enabled = proxy != null && proxy.CanReimport(contentItem);
                if (contentItem is BinaryAssetItem binaryAsset)
                {
                    if (!binaryAsset.GetImportPath(out string importPath))
                    {
                        string importLocation = System.IO.Path.GetDirectoryName(importPath);
                        if (!string.IsNullOrEmpty(importLocation) && System.IO.Directory.Exists(importLocation))
                        {
                            cm.AddButton("Show import location", () => FileSystem.ShowFileExplorer(importLocation));
                        }
                    }
                }
                cm.AddSeparator();
                if (contentItem is AssetItem assetItem)
                {
                    cm.AddButton("Copy asset ID", () => Clipboard.Text = FlaxEngine.Json.JsonSerializer.GetStringID(assetItem.ID));
                    cm.AddButton("Select actors using this asset", () => Editor.Instance.SceneEditing.SelectActorsUsingAsset(assetItem.ID));
                    cm.AddButton("Show asset references graph", () => Editor.Instance.Windows.Open(new AssetReferencesGraphWindow(Editor.Instance, assetItem)));
                    cm.AddButton("Copy name to Clipboard", () => Clipboard.Text = assetItem.NamePath);
                    cm.AddButton("Copy path to Clipboard", () => Clipboard.Text = assetItem.Path);
                    cm.AddSeparator();
                }
                proxy?.OnContentWindowContextMenu(cm, contentItem);
                contentItem.OnContextMenu(cm);
                cm.Show(this, location);
                Cm = cm;

                _finder.ResultPanel.VScrollBar.ValueChanged += () =>
                {
                    if (Cm != null)
                        Cm.Hide();
                };

                return true;
            }
            return false;
        }

        /// <inheritdoc />
        public override void Draw()
        {
            // Draw context menu hint
            if (Cm != null && Cm.Visible)
                Render2D.FillRectangle(new Rectangle(Float2.Zero, Size), Color.Gray);

            base.Draw();

            // Draw icon
            var iconRect = _icon.Bounds;
            _asset.DrawThumbnail(ref iconRect);
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            Cm = null;
            if (_asset != null)
            {
                _asset.RemoveReference(this);
                _asset = null;
            }

            base.OnDestroy();
        }

        /// <inheritdoc />
        public void OnItemDeleted(ContentItem item)
        {
            Dispose();
        }

        /// <inheritdoc />
        public void OnItemRenamed(ContentItem item)
        {
        }

        /// <inheritdoc />
        public void OnItemReimported(ContentItem item)
        {
        }

        /// <inheritdoc />
        public void OnItemDispose(ContentItem item)
        {
            Dispose();
        }
    }

    /// <summary>
    /// Item displayed in the results list of the <see cref="ActorAdder"/>.
    /// </summary>
    internal class ActorAdderPopupItem : PopupItemBase
    {
        public Type ActorType;

        /// <inheritdoc />
        public ActorAdderPopupItem(string name, object item, Type type, SearchableEditorPopup finder, float width, float height)
        : base(name, item, finder, width, height)
        {
            ActorType = type;
        }

        /// <inheritdoc />
        protected override bool ShowTooltip => true;

        public override void Select()
        {
            Actor selectedSceneActor = null;

            if (Editor.Instance.SceneEditing.Selection.Any(s => s is ActorNode))
                selectedSceneActor = ((ActorNode)Editor.Instance.SceneEditing.Selection.First(s => s is ActorNode)).Actor;

            AddActorToScene(ActorType, selectedSceneActor);
        }

        // Helper function to make context menu "Add to scene root" easier to implement
        private void AddActorToScene(Type actorType, Actor parent = null)
        {
            Actor newActor = (Actor)FlaxEngine.Object.New(ActorType);
            Editor.Instance.SceneEditing.Spawn(newActor, parent, false);

            Editor.Instance.SceneEditing.Select(newActor);
            Editor.Instance.Scene.GetActorNode(newActor).TreeNode.StartRenaming(Editor.Instance.Windows.SceneWin, Editor.Instance.Windows.SceneWin.SceneTreePanel);
        }

        /// <inheritdoc />
        public override bool OnShowTooltip(out string text, out Float2 location, out Rectangle area)
        {
            if (TooltipText == null)
                TooltipText = Editor.Instance.CodeDocs.GetTooltip(ActorType);

            return base.OnShowTooltip(out text, out location, out area);
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (base.OnMouseDown(location, button))
                return true;
            if (button == MouseButton.Right)// && Item is ContentItem)
                return true;

            return false;
        }

        // /*
        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (base.OnMouseUp(location, button))
                return true;
            if (button == MouseButton.Right)
            {
                // Show context menu
                var cm = new ContextMenu { Tag = _finder.SelectedItem };
                ContextMenuButton b = cm.AddButton("Add to scene");
                b.Clicked += () => Select();
                b.TooltipText = "Add the actor to the scene root or to the current selection if there is any.\nSame as pressing the Enter key.";
                cm.AddButton("Add to scene root").Clicked += () => AddActorToScene(ActorType);
                cm.AddSeparator();
                b = cm.AddButton("Convert selected actor");
                b.Enabled = Editor.Instance.SceneEditing.SelectionCount > 0;
                b.Clicked += () => Editor.Instance.SceneEditing.Convert(ActorType);
                b = cm.AddButton("Create parent for selected actors");
                b.Enabled = Editor.Instance.SceneEditing.SelectionCount > 0;
                b.Clicked += () => Editor.Instance.SceneEditing.CreateParentForSelectedActors(ActorType);
                cm.AddSeparator();
                cm.AddButton("Create prefab with actor as root").Clicked += () => Editor.Instance.Prefabs.CreatePrefab((Actor)FlaxEngine.Object.New(ActorType));
                cm.Show(this, location);
                Cm = cm;

                _finder.ResultPanel.VScrollBar.ValueChanged += () =>
                {
                    if (Cm != null)
                        Cm.Hide();
                };

                return true;
            }
            return false;
        }
        // */

        /// <inheritdoc />
        public override void Draw()
        {
            // Draw context menu hint
            if (Cm != null && Cm.Visible)
                Render2D.FillRectangle(new Rectangle(Float2.Zero, Size), Color.Gray);

            base.Draw();
        }
    }
}
