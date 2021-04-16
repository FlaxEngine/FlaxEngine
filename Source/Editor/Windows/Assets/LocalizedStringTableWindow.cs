// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using FlaxEditor.Content;
using FlaxEditor.CustomEditors;
using FlaxEditor.GUI;
using FlaxEngine;
using FlaxEngine.GUI;

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
        private Proxy _proxy;

        private class Proxy
        {
            [EditorOrder(0), Tooltip("The locale of the localized string table (eg. pl-PL).")]
            public string Locale;

            [EditorOrder(10), Tooltip("The string table. Maps the message id into the localized text. For plural messages the list contains separate items for value numbers.")]
            public Dictionary<string, string[]> Entries;
        }

        /// <inheritdoc />
        public LocalizedStringTableWindow(Editor editor, AssetItem item)
        : base(editor, item)
        {
            // Toolstrip
            _saveButton = (ToolStripButton)_toolstrip.AddButton(editor.Icons.Save32, Save).LinkTooltip("Save");

            // Panel
            var panel = new Panel(ScrollBars.Vertical)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(0, 0, _toolstrip.Bottom, 0),
                Parent = this
            };

            // Properties
            _presenter = new CustomEditorPresenter(null, "Loading...");
            _presenter.Panel.Parent = panel;
            _presenter.Modified += MarkAsEdited;
        }

        /// <inheritdoc />
        public override void Save()
        {
            if (!IsEdited)
                return;

            _asset.Locale = _proxy.Locale;
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

            base.UpdateToolstrip();
        }

        /// <inheritdoc />
        protected override void OnAssetLoaded()
        {
            _proxy = new Proxy
            {
                Locale = _asset.Locale,
                Entries = _asset.Entries,
            };
            _presenter.Select(_proxy);
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
