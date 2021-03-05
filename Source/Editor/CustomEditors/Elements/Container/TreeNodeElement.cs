// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using FlaxEditor.GUI.Tree;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Elements
{
    /// <summary>
    /// Tree nodes elements.
    /// </summary>
    public interface ITreeElement
    {
        /// <summary>
        /// Adds new tree node element.
        /// </summary>
        /// <param name="text">The node name (title text).</param>
        /// <returns>The created element.</returns>
        TreeNodeElement Node(string text);
    }

    /// <summary>
    /// The tree structure node element.
    /// </summary>
    /// <seealso cref="FlaxEditor.CustomEditors.LayoutElementsContainer" />
    public class TreeNodeElement : LayoutElementsContainer, ITreeElement
    {
        /// <summary>
        /// The tree node control.
        /// </summary>
        public readonly TreeNode TreeNode = new TreeNode(false);

        /// <inheritdoc />
        public override ContainerControl ContainerControl => TreeNode;

        /// <inheritdoc />
        public TreeNodeElement Node(string text)
        {
            TreeNodeElement element = new TreeNodeElement();
            element.TreeNode.Text = text;
            OnAddElement(element);
            return element;
        }
    }
}
