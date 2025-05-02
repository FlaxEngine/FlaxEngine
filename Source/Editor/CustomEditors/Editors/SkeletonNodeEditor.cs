// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.GUI;
using FlaxEngine;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Custom editor for picking skeleton node. Instead of choosing node index or entering node text it shows a combo box with simple tag picking by name.
    /// </summary>
    public sealed class SkeletonNodeEditor : CustomEditor
    {
        private ComboBoxElement element;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            element = layout.ComboBox();
            element.ComboBox.SelectedIndexChanged += OnSelectedIndexChanged;

            // Set node names
            if (ParentEditor != null
                && ParentEditor.Values.Count == 1 && ParentEditor.Values[0] is BoneSocket boneSocket
                && boneSocket.Parent is AnimatedModel animatedModel && animatedModel.SkinnedModel
                && !animatedModel.SkinnedModel.WaitForLoaded())
            {
                var nodes = animatedModel.SkinnedModel.Nodes;
                for (int nodeIndex = 0; nodeIndex < nodes.Length; nodeIndex++)
                    element.ComboBox.AddItem(nodes[nodeIndex].Name);
            }
        }

        private void OnSelectedIndexChanged(ComboBox comboBox)
        {
            string value = comboBox.SelectedItem;
            SetValue(value);
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            if (HasDifferentValues)
            {
                // TODO: support different values on many actor selected
            }
            else
            {
                string value = (string)Values[0];
                element.ComboBox.SelectedItem = value;
            }
        }
    }
}
