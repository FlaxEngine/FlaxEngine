// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI.Docking
{
    /// <summary>
    /// Helper class used to handle docking windows dragging and docking.
    /// </summary>
    public class DockHintWindow
    {
        private FloatWindowDockPanel _toMove;

        private Float2 _dragOffset;
        private Float2 _defaultWindowSize;
        private Rectangle _rectDock;
        private Rectangle _rectWindow;
        private Float2 _mouse;
        private DockState _toSet;
        private DockPanel _toDock;
        private bool _lateDragOffsetUpdate;

        private Rectangle _rLeft, _rRight, _rBottom, _rUpper, _rCenter;

        private DockHintWindow(FloatWindowDockPanel toMove)
        {
            _toMove = toMove;
            _toSet = DockState.Float;
            var window = toMove.Window.Window;

            // Remove focus from drag target
            _toMove.Focus();
            _toMove.Defocus();

            // Focus window
            window.Focus();

            // Check if window is maximized and restore window.
            if (window.IsMaximized)
            {
                // Restore window and set position to mouse.
                var mousePos = window.MousePosition;
                var previousSize = window.Size;
                window.Restore();
                window.Position = Platform.MousePosition - mousePos * window.Size / previousSize;
            }

            // Calculate dragging offset and move window to the destination position
            var mouseScreenPosition = Platform.MousePosition;

            // If the _toMove window was not focused when initializing this window, the result vector only contains zeros
            // and to prevent a failure, we need to perform an update for the drag offset at later time which will be done in the OnMouseMove event handler.
            if (mouseScreenPosition != Float2.Zero)
                CalculateDragOffset(mouseScreenPosition);
            else
                _lateDragOffsetUpdate = true;

            // Get initial size
            _defaultWindowSize = window.Size;

            // Init proxy window
            Proxy.Init(ref _defaultWindowSize);

            // Bind events
            Proxy.Window.MouseUp += OnMouseUp;
            Proxy.Window.MouseMove += OnMouseMove;
            Proxy.Window.LostFocus += OnLostFocus;

            // Start tracking mouse
            Proxy.Window.StartTrackingMouse(false);

            // Update window GUI
            Proxy.Window.GUI.PerformLayout();

            // Update rectangles
            UpdateRects();

            // Hide base window
            window.Hide();

            // Enable hit window presentation
            Proxy.Window.RenderingEnabled = true;
            Proxy.Window.Show();
            Proxy.Window.Focus();
        }

        /// <summary>
        /// Releases unmanaged and - optionally - managed resources.
        /// </summary>
        public void Dispose()
        {
            // End tracking mouse
            Proxy.Window.EndTrackingMouse();

            // Disable rendering
            Proxy.Window.RenderingEnabled = false;

            // Unbind events
            Proxy.Window.MouseUp -= OnMouseUp;
            Proxy.Window.MouseMove -= OnMouseMove;
            Proxy.Window.LostFocus -= OnLostFocus;

            // Hide the proxy
            Proxy.Hide();

            if (_toMove == null)
                return;

            // Check if window won't be docked
            if (_toSet == DockState.Float)
            {
                var window = _toMove.Window?.Window;
                if (window == null)
                    return;
                var mouse = Platform.MousePosition;

                // Move base window
                window.Position = mouse - _dragOffset;

                // Show base window
                window.Show();
            }
            else
            {
                bool hasNoChildPanels = _toMove.ChildPanelsCount == 0;

                // Check if window has only single tab
                if (hasNoChildPanels && _toMove.TabsCount == 1)
                {
                    // Dock window
                    _toMove.GetTab(0).Show(_toSet, _toDock);
                }
                // Check if dock as tab and has no child panels
                else if (hasNoChildPanels && _toSet == DockState.DockFill)
                {
                    // Dock all tabs
                    while (_toMove.TabsCount > 0)
                    {
                        _toMove.GetTab(0).Show(DockState.DockFill, _toDock);
                    }
                }
                else
                {
                    var selectedTab = _toMove.SelectedTab;

                    // Dock the first tab into the target location
                    var firstTab = _toMove.GetTab(0);
                    firstTab.Show(_toSet, _toDock);

                    // Dock rest of the tabs
                    while (_toMove.TabsCount > 0)
                    {
                        _toMove.GetTab(0).Show(DockState.DockFill, firstTab);
                    }

                    // Keep selected tab being selected
                    selectedTab?.SelectTab();
                }

                // Focus target window
                _toDock.Root.Focus();
            }

            _toMove = null;
        }

        /// <summary>
        /// Creates the new dragging hit window.
        /// </summary>
        /// <param name="toMove">Floating dock panel to move.</param>
        /// <returns>The dock hint window object.</returns>
        public static DockHintWindow Create(FloatWindowDockPanel toMove)
        {
            if (toMove == null)
                throw new ArgumentNullException();

            return new DockHintWindow(toMove);
        }

        /// <summary>
        /// Creates the new dragging hit window.
        /// </summary>
        /// <param name="toMove">Dock window to move.</param>
        /// <returns>The dock hint window object.</returns>
        public static DockHintWindow Create(DockWindow toMove)
        {
            if (toMove == null)
                throw new ArgumentNullException();

            // Show floating
            toMove.ShowFloating();

            // Move window to the mouse position (with some offset for caption bar)
            var window = (WindowRootControl)toMove.Root;
            var mouse = Platform.MousePosition;
            window.Window.Position = mouse - new Float2(8, 8);

            // Get floating panel
            var floatingPanelToMove = window.GetChild(0) as FloatWindowDockPanel;

            return new DockHintWindow(floatingPanelToMove);
        }

        /// <summary>
        /// Calculates window rectangle in the dock window.
        /// </summary>
        /// <param name="state">Window dock state.</param>
        /// <param name="rect">Dock panel rectangle.</param>
        /// <returns>Calculated window rectangle.</returns>
        public static Rectangle CalculateDockRect(DockState state, ref Rectangle rect)
        {
            Rectangle result = rect;
            switch (state)
            {
            case DockState.DockFill:
                result.Location.Y += Editor.Instance.Options.Options.Interface.TabHeight;
                result.Size.Y -= Editor.Instance.Options.Options.Interface.TabHeight;
                break;
            case DockState.DockTop:
                result.Size.Y *= DockPanel.DefaultSplitterValue;
                break;
            case DockState.DockLeft:
                result.Size.X *= DockPanel.DefaultSplitterValue;
                break;
            case DockState.DockBottom:
                result.Location.Y += result.Size.Y * (1 - DockPanel.DefaultSplitterValue);
                result.Size.Y *= DockPanel.DefaultSplitterValue;
                break;
            case DockState.DockRight:
                result.Location.X += result.Size.X * (1 - DockPanel.DefaultSplitterValue);
                result.Size.X *= DockPanel.DefaultSplitterValue;
                break;
            }
            return result;
        }

        private void CalculateDragOffset(Float2 mouseScreenPosition)
        {
            var baseWinPos = _toMove.Window.Window.Position;
            _dragOffset = mouseScreenPosition - baseWinPos;
        }

        private void UpdateRects()
        {
            // Cache mouse position
            _mouse = Platform.MousePosition;

            // Check intersection with any dock panel
            var uiMouse = _mouse;
            _toDock = _toMove.MasterPanel.HitTest(ref uiMouse, _toMove);

            // Check dock state to use
            bool showProxyHints = _toDock != null;
            bool showBorderHints = showProxyHints;
            bool showCenterHint = showProxyHints;
            if (showProxyHints)
            {
                // If moved window has not only tabs but also child panels disable docking as tab
                if (_toMove.ChildPanelsCount > 0)
                    showCenterHint = false;

                // Disable docking windows with one or more dock panels inside
                if (_toMove.ChildPanelsCount > 0)
                    showBorderHints = false;

                // Get dock area
                _rectDock = _toDock.DockAreaBounds;

                // Cache dock rectangles
                var size = _rectDock.Size;
                var offset = _rectDock.Location;
                var borderMargin = 4.0f;
                var hintWindowsSize = Proxy.HintWindowsSize * Platform.DpiScale;
                var hintWindowsSize2 = hintWindowsSize * 0.5f;
                var centerX = size.X * 0.5f;
                var centerY = size.Y * 0.5f;
                _rUpper = new Rectangle(centerX - hintWindowsSize2, borderMargin, hintWindowsSize, hintWindowsSize) + offset;
                _rBottom = new Rectangle(centerX - hintWindowsSize2, size.Y - hintWindowsSize - borderMargin, hintWindowsSize, hintWindowsSize) + offset;
                _rLeft = new Rectangle(borderMargin, centerY - hintWindowsSize2, hintWindowsSize, hintWindowsSize) + offset;
                _rRight = new Rectangle(size.X - hintWindowsSize - borderMargin, centerY - hintWindowsSize2, hintWindowsSize, hintWindowsSize) + offset;
                _rCenter = new Rectangle(centerX - hintWindowsSize2, centerY - hintWindowsSize2, hintWindowsSize, hintWindowsSize) + offset;

                // Hit test
                DockState toSet = DockState.Float;
                if (showBorderHints)
                {
                    if (_rUpper.Contains(_mouse))
                        toSet = DockState.DockTop;
                    else if (_rBottom.Contains(_mouse))
                        toSet = DockState.DockBottom;
                    else if (_rLeft.Contains(_mouse))
                        toSet = DockState.DockLeft;
                    else if (_rRight.Contains(_mouse))
                        toSet = DockState.DockRight;
                }
                if (showCenterHint && _rCenter.Contains(_mouse))
                    toSet = DockState.DockFill;
                _toSet = toSet;

                // Show proxy hint windows
                Proxy.Down.Position = _rBottom.Location;
                Proxy.Left.Position = _rLeft.Location;
                Proxy.Right.Position = _rRight.Location;
                Proxy.Up.Position = _rUpper.Location;
                Proxy.Center.Position = _rCenter.Location;
            }
            else
            {
                _toSet = DockState.Float;
            }

            // Update proxy hint windows visibility
            Proxy.Down.IsVisible = showProxyHints & showBorderHints;
            Proxy.Left.IsVisible = showProxyHints & showBorderHints;
            Proxy.Right.IsVisible = showProxyHints & showBorderHints;
            Proxy.Up.IsVisible = showProxyHints & showBorderHints;
            Proxy.Center.IsVisible = showProxyHints & showCenterHint;

            // Calculate proxy/dock/window rectangles
            if (_toDock == null)
            {
                // Floating window over nothing
                _rectWindow = new Rectangle(_mouse - _dragOffset, _defaultWindowSize);
            }
            else
            {
                if (_toSet == DockState.Float)
                {
                    // Floating window over dock panel
                    _rectWindow = new Rectangle(_mouse - _dragOffset, _defaultWindowSize);
                }
                else
                {
                    // Use only part of the dock panel to show hint
                    _rectWindow = CalculateDockRect(_toSet, ref _rectDock);
                }
            }

            // Update proxy window
            Proxy.Window.ClientBounds = _rectWindow;
        }

        private void OnMouseUp(ref Float2 location, MouseButton button, ref bool handled)
        {
            if (button == MouseButton.Left)
            {
                Dispose();
            }
        }

        private void OnMouseMove(ref Float2 mousePos)
        {
            // Recalculate the drag offset because the current mouse screen position was invalid when we initialized the window
            if (_lateDragOffsetUpdate)
            {
                // Calculate dragging offset and move window to the destination position
                CalculateDragOffset(mousePos);

                // Reset state
                _lateDragOffsetUpdate = false;
            }

            UpdateRects();
        }

        private void OnLostFocus()
        {
            Dispose();
        }

        /// <summary>
        /// Contains helper proxy windows shared across docking panels. They are used to visualize docking window locations.
        /// </summary>
        public static class Proxy
        {
            /// <summary>
            /// The drag proxy window.
            /// </summary>
            public static Window Window;

            /// <summary>
            /// The left hint proxy window.
            /// </summary>
            public static Window Left;

            /// <summary>
            /// The right hint proxy window.
            /// </summary>
            public static Window Right;

            /// <summary>
            /// The up hint proxy window.
            /// </summary>
            public static Window Up;

            /// <summary>
            /// The down hint proxy window.
            /// </summary>
            public static Window Down;

            /// <summary>
            /// The center hint proxy window.
            /// </summary>
            public static Window Center;

            /// <summary>
            /// The hint windows size.
            /// </summary>
            public const float HintWindowsSize = 32.0f;

            /// <summary>
            /// Initializes the hit proxy windows. Those windows are used to indicate drag target areas (left, right, top, bottom, etc.).
            /// </summary>
            public static void InitHitProxy()
            {
                CreateProxy(ref Left, "DockHint.Left");
                CreateProxy(ref Right, "DockHint.Right");
                CreateProxy(ref Up, "DockHint.Up");
                CreateProxy(ref Down, "DockHint.Down");
                CreateProxy(ref Center, "DockHint.Center");
            }

            /// <summary>
            /// Initializes the hint window.
            /// </summary>
            /// <param name="initSize">Initial size of the proxy window.</param>
            public static void Init(ref Float2 initSize)
            {
                if (Window == null)
                {
                    var settings = CreateWindowSettings.Default;
                    settings.Title = "DockHint.Window";
                    settings.Size = initSize;
                    settings.AllowInput = true;
                    settings.AllowMaximize = false;
                    settings.AllowMinimize = false;
                    settings.HasBorder = false;
                    settings.HasSizingFrame = false;
                    settings.IsRegularWindow = false;
                    settings.SupportsTransparency = true;
                    settings.ShowInTaskbar = false;
                    settings.ShowAfterFirstPaint = false;
                    settings.IsTopmost = true;

                    Window = Platform.CreateWindow(ref settings);
                    Window.Opacity = 0.6f;
                    Window.GUI.BackgroundColor = Style.Current.Selection;
                    Window.GUI.AddChild<DragVisuals>();
                }
                else
                {
                    // Resize proxy
                    Window.ClientSize = initSize;
                }

                InitHitProxy();
            }

            private sealed class DragVisuals : Control
            {
                public DragVisuals()
                {
                    AnchorPreset = AnchorPresets.StretchAll;
                    Offsets = Margin.Zero;
                }

                public override void Draw()
                {
                    Render2D.DrawRectangle(new Rectangle(Float2.Zero, Size), Style.Current.SelectionBorder);
                }
            }

            private static void CreateProxy(ref Window win, string name)
            {
                if (win != null)
                    return;

                var settings = CreateWindowSettings.Default;
                settings.Title = name;
                settings.Size = new Float2(HintWindowsSize * Platform.DpiScale);
                settings.AllowInput = false;
                settings.AllowMaximize = false;
                settings.AllowMinimize = false;
                settings.HasBorder = false;
                settings.HasSizingFrame = false;
                settings.IsRegularWindow = false;
                settings.SupportsTransparency = true;
                settings.ShowInTaskbar = false;
                settings.ActivateWhenFirstShown = false;
                settings.IsTopmost = true;
                settings.ShowAfterFirstPaint = false;

                win = Platform.CreateWindow(ref settings);
                win.Opacity = 0.6f;
                win.GUI.BackgroundColor = Style.Current.Selection;
                win.GUI.AddChild<DragVisuals>();
            }

            /// <summary>
            /// Hides proxy windows.
            /// </summary>
            public static void Hide()
            {
                HideProxy(ref Window);
                HideProxy(ref Left);
                HideProxy(ref Right);
                HideProxy(ref Up);
                HideProxy(ref Down);
                HideProxy(ref Center);
            }

            private static void HideProxy(ref Window win)
            {
                if (win)
                {
                    win.Hide();
                }
            }

            /// <summary>
            /// Releases proxy data and windows.
            /// </summary>
            public static void Dispose()
            {
                DisposeProxy(ref Window);
                DisposeProxy(ref Left);
                DisposeProxy(ref Right);
                DisposeProxy(ref Up);
                DisposeProxy(ref Down);
                DisposeProxy(ref Center);
            }

            private static void DisposeProxy(ref Window win)
            {
                if (win)
                {
                    win.Close(ClosingReason.User);
                    win = null;
                }
            }
        }
    }
}
