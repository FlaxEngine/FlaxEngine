// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Content;
using FlaxEngine;
using FlaxEngine.GUI;
using DockWindow = FlaxEditor.GUI.Docking.DockWindow;

namespace FlaxEditor.Windows
{
    /// <summary>
    ///  Base class for all windows in Editor.
    /// </summary>
    /// <seealso cref="DockWindow" />
    public abstract class EditorWindow : DockWindow
    {
        /// <summary>
        /// Gets the editor object.
        /// </summary>
        public readonly Editor Editor;

        /// <summary>
        /// Gets a value indicating whether this window can open content finder popup.
        /// </summary>
        protected virtual bool CanOpenContentFinder => true;

        /// <summary>
        /// Gets a value indicating whether this window can use UI navigation (tab/enter).
        /// </summary>
        protected virtual bool CanUseNavigation => true;

        /// <summary>
        /// Initializes a new instance of the <see cref="EditorWindow"/> class.
        /// </summary>
        /// <param name="editor">The editor.</param>
        /// <param name="hideOnClose">True if hide window on closing, otherwise it will be destroyed.</param>
        /// <param name="scrollBars">The scroll bars.</param>
        protected EditorWindow(Editor editor, bool hideOnClose, ScrollBars scrollBars)
        : base(editor.UI.MasterPanel, hideOnClose, scrollBars)
        {
            AutoFocus = true;
            Editor = editor;

            InputActions.Add(options => options.ContentFinder, () =>
            {
                if (CanOpenContentFinder)
                {
                    Editor.ContentFinding.ShowFinder(RootWindow);
                }
            });

            // Register
            Editor.Windows.OnWindowAdd(this);
        }

        /// <summary>
        /// Determines whether this window is holding reference to the specified item.
        /// </summary>
        /// <param name="item">The item.</param>
        /// <returns><c>true</c> if window is editing the specified item; otherwise, <c>false</c>.</returns>
        public virtual bool IsEditingItem(ContentItem item)
        {
            return false;
        }

        #region Window Events

        /// <summary>
        /// Fired when scene starts saving
        /// </summary>
        /// <param name="scene">The scene object. It may be null!</param>
        /// <param name="sceneId">The scene ID.</param>
        public virtual void OnSceneSaving(Scene scene, Guid sceneId)
        {
        }

        /// <summary>
        /// Fired when scene gets saved
        /// </summary>
        /// <param name="scene">The scene object. It may be null!</param>
        /// <param name="sceneId">The scene ID.</param>
        public virtual void OnSceneSaved(Scene scene, Guid sceneId)
        {
        }

        /// <summary>
        /// Fired when scene gets saving error
        /// </summary>
        /// <param name="scene">The scene object. It may be null!</param>
        /// <param name="sceneId">The scene ID.</param>
        public virtual void OnSceneSaveError(Scene scene, Guid sceneId)
        {
        }

        /// <summary>
        /// Fired when scene starts loading
        /// </summary>
        /// <param name="scene">The scene object. It may be null!</param>
        /// <param name="sceneId">The scene ID.</param>
        public virtual void OnSceneLoading(Scene scene, Guid sceneId)
        {
        }

        /// <summary>
        /// Fired when scene gets loaded
        /// </summary>
        /// <param name="scene">The scene object. It may be null!</param>
        /// <param name="sceneId">The scene ID.</param>
        public virtual void OnSceneLoaded(Scene scene, Guid sceneId)
        {
        }

        /// <summary>
        /// Fired when scene cannot be loaded
        /// </summary>
        /// <param name="scene">The scene object. It may be null!</param>
        /// <param name="sceneId">The scene ID.</param>
        public virtual void OnSceneLoadError(Scene scene, Guid sceneId)
        {
        }

        /// <summary>
        /// Fired when scene gets unloading
        /// </summary>
        /// <param name="scene">The scene object. It may be null!</param>
        /// <param name="sceneId">The scene ID.</param>
        public virtual void OnSceneUnloading(Scene scene, Guid sceneId)
        {
        }

        /// <summary>
        /// Fired when scene gets unloaded
        /// </summary>
        /// <param name="scene">The scene object. It may be null!</param>
        /// <param name="sceneId">The scene ID.</param>
        public virtual void OnSceneUnloaded(Scene scene, Guid sceneId)
        {
        }

        /// <summary>
        /// Called before Editor will enter play mode.
        /// </summary>
        public virtual void OnPlayBeginning()
        {
        }

        /// <summary>
        /// Called when Editor is entering play mode.
        /// </summary>
        public virtual void OnPlayBegin()
        {
        }

        /// <summary>
        /// Called when Editor leaves the play mode.
        /// </summary>
        public virtual void OnPlayEnd()
        {
        }

        /// <summary>
        /// Called when window should be initialized.
        /// At this point, main window, content database, default editor windows are ready.
        /// </summary>
        public virtual void OnInit()
        {
        }

        /// <summary>
        /// Called when every engine update.
        /// Note: <see cref="Control.Update"/> may be called at the lower frequency than the engine updates.
        /// </summary>
        public virtual void OnUpdate()
        {
        }

        /// <summary>
        /// Called when editor is being closed and window should perform release data operations.
        /// </summary>
        public virtual void OnExit()
        {
        }

        /// <summary>
        /// Called when Editor state gets changed.
        /// </summary>
        public virtual void OnEditorStateChanged()
        {
        }

        #endregion

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            // Prevent closing the editor window when using RMB + Ctrl + W to slow down the camera flight
            if (Editor.Options.Options.Input.CloseTab.Process(this, key))
            {
                if (Root.GetMouseButton(MouseButton.Right))
                    return true;
            }

            if (base.OnKeyDown(key))
                return true;

            switch (key)
            {
            case KeyboardKeys.Return:
                if (CanUseNavigation && Root?.FocusedControl != null)
                {
                    Root.SubmitFocused();
                    return true;
                }
                break;
            case KeyboardKeys.Tab:
                if (CanUseNavigation && Root != null)
                {
                    bool shiftDown = Root.GetKey(KeyboardKeys.Shift);
                    Root.Navigate(shiftDown ? NavDirection.Previous : NavDirection.Next);
                    return true;
                }
                break;
            }
            return false;
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            if (IsDisposing)
                return;
            OnExit();

            // Unregister
            Editor.Windows.OnWindowRemove(this);

            base.OnDestroy();
        }
    }
}
