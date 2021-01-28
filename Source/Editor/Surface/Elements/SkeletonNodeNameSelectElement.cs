// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Text;
using FlaxEngine;

namespace FlaxEditor.Surface.Elements
{
    /// <summary>
    /// The Visject surface node element used to pick a skeleton node name with a combo box.
    /// </summary>
    [HideInEditor]
    public class SkeletonNodeNameSelectElement : ComboBoxElement
    {
        private readonly Dictionary<string, int> _nodeNameToIndex = new Dictionary<string, int>();

        /// <summary>
        /// Gets or sets the name of the selected node.
        /// </summary>
        public string SelectedNodeName
        {
            get
            {
                var selectedIndex = SelectedIndex;
                foreach (var e in _nodeNameToIndex)
                {
                    if (e.Value == selectedIndex)
                        return e.Key;
                }
                return string.Empty;
            }
            set
            {
                if (!string.IsNullOrEmpty(value) && _nodeNameToIndex.TryGetValue(value, out var index))
                    SelectedIndex = index;
                else
                    SelectedIndex = -1;
            }
        }

        /// <inheritdoc />
        public SkeletonNodeNameSelectElement(SurfaceNode parentNode, NodeElementArchetype archetype)
        : base(parentNode, archetype)
        {
            _isAutoSelect = true;

            UpdateComboBox();

            // Select saved value
            _selectedIndices.Clear();
            if (Archetype.ValueIndex != -1)
            {
                SelectedNodeName = (string)ParentNode.Values[Archetype.ValueIndex];
                ParentNode.ValuesChanged += OnNodeValuesChanged;
            }
        }

        private void OnNodeValuesChanged()
        {
            _selectedIndices.Clear();
            var selectedNode = (string)ParentNode.Values[Archetype.ValueIndex];
            if (!string.IsNullOrEmpty(selectedNode) && _nodeNameToIndex.TryGetValue(selectedNode, out var index))
                _selectedIndices.Add(index);
            OnSelectedIndexChanged();
        }

        /// <summary>
        /// Updates the Combo Box items list from the current skeleton nodes hierarchy.
        /// </summary>
        protected void UpdateComboBox()
        {
            // Prepare
            var selectedNode = SelectedNodeName;
            _items.Clear();
            _nodeNameToIndex.Clear();

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
            _items.Capacity = Mathf.Max(_items.Capacity, nodes.Length);
            var sb = new StringBuilder(64);
            for (int nodeIndex = 0; nodeIndex < nodes.Length; nodeIndex++)
            {
                sb.Clear();
                var node = nodes[nodeIndex];
                _nodeNameToIndex[node.Name] = nodeIndex;
                int parent = node.ParentIndex;
                while (parent != -1)
                {
                    sb.Append(" ");
                    parent = nodes[parent].ParentIndex;
                }

                sb.Append("> ");
                sb.Append(node.Name);

                _items.Add(sb.ToString());
            }

            // Restore the selected node
            SelectedNodeName = selectedNode;
        }

        /// <inheritdoc />
        protected override void OnSelectedIndexChanged()
        {
            if ((string)ParentNode.Values[Archetype.ValueIndex] != SelectedNodeName)
            {
                // Edit value
                ParentNode.SetValue(Archetype.ValueIndex, SelectedNodeName);
            }

            //base.OnSelectedIndexChanged();
        }
    }
}
