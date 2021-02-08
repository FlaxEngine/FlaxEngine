// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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
        /// Show context menu over given control.
        /// </summary>
        /// <param name="parent">Parent control to attach to it.</param>
        /// <param name="location">Popup menu origin location in parent control coordinates.</param>
        public virtual void Show(Control parent, Vector2 location)
        {
            Assert.IsNotNull(parent);

            // Ensure to be closed
            Hide();

            // Peek parent control window
            var parentWin = parent.RootWindow;
            if (parentWin == null)
                return;

            // Check if show menu inside the other menu - then link as a child to prevent closing the calling menu window on lost focus
            if (_parentCM == null && parentWin.ChildrenCount == 1 && parentWin.Children[0] is ContextMenuBase parentCM)
            {
                parentCM.ShowChild(this, parentCM.PointFromScreen(parent.PointToScreen(location)), false);
                return;
            }

            // Unlock and perform controls update
            UnlockChildrenRecursive();
            PerformLayout();

            // Calculate popup direction and initial location (fit on a single monitor)
            var dpiScale = parentWin.DpiScale;
            Vector2 dpiSize = Size * dpiScale;
            Vector2 locationWS = parent.PointToWindow(location);
            Vector2 locationSS = parentWin.PointToScreen(locationWS);
            Location = Vector2.Zero;
            Rectangle monitorBounds = Platform.GetMonitorBounds(locationSS);
            Vector2 rightBottomLocationSS = locationSS + dpiSize;
            bool isUp = false, isLeft = false;
            if (monitorBounds.Bottom < rightBottomLocationSS.Y)
            {
                // Direction: up
                isUp = true;
                locationSS.Y -= dpiSize.Y;

                // Offset to fix sub-menu location
                if (parent is ContextMenu menu && menu._childCM != null)
                    locationSS.Y += 30.0f * dpiScale;
            }
            if (monitorBounds.Right < rightBottomLocationSS.X)
            {
                // Direction: left
                isLeft = true;
                locationSS.X -= dpiSize.X;
            }

            // Update direction flag
            if (isUp)
                _direction = isLeft ? ContextMenuDirection.LeftUp : ContextMenuDirection.RightUp;
            else
                _direction = isLeft ? ContextMenuDirection.LeftDown : ContextMenuDirection.RightDown;

            // Create window
            var desc = CreateWindowSettings.Default;
            desc.Position = locationSS;
            desc.StartPosition = WindowStartPosition.Manual;
            desc.Size = dpiSize;
            desc.Fullscreen = false;
            desc.HasBorder = false;
            desc.SupportsTransparency = false;
            desc.ShowInTaskbar = false;
            desc.ActivateWhenFirstShown = true;
            desc.AllowInput = true;
            desc.AllowMinimize = false;
            desc.AllowMaximize = false;
            desc.AllowDragAndDrop = false;
            desc.IsTopmost = true;
            desc.IsRegularWindow = false;
            desc.HasSizingFrame = false;
            OnWindowCreating(ref desc);
            _window = Platform.CreateWindow(ref desc);
            _window.LostFocus += OnWindowLostFocus;

            // Attach to the window
            _parentCM = parent as ContextMenuBase;
            Parent = _window.GUI;

            // Show
            Visible = true;
            if (_window == null)
                return;
            _window.Show();
            PerformLayout();
            _previouslyFocused = parentWin.FocusedControl;
            Focus();
            OnShow();
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
        public void ShowChild(ContextMenuBase child, Vector2 location, bool isSubMenu = true)
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

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            base.Update(deltaTime);

            // Let root context menu to check if none of the popup windows
            if (_parentCM == null)
            {
                var anyForeground = false;
                var c = this;
                while (!anyForeground && c != null)
                {
                    if (c._window != null && c._window.IsForegroundWindow)
                        anyForeground = true;
                    c = c._childCM;
                }
                if (!anyForeground)
                {
                    Hide();
                }
            }
        }

        /// <inheritdoc />
        public override void Draw()
        {
            // Draw background
            var style = Style.Current;
            var bounds = new Rectangle(Vector2.Zero, Size);
            Render2D.FillRectangle(bounds, style.Background);
            Render2D.DrawRectangle(bounds, Color.Lerp(style.BackgroundSelected, style.Background, 0.6f));

            base.Draw();
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Vector2 location, MouseButton button)
        {
            base.OnMouseDown(location, button);
            return true;
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Vector2 location, MouseButton button)
        {
            base.OnMouseUp(location, button);
            return true;
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
