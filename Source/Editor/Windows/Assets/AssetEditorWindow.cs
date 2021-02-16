// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Content;
using FlaxEditor.GUI;
using FlaxEditor.GUI.ContextMenu;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Windows.Assets
{
    /// <summary>
    /// Base class for assets editing/viewing windows.
    /// </summary>
    /// <seealso cref="FlaxEditor.Windows.EditorWindow" />
    public abstract class AssetEditorWindow : EditorWindow, IEditable, IContentItemOwner
    {
        /// <summary>
        /// The item.
        /// </summary>
        protected AssetItem _item;

        /// <summary>
        /// The toolstrip.
        /// </summary>
        protected readonly ToolStrip _toolstrip;

        /// <summary>
        /// Gets the item.
        /// </summary>
        public AssetItem Item => _item;

        /// <summary>
        /// Gets the toolstrip UI.
        /// </summary>
        public ToolStrip ToolStrip => _toolstrip;

        /// <inheritdoc />
        public override string SerializationTypename => _item.ID.ToString();

        /// <summary>
        /// Initializes a new instance of the <see cref="AssetEditorWindow"/> class.
        /// </summary>
        /// <param name="editor">The editor.</param>
        /// <param name="item">The item.</param>
        protected AssetEditorWindow(Editor editor, AssetItem item)
        : base(editor, false, ScrollBars.None)
        {
            _item = item ?? throw new ArgumentNullException(nameof(item));
            _item.AddReference(this);

            _toolstrip = new ToolStrip
            {
                Parent = this
            };
            _toolstrip.AddButton(editor.Icons.Find32, () => Editor.Windows.ContentWin.Select(_item)).LinkTooltip("Show and select in Content Window");

            InputActions.Add(options => options.Save, Save);

            UpdateTitle();
        }

        /// <summary>
        /// Unlinks the item. Removes reference to it and unbinds all events.
        /// </summary>
        protected virtual void UnlinkItem()
        {
            _item.RemoveReference(this);
            _item = null;
        }

        /// <summary>
        /// Updates the toolstrip buttons and other controls. Called after some window events.
        /// </summary>
        protected virtual void UpdateToolstrip()
        {
        }

        /// <summary>
        /// Gets the name of the window title format text ({0} to insert asset short name).
        /// </summary>
        protected virtual string WindowTitleName => "{0}";

        /// <summary>
        /// Updates the window title text.
        /// </summary>
        protected void UpdateTitle()
        {
            string title = string.Format(WindowTitleName, _item?.ShortName ?? string.Empty);
            if (IsEdited)
                title += '*';
            Title = title;
        }

        /// <summary>
        /// Tries to save asset changes if it has been edited.
        /// </summary>
        public virtual void Save()
        {
        }

        /// <inheritdoc />
        public override bool IsEditingItem(ContentItem item)
        {
            return item == _item;
        }

        /// <inheritdoc />
        protected override bool OnClosing(ClosingReason reason)
        {
            // Block closing only on user events
            if (reason == ClosingReason.User)
            {
                // Check if asset has been edited and not saved (and still has linked item)
                if (IsEdited && _item != null)
                {
                    // Ask user for further action
                    var result = MessageBox.Show(
                        string.Format("Asset \'{0}\' has been edited. Save before closing?", _item.Path),
                        "Save before closing?",
                        MessageBoxButtons.YesNoCancel
                    );
                    if (result == DialogResult.OK || result == DialogResult.Yes)
                    {
                        // Save and close
                        Save();
                    }
                    else if (result == DialogResult.Cancel || result == DialogResult.Abort)
                    {
                        // Cancel closing
                        return true;
                    }
                }
            }

            return base.OnClosing(reason);
        }

        /// <inheritdoc />
        protected override void OnClose()
        {
            if (_item != null)
            {
                // Ensure to remove linkage to the item
                UnlinkItem();
            }

            base.OnClose();
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            if (_item != null)
            {
                // Ensure to remove linkage to the item
                UnlinkItem();
            }

            base.OnDestroy();
        }

        #region IEditable Implementation

        private bool _isEdited;

        /// <summary>
        /// Occurs when object gets edited.
        /// </summary>
        public event Action OnEdited;

        /// <inheritdoc />
        public bool IsEdited
        {
            get => _isEdited;
            protected set
            {
                if (value)
                    MarkAsEdited();
                else
                    ClearEditedFlag();
            }
        }

        /// <inheritdoc />
        public void MarkAsEdited()
        {
            // Check if state will change
            if (_isEdited == false)
            {
                // Set flag
                _isEdited = true;

                // Call events
                OnEditedState();
                OnEdited?.Invoke();
                OnEditedStateChanged();
            }
        }

        /// <summary>
        /// Clears the edited flag.
        /// </summary>
        protected void ClearEditedFlag()
        {
            // Check if state will change
            if (_isEdited)
            {
                // Clear flag
                _isEdited = false;

                // Call event
                OnEditedStateChanged();
            }
        }

        /// <summary>
        /// Action fired when object gets edited.
        /// </summary>
        protected virtual void OnEditedState()
        {
        }

        /// <summary>
        /// Action fired when object edited state gets changed.
        /// </summary>
        protected virtual void OnEditedStateChanged()
        {
            UpdateTitle();
            UpdateToolstrip();
        }

        /// <inheritdoc />
        public override void OnShowContextMenu(ContextMenu menu)
        {
            base.OnShowContextMenu(menu);

            menu.AddButton("Save", Save).Enabled = IsEdited;
            menu.AddButton("Copy name", () => Clipboard.Text = Item.NamePath);

            menu.AddSeparator();
        }

        #endregion

        #region IContentItemOwner Implementation

        /// <inheritdoc />
        public void OnItemDeleted(ContentItem item)
        {
            if (item == _item)
            {
                Close();
            }
        }

        /// <inheritdoc />
        public void OnItemRenamed(ContentItem item)
        {
            if (item == _item)
            {
                UpdateTitle();
            }
        }

        /// <inheritdoc />
        public virtual void OnItemReimported(ContentItem item)
        {
        }

        /// <inheritdoc />
        public void OnItemDispose(ContentItem item)
        {
            if (item == _item)
            {
                Close();
            }
        }

        #endregion
    }

    /// <summary>
    /// Generic base class for asset editors.
    /// </summary>
    /// <typeparam name="T">Asset type.</typeparam>
    /// <seealso cref="FlaxEditor.Windows.Assets.AssetEditorWindow" />
    public abstract class AssetEditorWindowBase<T> : AssetEditorWindow where T : Asset
    {
        /// <summary>
        /// Flag set to true if window is waiting for asset to be loaded (to send <see cref="OnAssetLoaded"/> or <see cref="OnAssetLoadFailed"/> events).
        /// </summary>
        protected bool _isWaitingForLoaded;

        /// <summary>
        /// The asset reference.
        /// </summary>
        protected T _asset;

        /// <summary>
        /// Gets the asset.
        /// </summary>
        public T Asset => _asset;

        /// <inheritdoc />
        protected AssetEditorWindowBase(Editor editor, AssetItem item)
        : base(editor, item)
        {
        }

        /// <summary>
        /// Reloads the asset (window will receive <see cref="OnAssetLoaded"/> or <see cref="OnAssetLoadFailed"/> events).
        /// </summary>
        public void ReloadAsset()
        {
            _asset.Reload();
            _isWaitingForLoaded = true;
        }

        /// <summary>
        /// Loads the asset.
        /// </summary>
        /// <returns>Loaded asset or null if cannot do it.</returns>
        protected virtual T LoadAsset()
        {
            return FlaxEngine.Content.LoadAsync<T>(_item.Path);
        }

        /// <summary>
        /// Called when asset gets linked and window can setup UI for it.
        /// </summary>
        protected virtual void OnAssetLinked()
        {
        }

        /// <summary>
        /// Called when asset gets loaded and window can setup UI for it.
        /// </summary>
        protected virtual void OnAssetLoaded()
        {
        }

        /// <summary>
        /// Called when asset fails to load and window can setup UI for it.
        /// </summary>
        protected virtual void OnAssetLoadFailed()
        {
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            if (_isWaitingForLoaded)
            {
                if (_asset == null)
                {
                    _isWaitingForLoaded = false;
                }
                else if (_asset.IsLoaded)
                {
                    _isWaitingForLoaded = false;
                    OnAssetLoaded();
                }
                else if (_asset.LastLoadFailed)
                {
                    _isWaitingForLoaded = false;
                    OnAssetLoadFailed();
                }
            }

            base.Update(deltaTime);
        }

        /// <inheritdoc />
        protected override void OnShow()
        {
            // Check if has no asset (but has item linked)
            if (_asset == null && _item != null)
            {
                // Load asset
                _asset = LoadAsset();
                if (_asset == null)
                {
                    Editor.LogError(string.Format("Cannot load asset \'{0}\' ({1})", _item.Path, typeof(T)));
                    Close();
                    return;
                }

                // Fire event
                OnAssetLinked();
                _isWaitingForLoaded = true;
            }

            // Base
            base.OnShow();

            // Update
            UpdateTitle();
            UpdateToolstrip();
            PerformLayout();
        }

        /// <inheritdoc />
        protected override void UnlinkItem()
        {
            _asset = null;

            base.UnlinkItem();
        }

        /// <inheritdoc />
        public override void OnItemReimported(ContentItem item)
        {
            // Wait for loaded after reimport
            _isWaitingForLoaded = true;

            base.OnItemReimported(item);
        }
    }

    /// <summary>
    /// Generic base class for asset editors that modify cloned asset and update original asset on save.
    /// </summary>
    /// <typeparam name="T">Asset type.</typeparam>
    /// <seealso cref="FlaxEditor.Windows.Assets.AssetEditorWindow" />
    public abstract class ClonedAssetEditorWindowBase<T> : AssetEditorWindowBase<T> where T : Asset
    {
        // TODO: delete cloned asset on usage end?

        /// <summary>
        /// Gets the original asset. Note: <see cref="AssetEditorWindowBase{T}.Asset"/> is the cloned asset for local editing. Use <see cref="SaveToOriginal"/> to apply changes to the original asset.
        /// </summary>
        public T OriginalAsset => (T)FlaxEngine.Content.Load(_item.ID);

        /// <inheritdoc />
        protected ClonedAssetEditorWindowBase(Editor editor, AssetItem item)
        : base(editor, item)
        {
        }

        /// <summary>
        /// Saves the copy of the asset to the original location. This action cannot be undone!
        /// </summary>
        /// <returns>True if failed, otherwise false.</returns>
        protected virtual bool SaveToOriginal()
        {
            // Wait until temporary asset file be fully loaded
            if (_asset.WaitForLoaded())
            {
                Editor.LogError(string.Format("Cannot save asset {0}. Wait for temporary asset loaded failed.", _item.Path));
                return true;
            }

            // Cache data
            var id = _item.ID;
            var sourcePath = _asset.Path;
            var destinationPath = _item.Path;

            // Check if original asset is loaded
            var originalAsset = (T)FlaxEngine.Content.GetAsset(id);
            if (originalAsset)
            {
                // Wait for loaded to prevent any issues
                if (!originalAsset.IsLoaded && originalAsset.LastLoadFailed)
                {
                    Editor.LogWarning(string.Format("Copying asset \'{0}\' to \'{1}\' (last load failed)", sourcePath, destinationPath));
                }
                else if (originalAsset.WaitForLoaded())
                {
                    Editor.LogError(string.Format("Cannot save asset {0}. Wait for original asset loaded failed.", _item.Path));
                    return true;
                }
            }

            // Copy temporary material to the final destination (and restore ID)
            if (Editor.ContentEditing.CloneAssetFile(sourcePath, destinationPath, id))
            {
                Editor.LogError(string.Format("Cannot copy asset \'{0}\' to \'{1}\'", sourcePath, destinationPath));
                return true;
            }

            // Reload original asset
            if (originalAsset)
            {
                originalAsset.Reload();
            }

            // Refresh thumbnail
            _item.RefreshThumbnail();

            return false;
        }

        /// <inheritdoc />
        protected override T LoadAsset()
        {
            // Clone asset
            if (Editor.ContentEditing.FastTempAssetClone(_item.Path, out var clonePath))
                return null;

            // Load cloned asset
            var asset = FlaxEngine.Content.LoadAsync<T>(clonePath);
            if (asset == null)
                return null;

            // Validate data
            if (asset.ID == _item.ID)
                throw new InvalidOperationException("Cloned asset has the same IDs.");

            return asset;
        }
    }
}
