// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Text;
using FlaxEngine;

namespace FlaxEditor.Surface.Elements
{
    /// <summary>
    /// The Visject surface node element used to pick a skeleton bone index with a combo box.
    /// </summary>
    [HideInEditor]
    public class SkeletonBoneIndexSelectElement : ComboBoxElement
    {
        /// <inheritdoc />
        public SkeletonBoneIndexSelectElement(SurfaceNode parentNode, NodeElementArchetype archetype)
        : base(parentNode, archetype)
        {
            _isAutoSelect = true;

            if (Surface == null)
                return;
            UpdateComboBox();

            // Select saved value
            _selectedIndices.Clear();
            if (Archetype.ValueIndex != -1)
            {
                var selectedIndex = (int)ParentNode.Values[Archetype.ValueIndex];
                if (selectedIndex > -1 && selectedIndex < _items.Count)
                    SelectedIndex = selectedIndex;
                ParentNode.ValuesChanged += OnNodeValuesChanged;
            }
        }

        private void OnNodeValuesChanged()
        {
            _selectedIndices.Clear();
            _selectedIndices.Add((int)ParentNode.Values[Archetype.ValueIndex]);
            OnSelectedIndexChanged();
        }

        /// <summary>
        /// Updates the Combo Box items list from the current skeleton nodes hierarchy.
        /// </summary>
        protected void UpdateComboBox()
        {
            // Prepare
            var selectedBone = SelectedItem;
            _items.Clear();

            // Get the skeleton
            var surfaceParam = Surface.GetParameter(Windows.Assets.AnimationGraphWindow.BaseModelId);
            var skeleton = surfaceParam != null ? FlaxEngine.Content.LoadAsync<SkinnedModel>((Guid)surfaceParam.Value) : null;
            if (skeleton == null || skeleton.WaitForLoaded())
            {
                SelectedIndex = -1;
                return;
            }

            // Update the items
            var nodes = skeleton.Nodes;
            var bones = skeleton.Bones;
            _items.Capacity = Mathf.Max(_items.Capacity, bones.Length);
            StringBuilder sb = new StringBuilder(64);
            for (int i = 0; i < bones.Length; i++)
            {
                sb.Clear();
                int parent = bones[i].ParentIndex;
                while (parent != -1)
                {
                    sb.Append(" ");
                    parent = bones[parent].ParentIndex;
                }

                sb.Append("> ");
                sb.Append(nodes[bones[i].NodeIndex].Name);

                _items.Add(sb.ToString());
            }

            // Restore the selected bone
            SelectedItem = selectedBone;
        }
    }
}
