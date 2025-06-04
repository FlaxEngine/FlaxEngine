// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using FlaxEditor.Content.Settings;
using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.GUI.Tree;
using FlaxEditor.Scripting;
using FlaxEditor.Utilities;
using FlaxEngine;
using FlaxEngine.GUI;
using Utils = FlaxEditor.Utilities.Utils;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Default implementation of the inspector used to edit localized string properties.
    /// </summary>
    [CustomEditor(typeof(LocalizedString)), DefaultEditor]
    public sealed class LocalizedStringEditor : GenericEditor
    {
        private TextBoxElement _idElement, _valueElement;
        private Button _viewStringButton;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            base.Initialize(layout);

            if (layout.Children.Count == 0)
                return;
            var propList = layout.Children[layout.Children.Count - 1] as PropertiesListElement;
            if (propList == null || propList.Children.Count < 2)
                return;
            var idElement = propList.Children[propList.Children.Count - 2] as TextBoxElement;
            var valueElement = propList.Children[propList.Children.Count - 1] as TextBoxElement;
            if (idElement == null || valueElement == null)
                return;
            _idElement = idElement;
            _valueElement = valueElement;

            var attributes = Values.GetAttributes();
            var multiLine = attributes?.FirstOrDefault(x => x is MultilineTextAttribute);
            if (multiLine != null)
            {
                valueElement.TextBox.IsMultiline = true;
                valueElement.TextBox.Height *= 3;
            }

            var selectString = new Button
            {
                Width = 16.0f,
                Text = "...",
                TooltipText = "Select localized text from Localization Settings...",
                Parent = idElement.TextBox,
            };
            selectString.SetAnchorPreset(AnchorPresets.MiddleRight, false, true);
            selectString.ButtonClicked += OnSelectStringClicked;

            var addString = new Button
            {
                Width = 16.0f,
                Text = "+",
                TooltipText = "Add new localized text to Localization Settings (all used locales)",
                Parent = _valueElement.TextBox,
                Enabled = IsSingleObject,
            };
            addString.SetAnchorPreset(AnchorPresets.MiddleRight, false, true);
            addString.ButtonClicked += OnAddStringClicked;

            var viewString = new Button
            {
                Visible = false,
                Width = 16.0f,
                BackgroundColor = Color.White,
                BackgroundColorHighlighted = Color.Gray,
                BackgroundBrush = new SpriteBrush(Editor.Instance.Icons.Search12),
                TooltipText = "Find localized text in Localized String Table asset for the current locale...",
                Parent = valueElement.TextBox,
            };
            viewString.SetAnchorPreset(AnchorPresets.MiddleRight, false, true);
            viewString.LocalX -= 16.0f;
            viewString.ButtonClicked += OnViewStringClicked;
            _viewStringButton = viewString;
        }

        /// <inheritdoc />
        internal override void RefreshInternal()
        {
            base.RefreshInternal();

            if (_valueElement != null)
            {
                _valueElement.TextBox.WatermarkText = Localization.GetString(_idElement.Text);
                _viewStringButton.Visible = !string.IsNullOrEmpty(_valueElement.TextBox.WatermarkText);
            }
        }

        /// <inheritdoc />
        protected override void Deinitialize()
        {
            base.Deinitialize();

            _idElement = null;
            _valueElement = null;
        }

        private bool GetSettings(out LocalizationSettings settings)
        {
            settings = GameSettings.Load<LocalizationSettings>();
            if (settings?.LocalizedStringTables == null || settings.LocalizedStringTables.Length == 0)
            {
                MessageBox.Show("No valid localization settings setup.");
                return true;
            }
            return false;
        }

        private void OnSelectStringClicked(Button button)
        {
            if (GetSettings(out var settings))
                return;
            Profiler.BeginEvent("LocalizedStringEditor.OnSelectStringClicked");
            var allKeys = new HashSet<string>();
            for (int i = 0; i < settings.LocalizedStringTables.Length; i++)
            {
                var table = settings.LocalizedStringTables[i];
                if (table && !table.WaitForLoaded())
                {
                    var entries = table.Entries;
                    foreach (var e in entries)
                        allKeys.Add(e.Key);
                }
            }
            var allKeysSorted = allKeys.ToList();
            allKeysSorted.Sort();
            var value = _idElement?.TextBox.Text;
            var menu = Utils.CreateSearchPopup(out var searchBox, out var tree);
            var idToNode = new TreeNode[allKeysSorted.Count];
            for (var i = 0; i < allKeysSorted.Count; i++)
            {
                var key = allKeysSorted[i];
                var node = new TreeNode
                {
                    Text = key,
                    TooltipText = Localization.GetString(key),
                    Parent = tree,
                };
                if (key == value)
                    tree.Select(node);
                idToNode[i] = node;
            }
            tree.SelectedChanged += delegate(List<TreeNode> before, List<TreeNode> after)
            {
                if (after.Count == 1)
                {
                    menu.Hide();
                    _idElement.TextBox.SetTextAsUser(after[0].Text);
                    _valueElement.TextBox.SetTextAsUser(string.Empty);
                }
            };
            searchBox.TextChanged += delegate
            {
                if (tree.IsLayoutLocked)
                    return;
                tree.LockChildrenRecursive();
                var query = searchBox.Text;
                for (int i = 0; i < idToNode.Length; i++)
                {
                    var node = idToNode[i];
                    node.Visible = string.IsNullOrWhiteSpace(query) || QueryFilterHelper.Match(query, node.Text);
                }
                tree.UnlockChildrenRecursive();
                menu.PerformLayout();
            };
            menu.Show(button, new Float2(0, button.Height));
            Profiler.EndEvent();
        }

        private void OnAddStringClicked(Button button)
        {
            if (GetSettings(out var settings))
                return;
            Profiler.BeginEvent("LocalizedStringEditor.OnAddStringClicked");
            var allKeys = new HashSet<string>();
            for (int i = 0; i < settings.LocalizedStringTables.Length; i++)
            {
                var table = settings.LocalizedStringTables[i];
                if (table && !table.WaitForLoaded())
                {
                    var entries = table.Entries;
                    foreach (var e in entries)
                        allKeys.Add(e.Key);
                }
            }
            string newKey = null;
            if (string.IsNullOrEmpty(_idElement.Text))
            {
                CustomEditor customEditor = this;
                while (customEditor?.Values != null)
                {
                    if (customEditor.Values.Info != ScriptMemberInfo.Null)
                        if (newKey == null)
                            newKey = customEditor.Values.Info.Name;
                        else
                            newKey = customEditor.Values.Info.Name + '.' + newKey;
                    else if (customEditor.Values[0] is SceneObject sceneObject)
                        if (newKey == null)
                            newKey = sceneObject.GetNamePath('.');
                        else
                            newKey = sceneObject.GetNamePath('.') + '.' + newKey;
                    else
                        break;
                    customEditor = customEditor.ParentEditor;
                }
                if (string.IsNullOrWhiteSpace(newKey))
                    newKey = Guid.NewGuid().ToString("N");
            }
            else
            {
                newKey = _idElement.Text;
            }
            if (allKeys.Contains(newKey))
            {
                Profiler.EndEvent();
                if (_idElement.Text != newKey)
                    _idElement.TextBox.SetTextAsUser(newKey);
                else
                    MessageBox.Show("Already added.");
                return;
            }
            var newValue = _valueElement.Text;
            Editor.Log(newKey + (newValue != null ? " = " + newValue : string.Empty));
            var locales = settings.LocalizedStringTables.GroupBy(x => x.Locale);
            var defaultLocale = settings.LocalizedStringTables[0].Locale;
            foreach (var locale in locales)
            {
                var table = locale.First();
                var entries = table.Entries;
                if (table.Locale == defaultLocale)
                    entries[newKey] = new[] { newValue };
                else
                    entries[newKey] = new[] { string.Empty };
                table.Entries = entries;
                table.Save();
            }
            _valueElement.TextBox.SetTextAsUser(null);
            _idElement.TextBox.SetTextAsUser(newKey);
            Profiler.EndEvent();
        }

        private void OnViewStringClicked(Button button)
        {
            if (GetSettings(out var settings))
                return;
            var id = _idElement.TextBox.Text;
            var value = _valueElement.TextBox.WatermarkText;
            for (int i = 0; i < settings.LocalizedStringTables.Length; i++)
            {
                var table = settings.LocalizedStringTables[i];
                if (table && !table.WaitForLoaded())
                {
                    var entries = table.Entries;
                    if (entries.TryGetValue(id, out var messages))
                    {
                        if (messages.Length != 0 && messages[0] == value)
                        {
                            Editor.Instance.ContentEditing.Open(table);
                            return;
                        }
                    }
                }
            }
            MessageBox.Show("Unable to find localized string table.");
        }
    }
}
