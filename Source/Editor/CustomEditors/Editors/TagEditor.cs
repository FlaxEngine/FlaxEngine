// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

using FlaxEditor.GUI;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.Tree;
using FlaxEngine;
using FlaxEngine.GUI;
using System.Collections.Generic;
using System;
using System.Linq;
using System.Text;
using FlaxEditor.Content.Settings;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Custom editor for <see cref="Tag"/>.
    /// </summary>
    [CustomEditor(typeof(Tag)), DefaultEditor]
    public sealed class TagEditor : CustomEditor
    {
        private ClickableLabel _label;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            _label = layout.ClickableLabel(Tag.ToString()).CustomControl;
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
            _label.Text = Tag.ToString();
        }

        private Tag Tag
        {
            get
            {
                if (Values[0] is Tag asTag)
                    return asTag;
                if (Values[0] is uint asUInt)
                    return new Tag(asUInt);
                if (Values[0] is string asString)
                    return Tags.Get(asString);
                return Tag.Default;
            }
            set
            {
                if (Values[0] is Tag)
                    SetValue(value);
                if (Values[0] is uint)
                    SetValue(value.Index);
                else if (Values[0] is string)
                    SetValue(value.ToString());
            }
        }

        private void ShowPicker()
        {
            var menu = CreatePicker(Tag, null, new PickerData
            {
                IsSingle = true,
                SetValue = value => { Tag = value; },
            });
            menu.Show(_label, new Float2(0, _label.Height));
        }

        internal class PickerData
        {
            public bool IsEditing;
            public bool IsSingle;
            public Action<Tag> SetValue;
            public Action<Tag[]> SetValues;
            public List<Tag> CachedTags;
        }

        private static void UncheckAll(TreeNode n, TreeNode except = null)
        {
            foreach (var child in n.Children)
            {
                if (child is CheckBox checkBox && except != n)
                    checkBox.Checked = false;
                else if (child is TreeNode treeNode)
                    UncheckAll(treeNode, except);
            }
        }

        private static void UncheckAll(Tree n, TreeNode except = null)
        {
            foreach (var child in n.Children)
            {
                if (child is TreeNode treeNode)
                    UncheckAll(treeNode, except);
            }
        }

        private static void GetTags(TreeNode n, PickerData pickerData)
        {
            if (n is TreeNodeWithAddons a && a.Addons.Count != 0 && a.Addons[0] is CheckBox c && c.Checked)
                pickerData.CachedTags.Add((Tag)n.Tag);
            foreach (var child in n.Children)
            {
                if (child is TreeNode treeNode)
                    GetTags(treeNode, pickerData);
            }
        }

        private static void GetTags(Tree n, PickerData pickerData)
        {
            foreach (var child in n.Children)
            {
                if (child is TreeNode treeNode)
                    GetTags(treeNode, pickerData);
            }
        }

        private static void OnCheckboxEdited(TreeNode node, CheckBox c, PickerData pickerData)
        {
            if (pickerData.IsEditing)
                return;
            pickerData.IsEditing = true;
            if (pickerData.IsSingle)
            {
                UncheckAll(node.ParentTree, node);
                var value = c.Checked ? (Tag)node.Tag : Tag.Default;
                pickerData.SetValue?.Invoke(value);
                pickerData.SetValues?.Invoke(new[] { value });
            }
            else
            {
                if (pickerData.CachedTags == null)
                    pickerData.CachedTags = new List<Tag>();
                else
                    pickerData.CachedTags.Clear();
                GetTags(node.ParentTree, pickerData);
                pickerData.SetValue?.Invoke(pickerData.CachedTags.Count != 0 ? pickerData.CachedTags[0] : Tag.Default);
                pickerData.SetValues?.Invoke(pickerData.CachedTags.ToArray());
            }
            pickerData.IsEditing = false;
        }


        private static void OnAddButtonClicked(Tree tree, TreeNode parentNode, PickerData pickerData)
        {
            // Create new node
            var nodeIndent = 16.0f;
            var indentation = 0;
            var parentTag = string.Empty;
            if (parentNode.CustomArrowRect.HasValue)
            {
                indentation = (int)((parentNode.CustomArrowRect.Value.Location.X - 18) / nodeIndent) + 1;
                var parentTagValue = (Tag)parentNode.Tag;
                parentTag = parentTagValue.ToString();
            }
            var node = new TreeNodeWithAddons
            {
                ChildrenIndent = nodeIndent,
                CullChildren = false,
                ClipChildren = false,
                TextMargin = new Margin(22.0f, 2.0f, 2.0f, 2.0f),
                CustomArrowRect = new Rectangle(18 + indentation * nodeIndent, 2, 12, 12),
                BackgroundColorSelected = Color.Transparent,
                BackgroundColorHighlighted = parentNode.BackgroundColorHighlighted,
            };
            var checkbox = new CheckBox(32.0f + indentation * nodeIndent, 0)
            {
                Height = 16.0f,
                IsScrollable = false,
                Parent = node,
            };
            node.Addons.Add(checkbox);
            checkbox.StateChanged += c => OnCheckboxEdited(node, c, pickerData);
            var addButton = new Button(tree.Width - 16, 0, 14, 14)
            {
                Text = "+",
                TooltipText = "Add subtag within this tag namespace",
                IsScrollable = false,
                BorderColor = Color.Transparent,
                BackgroundColor = Color.Transparent,
                Parent = node,
                AnchorPreset = AnchorPresets.TopRight,
            };
            node.Addons.Add(addButton);
            addButton.ButtonClicked += button => OnAddButtonClicked(tree, node, pickerData);

            // Link
            node.Parent = parentNode;
            node.IndexInParent = 0;
            parentNode.Expand(true);
            ((Panel)tree.Parent.Parent).ScrollViewTo(node);

            // Start renaming the tag
            var prefix = parentTag.Length != 0 ? parentTag + '.' : string.Empty;
            var renameArea = node.HeaderRect;
            if (renameArea.Location.X < nodeIndent)
            {
                // Fix root node's child renaming
                renameArea.Location.X += nodeIndent;
                renameArea.Size.X -= nodeIndent;
            }
            var dialog = RenamePopup.Show(node, renameArea, prefix, false);
            var cursor = dialog.InputField.TextLength;
            dialog.InputField.SelectionRange = new TextRange(cursor, cursor);
            dialog.Validate = (popup, value) =>
            {
                // Ensure that name is unique
                if (Tags.List.Contains(value))
                    return false;

                // Ensure user entered direct subtag of the parent node
                if (value.StartsWith(popup.InitialValue))
                {
                    var name = value.Substring(popup.InitialValue.Length);
                    if (name.Length > 0 && name.IndexOf('.') == -1)
                        return true;
                }
                return false;
            };
            dialog.Renamed += popup =>
            {
                // Get tag name
                var tagName = popup.Text;
                var name = tagName.Substring(popup.InitialValue.Length);
                if (name.Length == 0)
                    return;

                // Add tag
                var tag = Tags.Get(tagName);
                node.Text = name;
                node.Tag = tag;
                var settingsAsset = GameSettings.LoadAsset<LayersAndTagsSettings>();
                if (settingsAsset && !settingsAsset.WaitForLoaded())
                {
                    // Save any local changes to the tags asset in Editor
                    var settingsAssetItem = Editor.Instance.ContentDatabase.FindAsset(settingsAsset.ID) as Content.JsonAssetItem;
                    var assetWindow = Editor.Instance.Windows.FindEditor(settingsAssetItem) as Windows.Assets.JsonAssetWindow;
                    assetWindow?.Save();

                    // Update asset
                    var settingsObj = (LayersAndTagsSettings)settingsAsset.Instance;
                    settingsObj.Tags.Add(tagName);
                    settingsAsset.SetInstance(settingsObj);
                }
            };
            dialog.Closed += popup =>
            {
                // Remove temporary node if renaming was canceled
                if (popup.InitialValue == popup.Text || popup.Text.Length == 0)
                    node.Dispose();
            };
        }

        internal static ContextMenuBase CreatePicker(Tag value, Tag[] values, PickerData pickerData)
        {
            // Initialize search popup
            var menu = Utilities.Utils.CreateSearchPopup(out var searchBox, out var tree, 20.0f);

            // Create tree with tags hierarchy
            tree.Margin = new Margin(-16.0f, 0.0f, -16.0f, -0.0f); // Hide root node
            var tags = Tags.List;
            var nameToNode = new Dictionary<string, ContainerControl>();
            var style = FlaxEngine.GUI.Style.Current;
            var nodeBackgroundColorHighlighted = style.BackgroundHighlighted * 0.5f;
            var nodeIndent = 16.0f;
            var root = tree.AddChild<TreeNode>();
            for (var i = 0; i < tags.Length; i++)
            {
                var tag = tags[i];
                var tagValue = new Tag((uint)(i + 1));
                bool isSelected = pickerData.IsSingle ? value == tagValue : values.Contains(tagValue);

                // Count parent tags count
                int indentation = 0;
                for (int j = 0; j < tag.Length; j++)
                {
                    if (tag[j] == '.')
                        indentation++;
                }

                // Create node
                var node = new TreeNodeWithAddons
                {
                    Tag = tagValue,
                    Text = tag,
                    ChildrenIndent = nodeIndent,
                    CullChildren = false,
                    ClipChildren = false,
                    TextMargin = new Margin(22.0f, 2.0f, 2.0f, 2.0f),
                    CustomArrowRect = new Rectangle(18 + indentation * nodeIndent, 2, 12, 12),
                    BackgroundColorSelected = Color.Transparent,
                    BackgroundColorHighlighted = nodeBackgroundColorHighlighted,
                };
                var checkbox = new CheckBox(32.0f + indentation * nodeIndent, 0, isSelected)
                {
                    Height = 16.0f,
                    IsScrollable = false,
                    Parent = node,
                };
                node.Addons.Add(checkbox);
                checkbox.StateChanged += c => OnCheckboxEdited(node, c, pickerData);
                var addButton = new Button(menu.Width - 16, 0, 14, 14)
                {
                    Text = "+",
                    TooltipText = "Add subtag within this tag namespace",
                    IsScrollable = false,
                    BorderColor = Color.Transparent,
                    BackgroundColor = Color.Transparent,
                    Parent = node,
                    AnchorPreset = AnchorPresets.TopRight,
                };
                node.Addons.Add(addButton);
                addButton.ButtonClicked += button => OnAddButtonClicked(tree, node, pickerData);

                // Link to parent
                {
                    var lastDotIndex = tag.LastIndexOf('.');
                    var parentTagName = lastDotIndex != -1 ? tag.Substring(0, lastDotIndex) : string.Empty;
                    if (!nameToNode.TryGetValue(parentTagName, out ContainerControl parent))
                        parent = root;
                    node.Parent = parent;
                }
                nameToNode[tag] = node;

                // Expand selected nodes to be visible in hierarchy
                if (isSelected)
                    node.ExpandAllParents(true);
            }
            nameToNode.Clear();

            // Adds top panel with utility buttons
            var buttonsPanel = new HorizontalPanel
            {
                Margin = new Margin(1.0f),
                AutoSize = false,
                Bounds = new Rectangle(0, 0, menu.Width, 20.0f),
                Parent = menu,
            };
            var buttonsSize = new Float2((menu.Width - buttonsPanel.Margin.Width) / 4.0f - buttonsPanel.Spacing, 18.0f);
            var buttonExpandAll = new Button
            {
                Size = buttonsSize,
                Parent = buttonsPanel,
                Text = "Expand all",
            };
            buttonExpandAll.Clicked += () => root.ExpandAll(true);
            var buttonCollapseAll = new Button
            {
                Size = buttonsSize,
                Parent = buttonsPanel,
                Text = "Collapse all",
            };
            buttonCollapseAll.Clicked += () =>
            {
                root.CollapseAll(true);
                root.Expand(true);
            };
            var buttonAddTag = new Button
            {
                Size = buttonsSize,
                Parent = buttonsPanel,
                Text = "Add Tag",
            };
            buttonAddTag.Clicked += () => OnAddButtonClicked(tree, root, pickerData);
            var buttonReset = new Button
            {
                Size = buttonsSize,
                Parent = buttonsPanel,
                Text = "Reset",
            };
            buttonReset.Clicked += () =>
            {
                pickerData.IsEditing = true;
                UncheckAll(root);
                pickerData.IsEditing = false;
                pickerData.SetValue?.Invoke(Tag.Default);
                pickerData.SetValues?.Invoke(null);
            };

            // Setup search filter
            searchBox.TextChanged += delegate
            {
                if (tree.IsLayoutLocked)
                    return;
                root.LockChildrenRecursive();
                Utilities.Utils.UpdateSearchPopupFilter(root, searchBox.Text);
                root.UnlockChildrenRecursive();
                menu.PerformLayout();
            };

            // Prepare for display
            root.SortChildrenRecursive();
            root.Expand(true);

            return menu;
        }
    }

    /// <summary>
    /// Custom editor for array of <see cref="Tag"/>.
    /// </summary>
    [CustomEditor(typeof(Tag[])), DefaultEditor]
    public sealed class TagsEditor : CustomEditor
    {
        private ClickableLabel _label;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            _label = layout.ClickableLabel(GetText(out _)).CustomControl;
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
            _label.Text = GetText(out var tags);
            _label.Height = Math.Max(tags.Length, 1) * 18.0f;
        }

        private string GetText(out Tag[] tags)
        {
            tags = Tags;
            if (tags.Length == 0)
                return string.Empty;
            if (tags.Length == 1)
                return tags[0].ToString();
            var sb = new StringBuilder();
            for (var i = 0; i < tags.Length; i++)
            {
                var tag = tags[i];
                if (i != 0)
                    sb.AppendLine();
                sb.Append(tag.ToString());
            }
            return sb.ToString();
        }

        private Tag[] Tags
        {
            get
            {
                if (Values[0] is Tag[] asArray)
                    return asArray;
                if (Values[0] is List<Tag> asList)
                    return asList.ToArray();
                return Utils.GetEmptyArray<Tag>();
            }
            set
            {
                if (Values[0] is Tag[])
                    SetValue(value);
                if (Values[0] is List<Tag>)
                    SetValue(new List<Tag>(value));
            }
        }

        private void ShowPicker()
        {
            var menu = TagEditor.CreatePicker(Tag.Default, Tags, new TagEditor.PickerData
            {
                SetValues = value => { Tags = value; },
            });
            menu.Show(_label, new Float2(0, _label.Height));
        }
    }
}
