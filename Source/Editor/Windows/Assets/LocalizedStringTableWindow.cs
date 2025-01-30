// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.Content;
using FlaxEditor.CustomEditors;
using FlaxEditor.GUI;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Utilities;

namespace FlaxEditor.Windows.Assets
{
    /// <summary>
    /// Editor window to view/modify <see cref="LocalizedStringTable"/> asset.
    /// </summary>
    /// <seealso cref="LocalizedStringTable" />
    /// <seealso cref="FlaxEditor.Windows.Assets.AssetEditorWindow" />
    public sealed class LocalizedStringTableWindow : AssetEditorWindowBase<LocalizedStringTable>
    {
        private readonly CustomEditorPresenter _presenter;
        private readonly ToolStripButton _saveButton;
        private readonly ToolStripButton _undoButton;
        private readonly ToolStripButton _redoButton;
        private readonly Undo _undo;
        private Proxy _proxy;

        private class EntryEditor : CustomEditor
        {
            private TextBox[] _textBoxes;
            private bool _isRefreshing;

            public override void Initialize(LayoutElementsContainer layout)
            {
                var values = (string[])Values[0];
                if (values == null || values.Length == 0)
                {
                    values = new string[1];
                    values[0] = string.Empty;
                }
                if (_textBoxes == null || _textBoxes.Length != values.Length)
                    _textBoxes = new TextBox[values.Length];
                for (int i = 0; i < values.Length; i++)
                {
                    var value = values[i];
                    var textBox = layout.TextBox(value.IsMultiline());
                    textBox.TextBox.Tag = i;
                    textBox.TextBox.Text = value;
                    textBox.TextBox.TextBoxEditEnd += OnEditEnd;
                    _textBoxes[i] = textBox.TextBox;
                }
            }

            public override void Refresh()
            {
                base.Refresh();

                var values = (string[])Values[0];
                if (values != null && values.Length == _textBoxes.Length)
                {
                    _isRefreshing = true;
                    var style = FlaxEngine.GUI.Style.Current;
                    var wrongColor = new Color(1.0f, 0.0f, 0.02745f, 1.0f);
                    var wrongColorBorder = Color.Lerp(wrongColor, style.TextBoxBackground, 0.6f);
                    for (int i = 0; i < _textBoxes.Length; i++)
                    {
                        var textBox = _textBoxes[i];
                        if (!textBox.IsEditing)
                        {
                            textBox.Text = values[i];
                            if (string.IsNullOrEmpty(textBox.Text))
                            {
                                textBox.BorderColor = wrongColorBorder;
                                textBox.BorderSelectedColor = wrongColor;
                            }
                            else
                            {
                                textBox.BorderColor = Color.Transparent;
                                textBox.BorderSelectedColor = style.BackgroundSelected;
                            }
                        }
                    }
                    _isRefreshing = false;
                }
            }

            protected override void Deinitialize()
            {
                base.Deinitialize();

                _textBoxes = null;
                _isRefreshing = false;
            }

            private void OnEditEnd(TextBoxBase textBox)
            {
                if (_isRefreshing)
                    return;
                var values = (string[])Values[0];
                var length = Mathf.Max(values?.Length ?? 0, 1);
                var toSet = new string[length];
                if (values != null && values.Length == length)
                    Array.Copy(values, toSet, length);
                var index = (int)textBox.Tag;
                toSet[index] = textBox.Text;
                SetValue(toSet);
            }
        }

        private class Proxy
        {
            [EditorOrder(0), EditorDisplay("General"), Tooltip("The locale of the localized string table (eg. pl-PL)."),]
            [CustomEditor(typeof(FlaxEditor.CustomEditors.Editors.CultureInfoEditor))]
            public string Locale;

            [EditorOrder(5), EditorDisplay("General"), Tooltip("The fallback language table to use for missing keys. Eg. table for 'en-GB' can point to 'en' as a fallback to prevent problem of missing localized strings.")]
            [AssetReference(true)]
            public LocalizedStringTable FallbackTable;

