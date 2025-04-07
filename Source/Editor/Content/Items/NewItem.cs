// Copyright (c) Wojciech Figat. All rights reserved.

using System.Text;
using FlaxEngine;

namespace FlaxEditor.Content
{
    /// <summary>
    /// Helper content item used to mock UI during creating new assets by <see cref="FlaxEditor.Windows.ContentWindow"/>.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.ContentItem" />
    public sealed class NewItem : ContentItem
    {
        /// <summary>
        /// Gets the proxy object related to the created asset.
        /// </summary>
        public ContentProxy Proxy { get; }

        /// <summary>
        /// Gets the argument passed to the proxy for the item creation. In most cases it is null.
        /// </summary>
        public object Argument { get; }

        /// <summary>
        /// Initializes a new instance of the <see cref="NewItem"/> class.
        /// </summary>
        /// <param name="path">The path for the new item.</param>
        /// <param name="proxy">The content proxy object.</param>
        /// <param name="arg">The argument passed to the proxy for the item creation. In most cases it is null.</param>
        public NewItem(string path, ContentProxy proxy, object arg)
        : base(path)
        {
            Proxy = proxy;
            Argument = arg;
        }

        /// <inheritdoc />
        public override ContentItemType ItemType => ContentItemType.Other;

        /// <inheritdoc />
        public override ContentItemSearchFilter SearchFilter => ContentItemSearchFilter.Other;

        /// <inheritdoc />
        public override string TypeDescription => "New";

        /// <inheritdoc />
        public override SpriteHandle DefaultThumbnail => Editor.Instance.Icons.Document128;

        /// <inheritdoc />
        protected override bool DrawShadow => true;

        /// <inheritdoc />
        public override void UpdateTooltipText()
        {
            TooltipText = null;
        }
    }
}
