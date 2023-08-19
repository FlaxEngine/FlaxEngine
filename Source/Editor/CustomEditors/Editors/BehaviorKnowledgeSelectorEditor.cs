// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.GUI;
using FlaxEditor.GUI.Tree;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Utilities;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Custom editor for <see cref="BehaviorKnowledgeSelector{T}"/> and <see cref="BehaviorKnowledgeSelectorAny"/>.
    /// </summary>
    public sealed class BehaviorKnowledgeSelectorEditor : CustomEditor
    {
        private ClickableLabel _label;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            _label = layout.ClickableLabel(Path).CustomControl;
            _label.RightClick += ShowPicker;
            var button = new Button
            {
                Size = new Float2(16.0f),
                Text = "...",
                TooltipText = "Edit...",
                Parent = _label,
            };
            button.SetAnchorPreset(AnchorPresets.MiddleRight, false, true);
            button.Clicked += ShowPicker;
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            // Update label
            _label.Text = Path;
        }

        private string Path
        {
            get
            {
                var v = Values[0];
                if (v is BehaviorKnowledgeSelectorAny any)
                    return any.Path;
                if (v is string str)
                    return str;
                var pathField = v.GetType().GetField("Path");
                return pathField.GetValue(v) as string;
            }
            set
            {
                var v = Values[0];
                if (v is BehaviorKnowledgeSelectorAny)
                    v = new BehaviorKnowledgeSelectorAny(value);
                else if (v is string)
                    v = value;
                else
                {
                    var pathField = v.GetType().GetField("Path");
                    pathField.SetValue(v, value);
                }
                SetValue(v);
            }
        }

        private void ShowPicker()
        {
            // Get Behavior Knowledge to select from
            var behaviorTreeWindow = Presenter.Owner as Windows.Assets.BehaviorTreeWindow;
            var blackboard = behaviorTreeWindow?.Blackboard;
            if (blackboard == null)
                return;
            var typed = ScriptType.Null;
            var valueType = Values[0].GetType();
            if (valueType.Name == "BehaviorKnowledgeSelector`1")
            {
                // Get typed selector type to show only assignable items
                typed = new ScriptType(valueType.GenericTypeArguments[0]);
            }
            // TODO: add support for selecting goals and sensors

            // Create menu with tree-like structure and search box
            var menu = Utilities.Utils.CreateSearchPopup(out var searchBox, out var tree, 0, true);
            var blackboardType = TypeUtils.GetObjectType(blackboard);
            var items = GenericEditor.GetItemsForType(blackboardType, blackboardType.IsClass, true);
            var selected = Path;
            var noneNode = new TreeNode
            {
                Text = "<none>",
                TooltipText = "Deselect value",
                Parent = tree,
            };
            if (string.IsNullOrEmpty(selected))
                tree.Select(noneNode);
            var blackboardNode = new TreeNode
            {
                Text = "Blackboard",
                TooltipText = blackboardType.TypeName,
                Tag = "Blackboard/", // Ability to select whole blackboard data
                Parent = tree,
            };
            if (typed && !typed.IsAssignableFrom(blackboardType))
                blackboardNode.Tag = null;
            if (string.Equals(selected, (string)blackboardNode.Tag, StringComparison.Ordinal))
                tree.Select(blackboardNode);
            foreach (var item in items)
            {
                if (typed && !typed.IsAssignableFrom(item.Info.ValueType))
                    continue;
                var path = "Blackboard/" + item.Info.Name;
                var node = new TreeNode
                {
                    Text = item.DisplayName,
                    TooltipText = item.TooltipText,
                    Tag = path,
                    Parent = blackboardNode,
                };
                if (string.Equals(selected, path, StringComparison.Ordinal))
                    tree.Select(node);
                // TODO: add support for nested items (eg. field from blackboard structure field)
            }
            blackboardNode.Expand(true);
            tree.SelectedChanged += delegate(List<TreeNode> before, List<TreeNode> after)
            {
                if (after.Count == 1)
                {
                    menu.Hide();
                    Path = after[0].Tag as string;
                }
            };
            menu.Show(_label, new Float2(0, _label.Height));
        }
    }
}
