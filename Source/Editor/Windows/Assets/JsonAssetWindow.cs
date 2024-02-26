// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Content;
using FlaxEditor.CustomEditors;
using FlaxEditor.GUI;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Windows.Assets
{
    /// <summary>
    /// Editor window to view/modify <see cref="JsonAsset"/> asset.
    /// </summary>
    /// <seealso cref="JsonAsset" />
    /// <seealso cref="FlaxEditor.Windows.Assets.AssetEditorWindow" />
    public sealed class JsonAssetWindow : AssetEditorWindowBase<JsonAsset>
    {
        private readonly CustomEditorPresenter _presenter;
        private readonly ToolStripButton _saveButton;
        private readonly ToolStripButton _undoButton;
        private readonly ToolStripButton _redoButton;
        private readonly Undo _undo;
        private object _object;
        private bool _isRegisteredForScriptsReload;
        private Label _typeText;

        /// <summary>
        /// Gets the instance of the Json asset object that is being edited.
        /// </summary>
        public object Instance => _object;

        /// <inheritdoc />
        public JsonAssetWindow(Editor editor, AssetItem item)
        : base(editor, item)
        {
            var inputOptions = Editor.Options.Options.Input;

            // Undo
            _undo = new Undo();
            _undo.UndoDone += OnUndoRedo;
            _undo.RedoDone += OnUndoRedo;
            _undo.ActionDone += OnUndoRedo;

            // Toolstrip
            _saveButton = (ToolStripButton)_toolstrip.AddButton(editor.Icons.Save64, Save).LinkTooltip("Save");
            _toolstrip.AddSeparator();
            _undoButton = (ToolStripButton)_toolstrip.AddButton(Editor.Icons.Undo64, _undo.PerformUndo).LinkTooltip($"Undo ({inputOptions.Undo})");
            _redoButton = (ToolStripButton)_toolstrip.AddButton(Editor.Icons.Redo64, _undo.PerformRedo).LinkTooltip($"Redo ({inputOptions.Redo})");

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

        private void OnScriptsReloadBegin()
        {
            Close();
        }

        /// <inheritdoc />
        public override void Save()
        {
            if (!IsEdited)
                return;
            if (_asset.WaitForLoaded())
                return;

            if (Editor.SaveJsonAsset(_item.Path, _object))
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
            _object = Asset.Instance;
            if (_object == null)
            {
                // Hint developer about cause of failure
                var dataTypeName = Asset.DataTypeName;
                var type = Type.GetType(dataTypeName);
                if (type != null)
                {
                    try
                    {
                        var obj = Activator.CreateInstance(type);
                        var data = Asset.Data;
                        FlaxEngine.Json.JsonSerializer.Deserialize(obj, data);
                    }
                    catch (Exception ex)
                    {
                        _presenter.NoSelectionText = "Failed to load asset. See log for more. " + ex.Message.Replace('\n', ' ');
                    }
                }
                else if (string.IsNullOrEmpty(dataTypeName))
                {
                    _presenter.NoSelectionText = "Empty data type.";
                }
                else
                {
                    _presenter.NoSelectionText = string.Format("Missing type '{0}'.", dataTypeName);
                }
            }
            _presenter.Select(_object);

            if (_typeText != null)
                _typeText.Dispose();
            var typeText = new ClickableLabel
            {
                Text = $"{Asset.DataTypeName}",
                TooltipText = "Asset data type (full name)",
                AnchorPreset = AnchorPresets.TopRight,
                AutoWidth = true,
                Parent = this,
            };
            typeText.LocalX += -(typeText.Width + 4);
            typeText.LocalY += (_toolstrip.Height - typeText.Height) * 0.5f;
            typeText.RightClick = () => Clipboard.Text = Asset.DataTypeName;
            _typeText = typeText;

            _undo.Clear();
            ClearEditedFlag();

            // Auto-close on scripting reload if json asset is from game scripts (it might be reloaded)
            if ((_object == null || FlaxEngine.Scripting.IsTypeFromGameScripts(_object.GetType())) && !_isRegisteredForScriptsReload)
            {
                _isRegisteredForScriptsReload = true;
                ScriptsBuilder.ScriptsReloadBegin += OnScriptsReloadBegin;
            }

            base.OnAssetLoaded();
        }

        /// <inheritdoc />
        protected override void OnAssetLoadFailed()
        {
            _presenter.NoSelectionText = "Failed to load the asset.";

            base.OnAssetLoadFailed();
        }

        /// <inheritdoc />
        public override void OnItemReimported(ContentItem item)
        {
            // Refresh the properties (will get new data in OnAssetLoaded)
            _presenter.Deselect();
            ClearEditedFlag();

            base.OnItemReimported(item);
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            if (_isRegisteredForScriptsReload)
            {
                _isRegisteredForScriptsReload = false;
                ScriptsBuilder.ScriptsReloadBegin -= OnScriptsReloadBegin;
            }
            _typeText = null;

            base.OnDestroy();
        }
    }
}
