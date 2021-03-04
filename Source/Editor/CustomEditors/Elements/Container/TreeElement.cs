// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using FlaxEditor.GUI.Tree;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Elements
{
    /// <summary>
    /// The tree structure element.
    /// </summary>
    /// <seealso cref="FlaxEditor.CustomEditors.LayoutElementsContainer" />
    public class TreeElement : LayoutElementsContainer, ITreeElement
    {
        /// <summary>
        /// The tree control.
        /// </summary>
        public readonly Tree TreeControl = new Tree(false);

        /// <inheritdoc />
        public override ContainerControl ContainerControl => TreeControl;

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
