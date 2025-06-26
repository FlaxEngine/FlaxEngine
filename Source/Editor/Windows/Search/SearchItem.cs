// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.Content;
using FlaxEditor.GUI.ContextMenu;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Windows.Search
{
    /// <summary>
    /// The <see cref="ContentFinder"/> item.
    /// </summary>
    [HideInEditor]
    internal class SearchItem : ContainerControl
    {
        private ContentFinder _finder;

        /// <summary>
        /// The item icon.
        /// </summary>
        protected Image _icon;

        /// <summary>
        /// The color of the accent strip.
        /// </summary>
        protected Color _accentColor;

        /// <summary>
        /// The item name.
        /// </summary>
        public string Name;

        /// <summary>
        /// The item type name.
        /// </summary>
        public string Type;

        /// <summary>
        /// The item object reference.
        /// </summary>
        public object Item;

        /// <summary>
        /// Initializes a new instance of the <see cref="SearchItem"/> class.
        /// </summary>
        /// <param name="name">The name.</param>
        /// <param name="type">The type.</param>
        /// <param name="item">The item.</param>
        /// <param name="finder">The finder.</param>
        /// <param name="width">The item width.</param>
        /// <param name="height">The item height.</param>
        public SearchItem(string name, string type, object item, ContentFinder finder, float width, float height)
        {
            Size = new Float2(width, height);
            Name = name;
            Type = type;
            Item = item;
            _finder = finder;

            var logoSize = 15.0f;
            var icon = new Image
            {
                Size = new Float2(logoSize),
                Location = new Float2(7, (height - logoSize) / 2)
            };
            _icon = icon;

            var nameLabel = AddChild<Label>();
            nameLabel.Height = 25;
            nameLabel.Location = new Float2(icon.X + icon.Width + 5, (height - nameLabel.Height) / 2);
            nameLabel.Text = Name;
            nameLabel.HorizontalAlignment = TextAlignment.Near;

            var typeLabel = AddChild<Label>();
            typeLabel.Height = 25;
            typeLabel.Location = new Float2((height - nameLabel.Height) / 2, X + width - typeLabel.Width - 17);
            typeLabel.HorizontalAlignment = TextAlignment.Far;
            typeLabel.Text = Type;
            typeLabel.TextColor = Style.Current.ForegroundGrey;
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            // Select and focus the item on right click to prevent the search from being cleared
            if (button == MouseButton.Right)
            {
                _finder.SelectedItem = this;
                _finder.Hand = true;
                Focus();
                return true;
            }
            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left)
            {
                _finder.Hide();
                Editor.Instance.ContentFinding.Open(Item);
            }

            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override void Draw()
        {
            if (IsMouseOver)
                Render2D.FillRectangle(new Rectangle(Float2.Zero, Size), Style.Current.BackgroundHighlighted);

            base.Draw();
        }

        /// <inheritdoc />
        public override void OnMouseEnter(Float2 location)
        {
            base.OnMouseEnter(location);

            var root = RootWindow;
            if (root != null)
                root.Cursor = CursorType.Hand;
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
    /// The <see cref="SearchItem"/> for assets. Supports using content item thumbnail.
    /// </summary>
    /// <seealso cref="FlaxEditor.Windows.Search.SearchItem" />
    /// <seealso cref="FlaxEditor.Content.IContentItemOwner" />
    internal class ContentSearchItem : SearchItem, IContentItemOwner
    {
        private ContentItem _asset;
        private ContextMenu _cm;

        /// <inheritdoc />
        public ContentSearchItem(string name, string type, ContentItem item, ContentFinder finder, float width, float height)
        : base(name, type, item, finder, width, height)
        {
            _asset = item;
            _asset.AddReference(this);
            _accentColor = Editor.Instance.ContentDatabase.GetProxy(item).AccentColor;
        }

        /// <inheritdoc />
        protected override bool ShowTooltip => true;

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
                            cm.AddButton("Show import location", () => FileSystem.ShowFileExplorer(importLocation));
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
                _cm = cm;
                return true;
            }
            return false;
        }

        /// <inheritdoc />
        public override void Draw()
        {
            // Draw context menu hint
            if (_cm != null && _cm.Visible)
                Render2D.FillRectangle(new Rectangle(Float2.Zero, Size), Color.Gray);

            base.Draw();

            // Draw icon
            var iconRect = _icon.Bounds;
            _asset.DrawThumbnail(ref iconRect);

            // Draw color strip
            var rect = iconRect with { Width = 2, Height = Height, Location = new Float2(2, 0) };
            Render2D.FillRectangle(rect, _accentColor);
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _cm = null;
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
}
