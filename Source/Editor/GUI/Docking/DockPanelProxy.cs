// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using FlaxEditor.GUI.ContextMenu;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI.Docking
{
    /// <summary>
    /// Proxy control used for docking <see cref="DockWindow"/> inside <see cref="DockPanel"/>.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    public class DockPanelProxy : ContainerControl
    {
        private DockPanel _panel;

        /// <summary>
        /// The is mouse down flag (left button).
        /// </summary>
        public bool IsMouseLeftButtonDown;

        /// <summary>
        /// The is mouse down flag (right button).
        /// </summary>
        public bool IsMouseRightButtonDown;

        /// <summary>
        /// The is mouse down flag (middle button).
        /// </summary>
        public bool IsMouseMiddleButtonDown;

        /// <summary>
        /// The is mouse down over cross button flag.
        /// </summary>
        public bool IsMouseDownOverCross;

        /// <summary>
        /// The mouse down window.
        /// </summary>
        public DockWindow MouseDownWindow;

        /// <summary>
        /// The mouse position.
        /// </summary>
        public Vector2 MousePosition;

        /// <summary>
        /// The start drag asynchronous window.
        /// </summary>
        public DockWindow StartDragAsyncWindow;

        private Rectangle HeaderRectangle => new Rectangle(0, 0, Width, DockPanel.DefaultHeaderHeight);

        /// <summary>
        /// Initializes a new instance of the <see cref="DockPanelProxy"/> class.
        /// </summary>
        /// <param name="panel">The panel.</param>
        internal DockPanelProxy(DockPanel panel)
        : base(0, 0, 64, 64)
        {
            AutoFocus = false;

            _panel = panel;
            AnchorPreset = AnchorPresets.StretchAll;
            Offsets = Margin.Zero;
        }

        private DockWindow GetTabAtPos(Vector2 position, out bool closeButton)
        {
            DockWindow result = null;
            closeButton = false;

            var tabsCount = _panel.TabsCount;
            if (tabsCount == 1)
            {
                var crossRect = new Rectangle(Width - DockPanel.DefaultButtonsSize - DockPanel.DefaultButtonsMargin, (DockPanel.DefaultHeaderHeight - DockPanel.DefaultButtonsSize) / 2, DockPanel.DefaultButtonsSize, DockPanel.DefaultButtonsSize);
                if (HeaderRectangle.Contains(position))
                {
                    closeButton = crossRect.Contains(position);
                    result = _panel.GetTab(0);
                }
            }
            else
            {
                float x = 0;
                for (int i = 0; i < tabsCount; i++)
                {
                    var tab = _panel.GetTab(i);
                    var titleSize = tab.TitleSize;
                    var iconWidth = tab.Icon.IsValid ? DockPanel.DefaultButtonsSize + DockPanel.DefaultLeftTextMargin : 0;
                    var width = titleSize.X + DockPanel.DefaultButtonsSize + 2 * DockPanel.DefaultButtonsMargin + DockPanel.DefaultLeftTextMargin + DockPanel.DefaultRightTextMargin + iconWidth;
                    var tabRect = new Rectangle(x, 0, width, DockPanel.DefaultHeaderHeight);
                    var isMouseOver = tabRect.Contains(position);
                    if (isMouseOver)
                    {
                        var crossRect = new Rectangle(x + width - DockPanel.DefaultButtonsSize - DockPanel.DefaultButtonsMargin, (DockPanel.DefaultHeaderHeight - DockPanel.DefaultButtonsSize) / 2, DockPanel.DefaultButtonsSize, DockPanel.DefaultButtonsSize);
                        closeButton = crossRect.Contains(position);
                        result = tab;
                        break;
                    }
                    x += width;
                }
            }

            return result;
        }

        private void GetTabRect(DockWindow win, out Rectangle bounds)
        {
            FlaxEngine.Assertions.Assert.IsTrue(_panel.ContainsTab(win));

            var tabsCount = _panel.TabsCount;
            if (tabsCount == 1)
            {
                bounds = HeaderRectangle;
                return;
            }
            else
            {
                float x = 0;
                for (int i = 0; i < tabsCount; i++)
                {
                    var tab = _panel.GetTab(i);
                    var titleSize = tab.TitleSize;
                    float width = titleSize.X + DockPanel.DefaultButtonsSize + 2 * DockPanel.DefaultButtonsMargin + DockPanel.DefaultLeftTextMargin + DockPanel.DefaultRightTextMargin;
                    if (tab == win)
                    {
                        bounds = new Rectangle(x, 0, width, DockPanel.DefaultHeaderHeight);
                        return;
                    }
                    x += width;
                }
            }

            bounds = Rectangle.Empty;
        }

        private void StartDrag(DockWindow win)
        {
            // Clear cache
            MouseDownWindow = null;
            StartDragAsyncWindow = win;

            // Register for late update in an async manner (to prevent from crash due to changing UI structure on window undock)
            FlaxEngine.Scripting.LateUpdate += StartDragAsync;
        }

        private void StartDragAsync()
        {
            FlaxEngine.Scripting.LateUpdate -= StartDragAsync;

            if (StartDragAsyncWindow != null)
            {
                var win = StartDragAsyncWindow;
                StartDragAsyncWindow = null;

                // Check if has only one window docked and is floating
                if (_panel.ChildPanelsCount == 0 && _panel.TabsCount == 1 && _panel.IsFloating)
                {
                    // Create docking hint window but in an async manner
                    DockHintWindow.Create(_panel as FloatWindowDockPanel);
                }
                else
                {
                    // Select another tab
                    int index = _panel.GetTabIndex(win);
                    if (index == 0)
                        index = _panel.TabsCount;
                    _panel.SelectTab(index - 1);

                    // Create docking hint window
                    DockHintWindow.Create(win);
                }
            }
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            // Cache data
            var style = Style.Current;
            var window = Root;
            bool containsFocus = ContainsFocus && ((WindowRootControl)window).Window.IsFocused;
            var headerRect = HeaderRectangle;
            var tabsCount = _panel.TabsCount;

            // Check if has only one window docked
            if (tabsCount == 1)
            {
                var tab = _panel.GetTab(0);

                // Draw header
                bool isMouseOver = IsMouseOver && headerRect.Contains(MousePosition);
                Render2D.FillRectangle(headerRect, containsFocus ? style.BackgroundSelected : isMouseOver ? style.BackgroundHighlighted : style.LightBackground);

                float iconWidth = tab.Icon.IsValid ? DockPanel.DefaultButtonsSize + DockPanel.DefaultLeftTextMargin : 0;

                if (tab.Icon.IsValid)
                {
                    Render2D.DrawSprite(
                        tab.Icon,
                        new Rectangle(DockPanel.DefaultLeftTextMargin, (DockPanel.DefaultHeaderHeight - DockPanel.DefaultButtonsSize) / 2, DockPanel.DefaultButtonsSize, DockPanel.DefaultButtonsSize),
                        style.Foreground);

                }

                // Draw text
                Render2D.DrawText(
                    style.FontMedium,
                    tab.Title,
                    new Rectangle(DockPanel.DefaultLeftTextMargin + iconWidth, 0, Width - DockPanel.DefaultLeftTextMargin - DockPanel.DefaultButtonsSize - 2 * DockPanel.DefaultButtonsMargin, DockPanel.DefaultHeaderHeight),
                    style.Foreground,
                    TextAlignment.Near,
                    TextAlignment.Center);

                // Draw cross
                var crossRect = new Rectangle(Width - DockPanel.DefaultButtonsSize - DockPanel.DefaultButtonsMargin, (DockPanel.DefaultHeaderHeight - DockPanel.DefaultButtonsSize) / 2, DockPanel.DefaultButtonsSize, DockPanel.DefaultButtonsSize);
                bool isMouseOverCross = isMouseOver && crossRect.Contains(MousePosition);
                if (isMouseOverCross)
                    Render2D.FillRectangle(crossRect, (containsFocus ? style.BackgroundSelected : style.LightBackground) * 1.3f);
                Render2D.DrawSprite(style.Cross, crossRect, isMouseOverCross ? style.Foreground : style.ForegroundGrey);
            }
            else
            {
                // Draw background
                Render2D.FillRectangle(headerRect, style.LightBackground);

                // Render all tabs
                float x = 0;
                for (int i = 0; i < tabsCount; i++)
                {
                    // Cache data
                    var tab = _panel.GetTab(i);
                    var tabColor = Color.Black;
                    var titleSize = tab.TitleSize;
                    var iconWidth = tab.Icon.IsValid ? DockPanel.DefaultButtonsSize + DockPanel.DefaultLeftTextMargin : 0;
                    var width = titleSize.X + DockPanel.DefaultButtonsSize + 2 * DockPanel.DefaultButtonsMargin + DockPanel.DefaultLeftTextMargin + DockPanel.DefaultRightTextMargin + iconWidth;
                    var tabRect = new Rectangle(x, 0, width, DockPanel.DefaultHeaderHeight);
                    var isMouseOver = IsMouseOver && tabRect.Contains(MousePosition);
                    var isSelected = _panel.SelectedTab == tab;

                    // Check if tab is selected
                    if (isSelected)
                    {
                        tabColor = containsFocus ? style.BackgroundSelected : style.BackgroundNormal;
                        Render2D.FillRectangle(tabRect, tabColor);
                    }
                    // Check if mouse is over
                    else if (isMouseOver)
                    {
                        tabColor = style.BackgroundHighlighted;
                        Render2D.FillRectangle(tabRect, tabColor);
                    }

                    if (tab.Icon.IsValid)
                    {
                        Render2D.DrawSprite(
                            tab.Icon,
                            new Rectangle(x + DockPanel.DefaultLeftTextMargin, (DockPanel.DefaultHeaderHeight - DockPanel.DefaultButtonsSize) / 2, DockPanel.DefaultButtonsSize, DockPanel.DefaultButtonsSize),
                            style.Foreground);

                    }

                    // Draw text
                    Render2D.DrawText(
                        style.FontMedium,
                        tab.Title,
                        new Rectangle(x + DockPanel.DefaultLeftTextMargin + iconWidth, 0, 10000, DockPanel.DefaultHeaderHeight),
                        style.Foreground,
                        TextAlignment.Near,
                        TextAlignment.Center);

                    // Draw cross
                    if (isSelected || isMouseOver)
                    {
                        var crossRect = new Rectangle(x + width - DockPanel.DefaultButtonsSize - DockPanel.DefaultButtonsMargin, (DockPanel.DefaultHeaderHeight - DockPanel.DefaultButtonsSize) / 2, DockPanel.DefaultButtonsSize, DockPanel.DefaultButtonsSize);
                        bool isMouseOverCross = isMouseOver && crossRect.Contains(MousePosition);
                        if (isMouseOverCross)
                            Render2D.FillRectangle(crossRect, tabColor * 1.3f);
                        Render2D.DrawSprite(style.Cross, crossRect, isMouseOverCross ? style.Foreground : style.ForegroundGrey);
                    }

                    // Move
                    x += width;
                }

                // Draw selected tab strip
                Render2D.FillRectangle(new Rectangle(0, DockPanel.DefaultHeaderHeight - 2, Width, 2), containsFocus ? style.BackgroundSelected : style.BackgroundNormal);
            }
        }

        /// <inheritdoc />
        public override void OnLostFocus()
        {
            // Clear
            IsMouseLeftButtonDown = false;
            IsMouseRightButtonDown = false;
            IsMouseMiddleButtonDown = false;
            MouseDownWindow = null;

            base.OnLostFocus();
        }

        /// <inheritdoc />
        public override void OnMouseEnter(Vector2 location)
        {
            // Cache mouse
            MousePosition = location;

            base.OnMouseEnter(location);
        }

        /// <inheritdoc />
        public override bool OnMouseDoubleClick(Vector2 location, MouseButton button)
        {
            // Maximize/restore on double click
            var tab = GetTabAtPos(location, out _);
            var rootWindow = tab?.RootWindow;
            if (rootWindow != null && button == MouseButton.Left)
            {
                if (rootWindow.IsMaximized)
                    rootWindow.Restore();
                else
                    rootWindow.Maximize();
                return true;
            }

            return base.OnMouseDoubleClick(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Vector2 location, MouseButton button)
        {
            MouseDownWindow = GetTabAtPos(location, out IsMouseDownOverCross);

            // Check buttons
            if (button == MouseButton.Left)
            {
                // Cache data
                IsMouseLeftButtonDown = true;
                if (!IsMouseDownOverCross && MouseDownWindow != null)
                    _panel.SelectTab(MouseDownWindow);
            }
            else if (button == MouseButton.Right)
            {
                // Cache data
                IsMouseRightButtonDown = true;
                if (MouseDownWindow != null)
                    _panel.SelectTab(MouseDownWindow);
            }
            else if (button == MouseButton.Middle)
            {
                // Cache data
                IsMouseMiddleButtonDown = true;
            }

            return base.OnMouseDown(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Vector2 location, MouseButton button)
        {
            // Check tabs under mouse position at the beginning and at the end
            var tab = GetTabAtPos(location, out var overCross);

            // Check buttons
            if (button == MouseButton.Left && IsMouseLeftButtonDown)
            {
                // Clear flag
                IsMouseLeftButtonDown = false;

                // Check if tabs are the same and cross was pressed
                if (tab != null && tab == MouseDownWindow && IsMouseDownOverCross && overCross)
                    tab.Close(ClosingReason.User);
                MouseDownWindow = null;
            }
            else if (button == MouseButton.Right && IsMouseRightButtonDown)
            {
                // Clear flag
                IsMouseRightButtonDown = false;

                if (tab != null)
                {
                    ShowContextMenu(tab, ref location);
                }
            }
            else if (button == MouseButton.Middle && IsMouseMiddleButtonDown)
            {
                // Clear flag
                IsMouseMiddleButtonDown = false;

                if (tab != null)
                {
                    tab.Close(ClosingReason.User);
                }
            }

            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override void OnMouseMove(Vector2 location)
        {
            // Cache mouse
            MousePosition = location;

            // Check if mouse is down
            if (IsMouseLeftButtonDown)
            {
                // Check if mouse is outside the header
                if (!HeaderRectangle.Contains(location))
                {
                    // Clear flag
                    IsMouseLeftButtonDown = false;

                    // Check tab under the mouse
                    if (!IsMouseDownOverCross && MouseDownWindow != null)
                        StartDrag(MouseDownWindow);
                    MouseDownWindow = null;
                }
                // Check if has more than one tab to change order
                else if (MouseDownWindow != null && _panel.TabsCount > 1)
                {
                    // Check if mouse left current tab rect
                    GetTabRect(MouseDownWindow, out Rectangle currWinRect);
                    if (!currWinRect.Contains(location))
                    {
                        int index = _panel.GetTabIndex(MouseDownWindow);

                        // Check if move right or left
                        if (location.X < currWinRect.X)
                        {
                            // Move left
                            _panel.MoveTabLeft(index);
                        }
                        else if (_panel.LastTab != MouseDownWindow)
                        {
                            // Move right
                            _panel.MoveTabRight(index);
                        }

                        // Update
                        _panel.PerformLayout();
                    }
                }
            }

            base.OnMouseMove(location);
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            // Check if mouse is down
            if (IsMouseLeftButtonDown)
            {
                // Clear flag
                IsMouseLeftButtonDown = false;

                // Check tabs under mouse position
                if (!IsMouseDownOverCross && MouseDownWindow != null)
                    StartDrag(MouseDownWindow);
                MouseDownWindow = null;
            }
            IsMouseRightButtonDown = false;
            IsMouseMiddleButtonDown = false;

            base.OnMouseLeave();
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragEnter(ref Vector2 location, DragData data)
        {
            var result = base.OnDragEnter(ref location, data);
            if (result != DragDropEffect.None)
                return result;

            if (TrySelectTabUnderLocation(ref location))
                return DragDropEffect.Move;

            return DragDropEffect.None;
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragMove(ref Vector2 location, DragData data)
        {
            var result = base.OnDragMove(ref location, data);
            if (result != DragDropEffect.None)
                return result;

            if (TrySelectTabUnderLocation(ref location))
                return DragDropEffect.Move;

            return DragDropEffect.None;
        }

        /// <inheritdoc />
        public override void GetDesireClientArea(out Rectangle rect)
        {
            rect = new Rectangle(0, DockPanel.DefaultHeaderHeight, Width, Height - DockPanel.DefaultHeaderHeight);
        }

        private bool TrySelectTabUnderLocation(ref Vector2 location)
        {
            var tab = GetTabAtPos(location, out _);
            if (tab != null)
            {
                _panel.SelectTab(tab);
                Update(0); // Fake update
                return true;
            }

            return false;
        }

        private void ShowContextMenu(DockWindow tab, ref Vector2 location)
        {
            var menu = new ContextMenu.ContextMenu
            {
                Tag = tab
            };
            tab.OnShowContextMenu(menu);
            menu.AddButton("Close", OnTabMenuCloseClicked);
            menu.AddButton("Close All", OnTabMenuCloseAllClicked);
            menu.AddButton("Close All But This", OnTabMenuCloseAllButThisClicked);
            if (_panel.Tabs.IndexOf(tab) + 1 < _panel.TabsCount)
            {
                menu.AddButton("Close All To The Right", OnTabMenuCloseAllToTheRightClicked);
            }
            if (!_panel.IsFloating)
            {
                menu.AddSeparator();
                menu.AddButton("Undock", OnTabMenuUndockClicked);
            }
            else if (!tab.RootWindow?.IsMaximized ?? false)
            {
                menu.AddSeparator();
                menu.AddButton("Maximize", OnTabMenuMaximizeClicked);
            }
            else if (tab.RootWindow?.IsMaximized ?? false)
            {
                menu.AddSeparator();
                menu.AddButton("Restore", OnTabMenuRestoreClicked);
            }
            menu.Show(this, location);
        }

        private void OnTabMenuCloseClicked(ContextMenuButton button)
        {
            var tab = (DockWindow)button.ParentContextMenu.Tag;
            tab.Close(ClosingReason.User);
            if (tab == MouseDownWindow)
                MouseDownWindow = null;
        }

        private void OnTabMenuCloseAllClicked(ContextMenuButton button)
        {
            _panel.CloseAll();
        }

        private void OnTabMenuCloseAllButThisClicked(ContextMenuButton button)
        {
            var tab = (DockWindow)button.ParentContextMenu.Tag;
            var windows = _panel.Tabs.ToArray();
            for (int i = 0; i < windows.Length; i++)
            {
                if (windows[i] != tab)
                    windows[i].Close();
            }
        }

        private void OnTabMenuCloseAllToTheRightClicked(ContextMenuButton button)
        {
            var tab = (DockWindow)button.ParentContextMenu.Tag;
            var windows = _panel.Tabs.ToArray();
            for (int i = _panel.Tabs.IndexOf(tab) + 1; i < windows.Length; i++)
            {
                windows[i].Close();
            }
        }

        private void OnTabMenuUndockClicked(ContextMenuButton button)
        {
            var tab = (DockWindow)button.ParentContextMenu.Tag;
            tab.ShowFloating();
        }

        private void OnTabMenuMaximizeClicked(ContextMenuButton button)
        {
            var tab = (DockWindow)button.ParentContextMenu.Tag;
            tab.RootWindow.Maximize();
        }

        private void OnTabMenuRestoreClicked(ContextMenuButton button)
        {
            var tab = (DockWindow)button.ParentContextMenu.Tag;
            tab.RootWindow.Restore();
        }
    }
}