            [EditorOrder(10), EditorDisplay("Entries", EditorDisplayAttribute.InlineStyle), Tooltip("The string table. Maps the message id into the localized text. For plural messages the list contains separate items for value numbers.")]
            [Collection(Spacing = 10, OverrideEditorTypeName = "FlaxEditor.Windows.Assets.LocalizedStringTableWindow+EntryEditor")]
            public Dictionary<string, string[]> Entries;
        }

        /// <inheritdoc />
        public LocalizedStringTableWindow(Editor editor, AssetItem item)
        : base(editor, item)
        {
            var inputOptions = Editor.Options.Options.Input;

            // Undo
            _undo = new Undo();
            _undo.UndoDone += OnUndoRedo;
            _undo.RedoDone += OnUndoRedo;
            _undo.ActionDone += OnUndoRedo;

            // Toolstrip
            _saveButton = _toolstrip.AddButton(editor.Icons.Save64, Save).LinkTooltip("Save", ref inputOptions.Save);
            _toolstrip.AddSeparator();
            _undoButton = _toolstrip.AddButton(Editor.Icons.Undo64, _undo.PerformUndo).LinkTooltip("Undo", ref inputOptions.Undo);
            _redoButton = _toolstrip.AddButton(Editor.Icons.Redo64, _undo.PerformRedo).LinkTooltip("Redo", ref inputOptions.Redo);
            _toolstrip.AddSeparator();
            _toolstrip.AddButton(Editor.Icons.Up64, OnExport).LinkTooltip("Export localization table entries for translation to .pot file");

            // Panel
            var panel = new Panel(ScrollBars.Vertical)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(0, 0, _toolstrip.Bottom, 0),
                Parent = this
            };

            // Properties
            _presenter = new CustomEditorPresenter(_undo, "Loading...");
            _presenter.Panel.Parent = panel;
            _presenter.Modified += MarkAsEdited;

            // Setup input actions
            InputActions.Add(options => options.Undo, _undo.PerformUndo);
            InputActions.Add(options => options.Redo, _undo.PerformRedo);
        }

        private void OnUndoRedo(IUndoAction action)
        {
            MarkAsEdited();
            UpdateToolstrip();
        }

        private void OnExport()
        {
            var tableEntries = new Dictionary<LocalizedStringTable, Dictionary<string, string[]>>();
            tableEntries[Asset] = _proxy.Entries;
            CustomEditors.Dedicated.LocalizationSettingsEditor.Export(tableEntries);
        }

        /// <inheritdoc />
        public override void Save()
        {
            if (!IsEdited)
                return;

            _asset.Locale = _proxy.Locale;
            _asset.FallbackTable = _proxy.FallbackTable;
            _asset.Entries = _proxy.Entries;
            if (_asset.Save(_item.Path))
            {
                Editor.LogError("Cannot save asset.");
                return;
            }

            ClearEditedFlag();
        }

        /// <inheritdoc />
        protected override void UpdateToolstrip()
        {
            _saveButton.Enabled = IsEdited;
            _undoButton.Enabled = _undo.CanUndo;
            _redoButton.Enabled = _undo.CanRedo;

            base.UpdateToolstrip();
        }

        /// <inheritdoc />
        protected override void OnAssetLoaded()
        {
            _proxy = new Proxy
            {
                Locale = _asset.Locale,
                FallbackTable = _asset.FallbackTable,
                Entries = _asset.Entries,
            };
            _presenter.Select(_proxy);
            _undo.Clear();
            ClearEditedFlag();

            base.OnAssetLoaded();
        }

        /// <inheritdoc />
        public override void OnItemReimported(ContentItem item)
        {
            // Refresh the properties (will get new data in OnAssetLoaded)
            _presenter.Deselect();
            ClearEditedFlag();

            base.OnItemReimported(item);
        }
    }
}
