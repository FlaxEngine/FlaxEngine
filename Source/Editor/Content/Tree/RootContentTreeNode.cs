// Copyright (c) Wojciech Figat. All rights reserved.

namespace FlaxEditor.Content
{
    /// <summary>
    /// Root tree node for the content workspace.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.ContentTreeNode" />
    public sealed class RootContentTreeNode : ContentTreeNode
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="RootContentTreeNode"/> class.
        /// </summary>
        public RootContentTreeNode()
        : base(null, string.Empty)
        {
        }

        /// <inheritdoc />
        public override string NavButtonLabel => " /";
    }
}
