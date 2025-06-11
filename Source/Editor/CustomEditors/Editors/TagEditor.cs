// Copyright (c) Wojciech Figat. All rights reserved.

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

        private static void OnAddTagButtonClicked(string input, Tree tree, TextBox textBox, DropPanel dropPanel, PickerData pickerData)
        {
            if (string.IsNullOrEmpty(input))
                return;

            // Ensure that name is unique
            if (Tags.List.Contains(input))
                return;

            var subInputs = input.Split('.');
            string tagString = string.Empty;
            string lastTagString = string.Empty;

            TreeNode parentNode = tree.GetChild<TreeNode>(); // Start at root
            int parentCount = 0;
            for (int i = 0; i < subInputs.Length; i++)
            {
                if (string.IsNullOrEmpty(subInputs[i]))
                    continue;

                // Check all entered subtags and create any that dont exist
                for (int j = 0; j <= i; j++)
                {
                    tagString += j == 0 ? subInputs[j] : "." + subInputs[j];
                }

                if (string.IsNullOrEmpty(tagString))
                {
                    tagString = string.Empty;
                    continue;
                }

                if (Tags.List.Contains(tagString))
                {
                    // Find next parent node
                    foreach (var child in parentNode.Children)
                    {
                        if (!(child is TreeNode childNode))
                            continue;

                        var tagValue = (Tag)childNode.Tag;
                        if (!string.Equals(tagValue.ToString(), tagString))
                            continue;

                        parentNode = childNode;
                        parentCount += 1;
                    }
                    lastTagString = tagString;
                    tagString = string.Empty;
                    continue;
                }
                else if (subInputs.Length > 1)
                {
                    // Find next parent node
                    foreach (var child in parentNode.Children)
                    {
                        if (!(child is TreeNode childNode))
                            continue;

                        var tagValue = (Tag)childNode.Tag;
                        if (!string.Equals(tagValue.ToString(), lastTagString))
                            continue;

                        parentNode = childNode;
                        parentCount += 1;
                    }
                }

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
                addButton.ButtonClicked += button => OnAddSubTagButtonClicked(((Tag)node.Tag).ToString(), textBox, dropPanel);

                // Link
                node.Parent = parentNode;
                node.IndexInParent = 0;
                parentNode.Expand(true);
                ((Panel)tree.Parent.Parent).ScrollViewTo(node);

                // Get tag name
                var tagName = tagString;
                var tagShortName = tagName.Substring(parentCount == 0 ? lastTagString.Length : lastTagString.Length + 1);
                if (tagShortName.Length == 0)
                    return;

                // Add tag
                var tag = Tags.Get(tagName);
                node.Text = tagShortName;
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
                    settingsObj.Tags.Sort();
                    settingsAsset.SetInstance(settingsObj);
                    settingsAsset.Save();

                    // Reload editor window to reflect new tag
                    assetWindow?.RefreshAsset();
                }

                lastTagString = tagString;
                tagString = string.Empty;
            }
            textBox.Text = string.Empty;
        }

        private static void OnAddSubTagButtonClicked(string parentTag, TextBox textBox, DropPanel dropPanel)
        {
            if (textBox == null || dropPanel == null || string.IsNullOrEmpty(parentTag))
            {
                return;
            }
            dropPanel.Open();
            textBox.Text = parentTag + ".";
            textBox.Focus();
            textBox.SelectionRange = new TextRange(textBox.Text.Length, textBox.Text.Length);
        }

        internal static ContextMenuBase CreatePicker(Tag value, Tag[] values, PickerData pickerData)
        {
            // Initialize search popup
            var menu = Utilities.Utils.CreateSearchPopup(out var searchBox, out var tree, 42.0f);

            // Add tag drop panel
            var addTagDropPanel = new DropPanel
            {
                HeaderText = "Add Tag",
                EnableDropDownIcon = true,
                ArrowImageOpened = new SpriteBrush(FlaxEngine.GUI.Style.Current.ArrowDown),
                ArrowImageClosed = new SpriteBrush(FlaxEngine.GUI.Style.Current.ArrowRight),
                Parent = menu,
                HeaderTextMargin = new Margin(2.0f),
                HeaderHeight = 18.0f,
                AnchorPreset = AnchorPresets.TopLeft,
                Bounds = new Rectangle(2, 2, menu.Width - 4, 30),
                IsClosed = true,
                ItemsMargin = new Margin(2.0f),
                CloseAnimationTime = 0,
                Pivot = Float2.Zero
            };
            var tagNamePanel = new HorizontalPanel
            {
                Parent = addTagDropPanel,
                AutoSize = false,
                Bounds = new Rectangle(0, 0, addTagDropPanel.Width, 20.0f),
            };
            var nameLabel = new Label
            {
                Size = new Float2(addTagDropPanel.Width / 4, 0),
                Parent = tagNamePanel,
                Text = "Tag Name:",
            };
            var nameTextBox = new TextBox
            {
                IsMultiline = false,
                WatermarkText = "X.Y.Z",
                Size = new Float2(addTagDropPanel.Width * 0.7f, 0),
                Parent = tagNamePanel,
                EndEditOnClick = false,
            };

            bool uniqueText = true;
            nameTextBox.TextChanged += () =>
            {
                // Ensure that name is unique
                if (Tags.List.Contains(nameTextBox.Text))
                    uniqueText = false;
                else
                    uniqueText = true;

                var currentStyle = FlaxEngine.GUI.Style.Current;
                if (uniqueText)
                {
                    nameTextBox.BorderColor = Color.Transparent;
                    nameTextBox.BorderSelectedColor = currentStyle.BackgroundSelected;
                }
                else
                {
                    var color = new Color(1.0f, 0.0f, 0.02745f, 1.0f);
                    nameTextBox.BorderColor = Color.Lerp(color, currentStyle.TextBoxBackground, 0.6f);
                    nameTextBox.BorderSelectedColor = color;
                }
            };

            nameTextBox.EditEnd += () =>
            {
                if (!uniqueText)
                    return;

                if (addTagDropPanel.IsClosed)
                {
                    nameTextBox.BorderColor = Color.Transparent;
                    nameTextBox.BorderSelectedColor = FlaxEngine.GUI.Style.Current.BackgroundSelected;
                    return;
                }

                OnAddTagButtonClicked(nameTextBox.Text, tree, nameTextBox, addTagDropPanel, pickerData);
            };

            var addButtonPanel = new HorizontalPanel
            {
                Parent = addTagDropPanel,
                AutoSize = false,
                Bounds = new Rectangle(0, 0, addTagDropPanel.Width, 30.0f),
            };
            var buttonAddTag = new Button
            {
                Parent = addButtonPanel,
                Size = new Float2(100, 18),
                Text = "Add Tag",
                AnchorPreset = AnchorPresets.MiddleCenter,
            };
            buttonAddTag.Clicked += () =>
            {
                if (!uniqueText)
                    return;
                OnAddTagButtonClicked(nameTextBox.Text, tree, nameTextBox, addTagDropPanel, pickerData);
            };

            // Used for how far everything should drop when the drop down panel is opened
            var dropPanelOpenHeight = tagNamePanel.Height + addButtonPanel.Height + 4;

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
                var tagName = tags[i];
                var tagValue = new Tag((uint)(i + 1));
                bool isSelected = pickerData.IsSingle ? value == tagValue : values.Contains(tagValue);

                // Count parent tags count
                int indentation = 0, lastDotIndex = -1;
                for (int j = 0; j < tagName.Length; j++)
                {
                    if (tagName[j] == '.')
                    {
                        indentation++;
                        lastDotIndex = j;
                    }
                }
                var tagShortName = tagName;
                var tagParentName = string.Empty;
                if (lastDotIndex != -1)
                {
                    tagShortName = tagName.Substring(lastDotIndex + 1);
                    tagParentName = tagName.Substring(0, lastDotIndex);
                }

                // Create node
                var node = new TreeNodeWithAddons
                {
                    Tag = tagValue,
                    Text = tagShortName,
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
                addButton.ButtonClicked += button => OnAddSubTagButtonClicked(((Tag)node.Tag).ToString(), nameTextBox, addTagDropPanel);

                // Link to parent
                {
                    if (!nameToNode.TryGetValue(tagParentName, out ContainerControl parent))
                        parent = root;
                    node.Parent = parent;
                }
                nameToNode[tagName] = node;

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
                Bounds = new Rectangle(0, 0, menu.Width - 4, 20.0f),
                Parent = menu,
            };
            buttonsPanel.Y += addTagDropPanel.HeaderHeight + 4;
            buttonsPanel.X += 2;

            var buttonsSize = new Float2((menu.Width - 2 - buttonsPanel.Margin.Width) / 3.0f - buttonsPanel.Spacing, 18.0f);
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
            var buttonClearAll = new Button
            {
                Size = buttonsSize,
                Parent = buttonsPanel,
                Text = "Clear all",
            };
            buttonClearAll.Clicked += () =>
            {
                pickerData.IsEditing = true;
                UncheckAll(root);
                pickerData.IsEditing = false;
                pickerData.SetValue?.Invoke(Tag.Default);
                pickerData.SetValues?.Invoke(null);
            };

            // Move menu children to location when drop panel is opened and closed
            addTagDropPanel.IsClosedChanged += panel =>
            {
                if (panel.IsClosed)
                {
                    // Resize/ Move UI to fit space with drop down
                    foreach (var child in menu.Children)
                    {
                        if (child == panel)
                            continue;

                        // Expand panels so scrollbars work
                        if (child is Panel)
                        {
                            child.Bounds = new Rectangle(child.Bounds.X, child.Bounds.Y - dropPanelOpenHeight, child.Width, child.Height + dropPanelOpenHeight);
                            continue;
                        }
                        child.Y -= dropPanelOpenHeight;
                        nameTextBox.Text = string.Empty;
                        nameTextBox.Defocus();
                    }
                }
                else
                {
                    // Resize/ Move UI to fit space with drop down
                    foreach (var child in menu.Children)
                    {
                        if (child == panel)
                            continue;

                        // Shrink panels so scrollbars work
                        if (child is Panel)
                        {
                            child.Bounds = new Rectangle(child.Bounds.X, child.Bounds.Y + dropPanelOpenHeight, child.Width, child.Height - dropPanelOpenHeight);
                            continue;
                        }
                        child.Y += dropPanelOpenHeight;
                        nameTextBox.Text = string.Empty;
                    }
                }
            };

            // Setup search filter
            searchBox.TextChanged += delegate
            {
                if (tree.IsLayoutLocked)
                    return;
                tree.LockChildrenRecursive();
                Utilities.Utils.UpdateSearchPopupFilter(root, searchBox.Text);
                tree.UnlockChildrenRecursive();
                menu.PerformLayout();
            };

            // Prepare for display
            root.SortChildrenRecursive();
            root.Expand(true);

            if (Input.GetKey(KeyboardKeys.Shift))
                root.ExpandAll(true);

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
            var buttonText = "...";
            var button = new Button
            {
                Size = new Float2(16.0f),
                Text = buttonText,
                TooltipText = "Edit...",
                Parent = _label,
            };
            var textSize = FlaxEngine.GUI.Style.Current.FontMedium.MeasureText(buttonText);
            if (textSize.Y > button.Width)
                button.Width = textSize.Y + 2;

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
                if (Values[0] is Tag[] || Values.Type.Type == typeof(Tag[]))
                    SetValue(value);
                else if (Values[0] is List<Tag> || Values.Type.Type == typeof(List<Tag>))
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
