// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Globalization;
using FlaxEditor.GUI;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.Tree;
using FlaxEditor.Utilities;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Default implementation of the inspector used to edit <see cref="CultureInfo"/> value type properties. Supports editing property of <see cref="string"/> type (as culture name).
    /// </summary>
    [CustomEditor(typeof(CultureInfo)), DefaultEditor]
    internal class CultureInfoEditor : CustomEditor
    {
        private ClickableLabel _label;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            _label = layout.ClickableLabel(GetName(Culture)).CustomControl;
            _label.RightClick += () => CreatePicker(Culture, OnValueChanged).Show(_label, new Float2(0, _label.Height));
            var button = new Button
            {
                Width = 16.0f,
                Text = "...",
                TooltipText = "Edit...",
                Parent = _label,
            };
            button.SetAnchorPreset(AnchorPresets.MiddleRight, false, true);
            button.ButtonClicked += b => CreatePicker(Culture, OnValueChanged).Show(b, new Float2(0, b.Height));
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            _label.Text = GetName(Culture);
        }

        /// <inheritdoc />
        protected override void Deinitialize()
        {
            _label = null;

            base.Deinitialize();
        }

        private CultureInfo Culture
        {
            get
            {
                if (Values[0] is CultureInfo asCultureInfo)
                    return asCultureInfo;
                if (Values[0] is string asString)
                    return new CultureInfo(asString);
                return null;
            }
            set
            {
                if (Values[0] is CultureInfo)
                    SetValue(value);
                else if (Values[0] is string)
                    SetValue(value.Name);
            }
        }

        private void OnValueChanged(CultureInfo value)
        {
            Culture = value;
        }

        private class CultureInfoComparer : IComparer<CultureInfo>
        {
            public int Compare(CultureInfo a, CultureInfo b)
            {
                return string.Compare(a.Name, b.Name, StringComparison.Ordinal);
            }
        }

        internal static ContextMenuBase CreatePicker(CultureInfo value, Action<CultureInfo> changed)
        {
            var menu = Utilities.Utils.CreateSearchPopup(out var searchBox, out var tree);
            tree.Margin = new Margin(-16.0f, 0.0f, -16.0f, -0.0f); // Hide root node
            var root = tree.AddChild<TreeNode>();
            var cultures = CultureInfo.GetCultures(CultureTypes.AllCultures);
            Array.Sort(cultures, 1, cultures.Length - 2, new CultureInfoComparer()); // at 0 there is Invariant Culture
            var lcidToNode = new Dictionary<int, ContainerControl>();
            for (var i = 0; i < cultures.Length; i++)
            {
                var culture = cultures[i];
                var node = new TreeNode
                {
                    Tag = culture,
                    Text = GetName(culture),
                };
                if (!lcidToNode.TryGetValue(culture.Parent.LCID, out ContainerControl parent))
                    parent = root;
                node.Parent = parent;
                lcidToNode[culture.LCID] = node;
            }
            if (value != null)
                tree.Select((TreeNode)lcidToNode[value.LCID]);
            tree.SelectedChanged += delegate(List<TreeNode> before, List<TreeNode> after)
            {
                if (after.Count == 1)
                {
                    menu.Hide();
                    changed((CultureInfo)after[0].Tag);
                }
            };
            searchBox.TextChanged += delegate
            {
                if (tree.IsLayoutLocked)
                    return;
                tree.LockChildrenRecursive();
                Utilities.Utils.UpdateSearchPopupFilter(root, searchBox.Text);
                tree.UnlockChildrenRecursive();
                menu.PerformLayout();
            };
            root.ExpandAll(true);
            return menu;
        }

        internal static string GetName(CultureInfo value)
        {
            return value != null ? string.Format("{0} - {1}", value.Name, value.EnglishName) : null;
        }
    }
}
