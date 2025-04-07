// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.SceneGraph.Actors;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.SceneGraph.GUI
{
    /// <summary>
    /// A <see cref="ActorTreeNode"/> custom implementation for <see cref="SceneNode"/>.
    /// </summary>
    /// <seealso cref="FlaxEditor.SceneGraph.GUI.ActorTreeNode" />
    public sealed class SceneTreeNode : ActorTreeNode
    {
        /// <inheritdoc />
        public override void UpdateText()
        {
            base.UpdateText();

            // Append asset name if used
            var filename = ((Scene)Actor).Filename;
            if (!string.IsNullOrEmpty(filename))
            {
                Text += " (";
                Text += filename;
                Text += ")";
            }

            // Append star character to modified scenes
            if (ActorNode is SceneNode node && node.IsEdited)
                Text += "*";
        }

        /// <inheritdoc />
        protected override Color CacheTextColor()
        {
            return Style.Current.Foreground;
        }
    }
}
