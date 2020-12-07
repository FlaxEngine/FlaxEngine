// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using FlaxEditor.Content;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Surface.ContextMenu
{
    /// <summary>
    /// The <see cref="ContentFinder"/> item.
    /// </summary>
    [HideInEditor]
    public class SearchItem : ContainerControl
    {
        private ContentFinder _finder;

        /// <summary>
        /// The item icon.
        /// </summary>
        protected Image _icon;

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
            Size = new Vector2(width, height);
            Name = name;
            Type = type;
            Item = item;
            _finder = finder;

            var logoSize = 15.0f;
            var icon = new Image
            {
                Size = new Vector2(logoSize),
                Location = new Vector2(5, (height - logoSize) / 2)
            };
            _icon = icon;

            var nameLabel = AddChild<Label>();
            nameLabel.Height = 25;
            nameLabel.Location = new Vector2(icon.X + icon.Width + 5, (height - nameLabel.Height) / 2);
            nameLabel.Text = Name;
            nameLabel.HorizontalAlignment = TextAlignment.Near;

            var typeLabel = AddChild<Label>();
            typeLabel.Height = 25;
            typeLabel.Location = new Vector2((height - nameLabel.Height) / 2, X + width - typeLabel.Width - 17);
            typeLabel.HorizontalAlignment = TextAlignment.Far;
            typeLabel.Text = Type;
            typeLabel.TextColor = Style.Current.ForegroundGrey;
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Vector2 location, MouseButton button)
        {
            if (button == MouseButton.Left)
            {
                _finder.Hide();
                Editor.Instance.ContentFinding.Open(Item);
            }

            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override void OnMouseEnter(Vector2 location)
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

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            base.OnMouseLeave();

            var root = RootWindow;
            if (!_finder.Hand && root != null)
            {
                root.Cursor = CursorType.Default;
                _finder.SelectedItem = null;
            }
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _finder = null;
            _icon = null;

            base.OnDestroy();
        }
    }

    /// <summary>
    /// The <see cref="SearchItem"/> for assets. Supports using asset thumbnail.
    /// </summary>
    /// <seealso cref="FlaxEditor.Surface.ContextMenu.SearchItem" />
    /// <seealso cref="FlaxEditor.Content.IContentItemOwner" />
    public class AssetSearchItem : SearchItem, IContentItemOwner
    {
        private AssetItem _asset;

        /// <inheritdoc />
        public AssetSearchItem(string name, string type, AssetItem item, ContentFinder finder, float width, float height)
        : base(name, type, item, finder, width, height)
        {
            _asset = item;
            _asset.AddReference(this);
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            // Draw icon
            var iconRect = _icon.Bounds;
            _asset.DrawThumbnail(ref iconRect);
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
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
