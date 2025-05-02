// Copyright (c) Wojciech Figat. All rights reserved.

using System.Threading;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI.Dialogs
{
    /// <summary>
    /// Helper class for showing user dialogs.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    public abstract class Dialog : ContainerControl
    {
        private string _title;

        /// <summary>
        /// Flag used to block the calling thread if it used ShowDialog option.
        /// </summary>
        protected long _isWaitingForDialog;

        /// <summary>
        /// The parent window.
        /// </summary>
        protected volatile Window _window;

        /// <summary>
        /// The dialog result.
        /// </summary>
        protected DialogResult _result;

        /// <summary>
        /// The dialog size.
        /// </summary>
        protected Float2 _dialogSize = new Float2(300, 100);

        /// <summary>
        /// Gets the dialog result.
        /// </summary>
        public DialogResult Result => _result;

        /// <summary>
        /// Returns the size of the dialog.
        /// </summary>
        public Float2 DialogSize => _dialogSize;

        /// <summary>
        /// Initializes a new instance of the <see cref="Dialog"/> class.
        /// </summary>
        /// <param name="title">The title.</param>
        protected Dialog(string title)
        {
            BackgroundColor = Style.Current.Background;
            AnchorPreset = AnchorPresets.StretchAll;
            Offsets = Margin.Zero;
            ClipChildren = false;

            _title = title;
            _result = DialogResult.None;
        }

        /// <summary>
        /// Shows the dialog and waits for the result.
        /// </summary>
        /// <returns>The dialog result.</returns>
        public DialogResult ShowDialog()
        {
            return ShowDialog(Editor.Instance.Windows.MainWindow);
        }

        /// <summary>
        /// Shows the dialog and waits for the result.
        /// </summary>
        /// <param name="parentWindow">The parent window.</param>
        /// <returns>The dialog result.</returns>
        public DialogResult ShowDialog(WindowRootControl parentWindow)
        {
            return ShowDialog(parentWindow?.Window);
        }

        /// <summary>
        /// Shows the dialog and waits for the result.
        /// </summary>
        /// <param name="control">The control calling.</param>
        /// <returns>The dialog result.</returns>
        public DialogResult ShowDialog(Control control)
        {
            return ShowDialog(control?.RootWindow);
        }

        /// <summary>
        /// Shows the dialog and waits for the result.
        /// </summary>
        /// <param name="parentWindow">The parent window.</param>
        /// <returns>The dialog result.</returns>
        public DialogResult ShowDialog(Window parentWindow)
        {
            // Show window
            Show(parentWindow);

            // Set wait flag
            Interlocked.Increment(ref _isWaitingForDialog);

            // Wait for the closing
            do
            {
                Thread.Sleep(1);
            } while (_window);

            // Clear wait flag
            Interlocked.Decrement(ref _isWaitingForDialog);

            return _result;
        }

        /// <summary>
        /// Shows the dialog.
        /// </summary>
        public void Show()
        {
            Show(Editor.Instance.Windows.MainWindow);
        }

        /// <summary>
        /// Shows the dialog.
        /// </summary>
        /// <param name="parentWindow">The parent window.</param>
        public void Show(WindowRootControl parentWindow)
        {
            Show(parentWindow?.Window);
        }

        /// <summary>
        /// Shows the dialog.
        /// </summary>
        /// <param name="control">The control calling.</param>
        public void Show(Control control)
        {
            Show(control?.Root.RootWindow);
        }

        /// <summary>
        /// Shows the dialog.
        /// </summary>
        /// <param name="parentWindow">The parent window.</param>
        public void Show(Window parentWindow)
        {
            // Setup initial window settings
            CreateWindowSettings settings = CreateWindowSettings.Default;
            settings.Title = _title;
            settings.Size = _dialogSize * parentWindow.DpiScale;
            settings.AllowMaximize = false;
            settings.AllowMinimize = false;
            settings.HasSizingFrame = false;
            settings.StartPosition = WindowStartPosition.CenterParent;
            settings.Parent = parentWindow;
            SetupWindowSettings(ref settings);

            // Create window
            _window = Platform.CreateWindow(ref settings);
            var windowGUI = _window.GUI;

            // Attach events
            _window.Closing += OnClosing;
            _window.Closed += OnClosed;

            // Link to the window
            Offsets = Margin.Zero;
            Parent = windowGUI;

            // Show
            _window.Show();
            _window.Focus();
            _window.FlashWindow();

            // Perform layout
            windowGUI.UnlockChildrenRecursive();
            windowGUI.PerformLayout();

            OnShow();
        }

        private void OnClosing(ClosingReason reason, ref bool cancel)
        {
            // Check if can close window
            if (CanCloseWindow(reason))
            {
                if (reason == ClosingReason.User)
                    _result = DialogResult.Cancel;

                // Clean up
                _window = null;

                // Check if any thread is blocked during ShowDialog, then wait for it
                bool wait = true;
                while (wait)
                {
                    wait = Interlocked.Read(ref _isWaitingForDialog) > 0;
                    Thread.Sleep(1);
                }

                // Close window
                return;
            }

            // Suppress closing
            cancel = true;
        }

        private void OnClosed()
        {
            _window = null;
        }

        /// <summary>
        /// Closes this dialog.
        /// </summary>
        public void Close()
        {
            if (_window != null)
            {
                var win = _window;
                _window = null;
                win.Close();
            }
        }

        /// <summary>
        /// Called to cancel action and close the dialog.
        /// </summary>
        public virtual void OnCancel()
        {
            Close(DialogResult.Cancel);
        }

        /// <summary>
        /// Closes dialog with the specified result.
        /// </summary>
        /// <param name="result">The result.</param>
        protected void Close(DialogResult result)
        {
            _result = result;
            Close();
        }

        /// <summary>
        /// Setups the window settings.
        /// </summary>
        /// <param name="settings">The settings.</param>
        protected virtual void SetupWindowSettings(ref CreateWindowSettings settings)
        {
        }

        /// <summary>
        /// Called when dialogs popups.
        /// </summary>
        protected virtual void OnShow()
        {
            Focus();
        }

        /// <summary>
        /// Determines whether this dialog can be closed.
        /// </summary>
        /// <param name="reason">The reason.</param>
        /// <returns><c>true</c> if this dialog can be closed; otherwise, <c>false</c>.</returns>
        protected virtual bool CanCloseWindow(ClosingReason reason)
        {
            return true;
        }

        /// <inheritdoc />
        public override void OnSubmit()
        {
            base.OnSubmit();

            Close(DialogResult.OK);
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            if (base.OnKeyDown(key))
                return true;

            switch (key)
            {
            case KeyboardKeys.Return:
                if (Root?.FocusedControl != null)
                    Root.SubmitFocused();
                else
                    OnSubmit();
                return true;
            case KeyboardKeys.Escape:
                OnCancel();
                return true;
            case KeyboardKeys.Tab:
                if (Root != null)
                {
                    bool shiftDown = Root.GetKey(KeyboardKeys.Shift);
                    Root.Navigate(shiftDown ? NavDirection.Previous : NavDirection.Next);
                }
                return true;
            }
            return false;
        }
    }
}
