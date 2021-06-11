// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using FlaxEditor.Content;
using FlaxEditor.GUI;
using FlaxEditor.Surface;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Windows.Assets
{
    /// <summary>
    /// The base class for editor windows that use <see cref="FlaxEditor.Surface.VisjectSurface"/> for content editing by graph functions (eg. material functions, particle emitter functions).
    /// </summary>
    /// <seealso cref="FlaxEditor.Windows.Assets.AssetEditorWindow" />
    /// <seealso cref="FlaxEditor.Surface.IVisjectSurfaceOwner" />
    public abstract class VisjectFunctionSurfaceWindow<TAsset, TSurface> : ClonedAssetEditorWindowBase<TAsset>, IVisjectSurfaceOwner
    where TAsset : Asset
    where TSurface : VisjectSurface
    {
        /// <summary>
        /// The surface.
        /// </summary>
        protected TSurface _surface;

        /// <summary>
        /// The panel for the surface.
        /// </summary>
        protected readonly Panel _panel;

        private readonly ToolStripButton _saveButton;
        private readonly ToolStripButton _undoButton;
        private readonly ToolStripButton _redoButton;
        private bool _showWholeGraphOnLoad = true;

        /// <summary>
        /// True if temporary asset is dirty, otherwise false.
        /// </summary>
        protected bool _tmpAssetIsDirty;

        /// <summary>
        /// True if window is waiting for asset load to load surface.
        /// </summary>
        protected bool _isWaitingForSurfaceLoad;

        /// <summary>
        /// The undo.
        /// </summary>
        protected Undo _undo;

        /// <summary>
        /// Gets the Visject Surface.
        /// </summary>
        public TSurface Surface => _surface;

        /// <summary>
        /// Gets the undo history context for this window.
        /// </summary>
        public Undo Undo => _undo;

        /// <inheritdoc />
        protected VisjectFunctionSurfaceWindow(Editor editor, AssetItem item)
        : base(editor, item)
        {
            // Undo
            _undo = new Undo();
            _undo.UndoDone += OnUndoRedo;
            _undo.RedoDone += OnUndoRedo;
            _undo.ActionDone += OnUndoRedo;

            // Toolstrip
            _saveButton = (ToolStripButton)_toolstrip.AddButton(Editor.Icons.Save64, Save).LinkTooltip("Save");
            _toolstrip.AddSeparator();
            _undoButton = (ToolStripButton)_toolstrip.AddButton(Editor.Icons.Undo64, _undo.PerformUndo).LinkTooltip("Undo (Ctrl+Z)");
            _redoButton = (ToolStripButton)_toolstrip.AddButton(Editor.Icons.Redo64, _undo.PerformRedo).LinkTooltip("Redo (Ctrl+Y)");
            _toolstrip.AddSeparator();
            _toolstrip.AddButton(editor.Icons.CenterView64, ShowWholeGraph).LinkTooltip("Show whole graph");

            // Panel
            _panel = new Panel(ScrollBars.None)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(0, 0, _toolstrip.Bottom, 0),
                Parent = this
            };

            // Setup input actions
            InputActions.Add(options => options.Undo, _undo.PerformUndo);
            InputActions.Add(options => options.Redo, _undo.PerformRedo);
        }

        private void OnUndoRedo(IUndoAction action)
        {
            MarkAsEdited();
            UpdateToolstrip();
        }

        /// <summary>
        /// Shows the whole surface graph.
        /// </summary>
        public void ShowWholeGraph()
        {
            _surface.ShowWholeGraph();
        }

        /// <summary>
        /// Refreshes temporary asset to see changes live when editing the surface.
        /// </summary>
        /// <returns>True if cannot refresh it, otherwise false.</returns>
        public bool RefreshTempAsset()
        {
            // Early check
            if (_asset == null || _isWaitingForSurfaceLoad)
                return true;

            // Check if surface has been edited
            if (_surface.IsEdited)
            {
                return SaveSurface();
            }

            return false;
        }

        /// <inheritdoc />
        public override void Save()
        {
            if (!IsEdited)
                return;

            if (RefreshTempAsset())
            {
                return;
            }

            if (SaveToOriginal())
            {
                return;
            }

            ClearEditedFlag();
            OnSurfaceEditedChanged();
            _item.RefreshThumbnail();
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
        protected override void UnlinkItem()
        {
            _isWaitingForSurfaceLoad = false;

            base.UnlinkItem();
        }

        /// <inheritdoc />
        protected override void OnAssetLinked()
        {
            _isWaitingForSurfaceLoad = true;

            base.OnAssetLinked();
        }

        /// <inheritdoc />
        public abstract string SurfaceName { get; }

        /// <inheritdoc />
        public abstract byte[] SurfaceData { get; set; }

        /// <inheritdoc />
        public void OnContextCreated(VisjectSurfaceContext context)
        {
        }

        /// <inheritdoc />
        public void OnSurfaceEditedChanged()
        {
            if (_surface.IsEdited)
                MarkAsEdited();
        }

        /// <inheritdoc />
        public void OnSurfaceGraphEdited()
        {
            // Mark as dirty
            _tmpAssetIsDirty = true;
        }

        /// <inheritdoc />
        public void OnSurfaceClose()
        {
            Close();
        }

        /// <summary>
        /// Loads the surface from the asset. Called during <see cref="Update"/> when asset is loaded and surface is missing.
        /// </summary>
        /// <returns>True if failed, otherwise false.</returns>
        protected virtual bool LoadSurface()
        {
            if (_surface.Load())
            {
                Editor.LogError("Failed to load surface.");
                return true;
            }
            return false;
        }

        /// <summary>
        /// Saves the surface to the asset. Called during <see cref="Update"/> when asset is loaded and surface is missing.
        /// </summary>
        /// <returns>True if failed, otherwise false.</returns>
        protected virtual bool SaveSurface()
        {
            _surface.Save();
            return false;
        }


        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            base.Update(deltaTime);

            if (_tmpAssetIsDirty)
            {
                _tmpAssetIsDirty = false;

                RefreshTempAsset();
            }

            if (_isWaitingForSurfaceLoad && _asset.IsLoaded)
            {
                _isWaitingForSurfaceLoad = false;

                if (LoadSurface())
                {
                    Close();
                    return;
                }

                // Setup
                _undo.Clear();
                _surface.Enabled = true;
                ClearEditedFlag();
                if (_showWholeGraphOnLoad)
                {
                    _showWholeGraphOnLoad = false;
                    _surface.ShowWholeGraph();
                }
            }
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _undo.Clear();

            base.OnDestroy();
        }
    }
}
