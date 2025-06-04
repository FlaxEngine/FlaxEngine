#if PLATFORM_WINDOWS
#define USE_IS_FOREGROUND
#else
#endif
// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using FlaxEngine;
using FlaxEngine.Assertions;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI.ContextMenu
{
    /// <summary>
    /// Context menu popup directions.
    /// </summary>
    [HideInEditor]
    public enum ContextMenuDirection
    {
        /// <summary>
        /// The right down.
        /// </summary>
        RightDown,

        /// <summary>
        /// The right up.
        /// </summary>
        RightUp,

        /// <summary>
        /// The left down.
        /// </summary>
        LeftDown,

        /// <summary>
        /// The left up.
        /// </summary>
        LeftUp,
    }

    /// <summary>
    /// Base class for all context menu controls.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    [HideInEditor]
    public class ContextMenuBase : ContainerControl
    {
        private ContextMenuDirection _direction;
        private ContextMenuBase _parentCM;
        private bool _isSubMenu;
        private ContextMenuBase _childCM;
        private Window _window;
        private Control _previouslyFocused;

        /// <summary>
        /// Gets a value indicating whether use automatic popup direction fix based on the screen dimensions.
        /// </summary>
        protected virtual bool UseAutomaticDirectionFix => true;

        /// <summary>
        /// Returns true if context menu is opened
        /// </summary>
        public bool IsOpened => Parent != null;

        /// <summary>
        /// Gets the popup direction.
        /// </summary>
        public ContextMenuDirection Direction => _direction;

        /// <summary>
        /// Gets a value indicating whether any child context menu has been opened.
        /// </summary>
        public bool HasChildCMOpened => _childCM != null;

        /// <summary>
        /// Gets the topmost context menu.
        /// </summary>
        public ContextMenuBase TopmostCM
        {
            get
            {
                var cm = this;
                while (cm._parentCM != null && cm._isSubMenu)
                {
                    cm = cm._parentCM;
                }
                return cm;
            }
        }

        /// <summary>
        /// Gets a value indicating whether this context menu is a sub-menu. Sub menus are treated like child context menus of the other menu (eg. hierarchy).
        /// </summary>
        public bool IsSubMenu => _isSubMenu;

        /// <summary>
        /// External dialog popups opened within the context window (eg. color picker) that should preserve context menu visibility (prevent from closing context menu).
        /// </summary>
        public List<Window> ExternalPopups = new List<Window>();

        /// <summary>
        /// Optional flag that can disable popup visibility based on window focus and use external control via Hide.
        /// </summary>
        public bool UseVisibilityControl = true;

        /// <summary>
        /// Optional flag that can disable popup input capturing. Useful for transparent or visual-only popups.
        /// </summary>
        public bool UseInput = true;

        /// <summary>
        /// Initializes a new instance of the <see cref="ContextMenuBase"/> class.
        /// </summary>
        public ContextMenuBase()
        : base(0, 0, 120, 32)
        {
            _direction = ContextMenuDirection.RightDown;
            Visible = false;

            _isSubMenu = true;
        }

        /// <summary>
        /// Shows the empty menu popup o na screen.
        /// </summary>
        /// <param name="control">The target control.</param>
        /// <param name="area">The target control area to cover.</param>
        /// <returns>Created popup.</returns>
        public static ContextMenuBase ShowEmptyMenu(Control control, Rectangle area)
        {
            // Calculate the control size in the window space to handle scaled controls
            var upperLeft = control.PointToWindow(area.UpperLeft);
            var bottomRight = control.PointToWindow(area.BottomRight);
            var size = bottomRight - upperLeft;

            var popup = new ContextMenuBase();
            popup.Size = size;
            popup.Show(control, area.Location + new Float2(0, (size.Y - popup.Height) * 0.5f));
            return popup;
        }

        /// <summary>
        /// Show context menu over given control.
        /// </summary>
        /// <param name="parent">Parent control to attach to it.</param>
        /// <param name="location">Popup menu origin location in parent control coordinates.</param>
        /// <param name="direction">The custom popup direction. Null to use automatic direction.</param>
        public virtual void Show(Control parent, Float2 location, ContextMenuDirection? direction = null)
        {
            Assert.IsNotNull(parent);
            bool isAlreadyVisible = Visible && _window;
            if (!isAlreadyVisible)
                Hide();

            // Peek parent control window
            var parentWin = parent.RootWindow;
            if (parentWin == null)
            {
                Hide();
                return;
            }

            // Check if show menu inside the other menu - then link as a child to prevent closing the calling menu window on lost focus
            if (_parentCM == null && parentWin.ChildrenCount == 1 && parentWin.Children[0] is ContextMenuBase parentCM)
            {
                Hide();
                parentCM.ShowChild(this, parentCM.PointFromScreen(parent.PointToScreen(location)), false);
                return;
            }

            // Unlock and perform controls update
            Location = Float2.Zero;
            UnlockChildrenRecursive();
            PerformLayout();

            // Calculate popup direction and initial location (fit on a single monitor)
            var dpiScale = parentWin.DpiScale;
            var dpiSize = Size * dpiScale;
            var locationWS = parent.PointToWindow(location);
            var locationSS = parentWin.PointToScreen(locationWS);
            var monitorBounds = Platform.GetMonitorBounds(locationSS);
            var rightBottomLocationSS = locationSS + dpiSize;
            bool isUp = false, isLeft = false;
            if (UseAutomaticDirectionFix && direction == null)
            {
                var parentMenu = parent as ContextMenu;
                if (monitorBounds.Bottom < rightBottomLocationSS.Y)
                {
                    isUp = true;
                    locationSS.Y -= dpiSize.Y;
                    if (parentMenu != null && parentMenu._childCM != null)
                        locationSS.Y += 30.0f * dpiScale;
                }
                if (parentMenu == null)
                {
                    if (monitorBounds.Right < rightBottomLocationSS.X)
                    {
                        isLeft = true;
                        locationSS.X -= dpiSize.X;
                    }
                }
                else if (monitorBounds.Right < rightBottomLocationSS.X || _parentCM?.Direction == ContextMenuDirection.LeftDown || _parentCM?.Direction == ContextMenuDirection.LeftUp)
                {
                    isLeft = true;
                    if (IsSubMenu && _parentCM != null)
                        locationSS.X -= _parentCM.Width + dpiSize.X;
                    else
                        locationSS.X -= dpiSize.X;
                }
            }
            else if (direction.HasValue)
            {
                switch (direction.Value)
                {
                case ContextMenuDirection.RightUp:
                    isUp = true;
                    break;
                case ContextMenuDirection.LeftDown:
                    isLeft = true;
                    break;
                case ContextMenuDirection.LeftUp:
                    isLeft = true;
                    isUp = true;
                    break;
                }
                if (isLeft)
                    locationSS.X -= dpiSize.X;
                if (isUp)
                    locationSS.Y -= dpiSize.Y;
            }

            // Update direction flag
            if (isUp)
                _direction = isLeft ? ContextMenuDirection.LeftUp : ContextMenuDirection.RightUp;
            else
                _direction = isLeft ? ContextMenuDirection.LeftDown : ContextMenuDirection.RightDown;

            if (isAlreadyVisible)
            {
                _window.ClientBounds = new Rectangle(locationSS, dpiSize);
            }
            else
            {
                // Create window
                var desc = CreateWindowSettings.Default;
                desc.Position = locationSS;
                desc.StartPosition = WindowStartPosition.Manual;
                desc.Size = dpiSize;
                desc.Fullscreen = false;
                desc.HasBorder = false;
                desc.SupportsTransparency = false;
                desc.ShowInTaskbar = false;
                desc.ActivateWhenFirstShown = UseInput;
                desc.AllowInput = UseInput;
                desc.AllowMinimize = false;
                desc.AllowMaximize = false;
                desc.AllowDragAndDrop = false;
                desc.IsTopmost = true;
                desc.IsRegularWindow = false;
                desc.HasSizingFrame = false;
                OnWindowCreating(ref desc);
                _window = Platform.CreateWindow(ref desc);
                if (UseVisibilityControl)
                {
                    _window.GotFocus += OnWindowGotFocus;
                    _window.LostFocus += OnWindowLostFocus;
                }

                // Attach to the window
                _parentCM = parent as ContextMenuBase;
                Parent = _window.GUI;

                // Show
                Visible = true;
                if (_window == null)
                    return;
                _window.Show();
            }
            PerformLayout();
            if (UseVisibilityControl)
            {
                _previouslyFocused = parentWin.FocusedControl;
                Focus();
                OnShow();
            }
        }

        private static void ForceDefocus(ContainerControl c)
        {
            foreach (var cc in c.Children)
            {
                if (cc.ContainsFocus)
                    cc.Defocus();
                if (cc is ContainerControl ccc)
                    ForceDefocus(ccc);
            }
        }

        /// <summary>
        /// Hide popup menu and all child menus.
        /// </summary>
        public virtual void Hide()
        {
            if (!Visible)
                return;

            // Lock update
            IsLayoutLocked = true;

            // Close child
            HideChild();

            // Force defocus
            ForceDefocus(this);

            // Unlink from window
            Parent = null;

            // Close window
            if (_window != null)
            {
                var win = _window;
                _window = null;
                win.Close();
            }

            // Unlink from parent
            if (_parentCM != null)
            {
                _parentCM._childCM = null;
                _parentCM = null;
            }

            // Return focus
            if (_previouslyFocused != null)
            {
                _previouslyFocused.RootWindow?.Focus();
                _previouslyFocused.Focus();
                _previouslyFocused = null;
            }

            // Hide
            Visible = false;
            OnHide();
        }

        /// <summary>
        /// Shows new child context menu.
        /// </summary>
        /// <param name="child">The child menu.</param>
        /// <param name="location">The child menu initial location.</param>
        /// <param name="isSubMenu">True if context menu is a normal sub-menu, otherwise it is a custom menu popup linked as child.</param>
        public void ShowChild(ContextMenuBase child, Float2 location, bool isSubMenu = true)
        {
            // Hide current child
            HideChild();

            // Set child
            _childCM = child;
            _childCM._parentCM = this;
            _childCM._isSubMenu = isSubMenu;

            // Show child
            _childCM.Show(this, location);
        }

        /// <summary>
        /// Hides child popup menu if any opened.
        /// </summary>
        public void HideChild()
        {
            if (_childCM != null)
            {
                _childCM.Hide();
                _childCM = null;
            }
        }

        /// <summary>
        /// Updates the size of the window to match context menu dimensions.
        /// </summary>
        protected void UpdateWindowSize()
        {
            if (_window != null)
            {
                _window.ClientSize = Size * _window.DpiScale;
            }
        }

        /// <summary>
        /// Called when context menu window setup is performed. Can be used to adjust the popup window options.
        /// </summary>
        /// <param name="settings">The settings.</param>
        protected virtual void OnWindowCreating(ref CreateWindowSettings settings)
        {
        }

        /// <summary>
        /// Called on context menu show.
        /// </summary>
        protected virtual void OnShow()
        {
            // Nothing to do
        }

        /// <summary>
        /// Called on context menu hide.
        /// </summary>
        protected virtual void OnHide()
        {
            // Nothing to do
        }

#if USE_IS_FOREGROUND
        /// <summary>
        /// Returns true if context menu is in foreground (eg. context window or any child window has user focus or user opened additional popup within this context).
        /// </summary>
        protected virtual bool IsForeground
        {
            get
            {
                // Any external popup is focused
                foreach (var externalPopup in ExternalPopups)
                {
                    if (externalPopup && externalPopup.IsForegroundWindow)
                        return true;
                }

                // Any context menu window is focused
                var anyForeground = false;
                var c = this;
                while (!anyForeground && c != null)
                {
                    if (c._window != null && c._window.IsForegroundWindow)
                        anyForeground = true;
                    c = c._childCM;
                }

                return anyForeground;
            }
        }

        private void OnWindowGotFocus()
        {
            var child = _childCM;
            if (child != null && _window && _window.IsForegroundWindow)
            {
                // Hide child if user clicked over parent (do it next frame to process other events before - eg. child windows focus loss)
                FlaxEngine.Scripting.InvokeOnUpdate(() =>
                {
                    if (child == _childCM)
                        HideChild();
                });
            }
        }

        private void OnWindowLostFocus()
        {
            // Skip for parent menus (child should handle lost of focus)
            if (_childCM != null)
                return;

            // Check if user stopped using that popup menu
            if (_parentCM != null)
            {
                // Focus parent if user clicked over the parent popup
                var mouse = _parentCM.PointFromScreen(FlaxEngine.Input.MouseScreenPosition);
                if (_parentCM.ContainsPoint(ref mouse))
                {
                    _parentCM._window.Focus();
                }
            }
        }
#else
        private void OnWindowGotFocus()
        {
        }

        private void OnWindowLostFocus()
        {
            // Skip for parent menus (child should handle lost of focus)
            if (_childCM != null)
                return;

            if (_parentCM != null)
            {
                if (IsMouseOver)
                    return;

                // Check if any external popup is focused
                foreach (var externalPopup in ExternalPopups)
                {
                    if (externalPopup && externalPopup.IsFocused)
                        return;
                }

                // Check if mouse is over any of the parents
                ContextMenuBase focusCM = null;
                var cm = _parentCM;
                while (cm != null)
                {
                    if (cm.IsMouseOver)
                        focusCM = cm;
                    cm = cm._parentCM;
                }

                if (focusCM != null)
                {
                    // Focus on the clicked parent and hide any open sub-menus
                    focusCM.HideChild();
                    focusCM._window?.Focus();
                }
                else
                {
                    // User clicked outside the context menus, hide the whole context menu tree
                    TopmostCM.Hide();
                }
            }
            else if (!IsMouseOver)
            {
                Hide();
            }
        }
#endif

        /// <inheritdoc />
        public override bool IsMouseOver
        {
            get
            {
                bool result = false;
                for (int i = 0; i < _children.Count; i++)
                {
                    var c = _children[i];
                    if (c.Visible && c.IsMouseOver)
                    {
                        result = true;
                        break;
                    }
                }
                return result;
            }
        }

#if USE_IS_FOREGROUND
        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            base.Update(deltaTime);

            // Let root context menu to check if none of the popup windows
            if (_parentCM == null && UseVisibilityControl && !IsForeground)
            {
                Hide();
            }
        }
#endif

        /// <inheritdoc />
        public override void Draw()
        {
            // Draw background
            var style = Style.Current;
            var bounds = new Rectangle(Float2.Zero, Size);
            Render2D.FillRectangle(bounds, style.Background);
            Render2D.DrawRectangle(bounds, Color.Lerp(style.BackgroundSelected, style.Background, 0.6f));

            base.Draw();
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            base.OnMouseDown(location, button);
            return true;
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            base.OnMouseUp(location, button);
            return true;
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            if (base.OnKeyDown(key))
                return true;

            switch (key)
            {
            case KeyboardKeys.Escape:
                Hide();
                return true;
            }
            return false;
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            // Ensure to be hidden
            Hide();

            base.OnDestroy();
        }
    }
}
