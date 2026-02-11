// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI.Docking
{
    /// <summary>
    /// Helper class used to handle docking windows dragging and docking.
    /// </summary>
    public class WindowDragHelper
    {
        private FloatWindowDockPanel _toMove;

        private Float2 _dragOffset;
        private Rectangle _rectDock;
        private Float2 _mouse;
        private DockState _toSet;
        private DockPanel _toDock;
        private Window _dragSourceWindow;

        private Rectangle _rLeft, _rRight, _rBottom, _rUpper, _rCenter;
        private Control _dockHintDown, _dockHintUp, _dockHintLeft, _dockHintRight, _dockHintCenter;

        /// <summary>
        /// The hint control size.
        /// </summary>
        public const float HintControlSize = 32.0f;

        /// <summary>
        /// The opacity of the dragged window when hint controls are shown.
        /// </summary>
        public const float DragWindowOpacity = 0.4f;
        
        /// <summary>
        /// Returns true if any windows are being dragged.
        /// </summary>
        public static bool IsDragActive { get; private set; }

        private WindowDragHelper(FloatWindowDockPanel toMove, Window dragSourceWindow)
        {
            IsDragActive = true;
            _toMove = toMove;
            _toSet = DockState.Float;
            var window = toMove.Window.Window;
            var mousePos = Platform.MousePosition;

            // Check if window is maximized and restore window for correct dragging
            if (window.IsMaximized)
            {
                var windowMousePos = mousePos - window.Position;
                var previousSize = window.Size;
                window.Restore();
                window.Position = mousePos - windowMousePos * window.Size / previousSize;
            }

            // When drag starts from a tabs the window might not be shown yet
            if (!window.IsVisible)
            {
                window.Show();
                window.Position = mousePos - new Float2(40, 10);
            }

            // Bind events
            FlaxEngine.Scripting.Update += OnUpdate;
            window.MouseUp += OnMouseUp;
#if !PLATFORM_SDL
            window.StartTrackingMouse(false);
#endif

            // Update rectangles
            UpdateRects(mousePos);
            
            // Ensure the dragged window stays on top of every other window
            window.IsAlwaysOnTop = true;

            _dragSourceWindow = dragSourceWindow;
            if (_dragSourceWindow != null) // Detaching a tab from existing window
            {
#if PLATFORM_SDL
                _dragOffset = new Float2(window.Size.X / 2, 10.0f);
#else
                _dragOffset = mousePos - window.Position;
#endif

                // The mouse up event is sent to the source window on Windows
                _dragSourceWindow.MouseUp += OnMouseUp;

                // TODO: when detaching tab in floating window (not main window), the drag source window is still main window?
                var dragSourceWindowWayland = toMove.MasterPanel?.RootWindow.Window ?? Editor.Instance.Windows.MainWindow;
                window.DoDragDrop(window.Title, _dragOffset, dragSourceWindowWayland);
#if !PLATFORM_SDL
                _dragSourceWindow.BringToFront();
#endif
            }
            else
            {
                _dragOffset = window.MousePosition;
                window.DoDragDrop(window.Title, _dragOffset, window);
            }
        }

        /// <summary>
        /// Releases unmanaged and - optionally - managed resources.
        /// </summary>
        public void Dispose()
        {
            IsDragActive = false;
            var window = _toMove?.Window?.Window;

            // Unbind events
            FlaxEngine.Scripting.Update -= OnUpdate;
            if (window != null)
            {
                window.MouseUp -= OnMouseUp;
#if !PLATFORM_SDL
                window.EndTrackingMouse();
#endif
            }
            if (_dragSourceWindow != null)
                _dragSourceWindow.MouseUp -= OnMouseUp;

            RemoveDockHints();

            if (_toMove == null)
                return;

            if (window != null)
            {
                window.Opacity = 1.0f;
                window.IsAlwaysOnTop = false;
                window.BringToFront();
            }

            // Check if window won't be docked
            if (_toSet == DockState.Float)
            {
                if (window == null)
                    return;

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
                    if (_toMove.TabsCount > 0)
                    {
                        var firstTab = _toMove.GetTab(0);
                        firstTab.Show(_toSet, _toDock);

                        // Dock rest of the tabs
                        while (_toMove.TabsCount > 0)
                        {
                            _toMove.GetTab(0).Show(DockState.DockFill, firstTab);
                        }
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
        /// Start dragging a floating dock panel.
        /// </summary>
        /// <param name="toMove">Floating dock panel to move.</param>
        /// <param name="dragSourceWindow">The window where dragging started from.</param>
        /// <returns>The window drag helper object.</returns>
        public static WindowDragHelper StartDragging(FloatWindowDockPanel toMove, Window dragSourceWindow = null)
        {
            if (toMove == null)
                throw new ArgumentNullException();

            return new WindowDragHelper(toMove, dragSourceWindow);
        }

        /// <summary>
        /// Start dragging a docked panel into a floating window.
        /// </summary>
        /// <param name="toMove">Dock window to move.</param>
        /// <param name="dragSourceWindow">The window where dragging started from.</param>
        /// <returns>The window drag helper object.</returns>
        public static WindowDragHelper StartDragging(DockWindow toMove, Window dragSourceWindow)
        {
            if (toMove == null)
                throw new ArgumentNullException();

            // Create floating window
            toMove.CreateFloating();

            // Get floating panel
            var window = (WindowRootControl)toMove.Root;
            var floatingPanelToMove = window.GetChild(0) as FloatWindowDockPanel;

            return new WindowDragHelper(floatingPanelToMove, dragSourceWindow);
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
                base.Draw();
                Render2D.DrawRectangle(new Rectangle(Float2.Zero, Size), Style.Current.SelectionBorder);
            }
        }

        private void AddDockHints()
        {
            if (_toDock == null)
                return;

            if (_toDock.RootWindow.Window != _dragSourceWindow)
                _toDock.RootWindow.Window.MouseUp += OnMouseUp;

            _dockHintDown = AddHintControl(new Float2(0.5f, 1));
            _dockHintUp = AddHintControl(new Float2(0.5f, 0));
            _dockHintLeft = AddHintControl(new Float2(0, 0.5f));
            _dockHintRight = AddHintControl(new Float2(1, 0.5f));
            _dockHintCenter = AddHintControl(new Float2(0.5f, 0.5f));

            Control AddHintControl(Float2 pivot)
            {
                DragVisuals hintControl = _toDock.AddChild<DragVisuals>();
                hintControl.Size = new Float2(HintControlSize);
                hintControl.BackgroundColor = Style.Current.Selection.AlphaMultiplied(0.6f);
                hintControl.Pivot = pivot;
                hintControl.PivotRelative = true;
                return hintControl;
            }
        }
        
        private void RemoveDockHints()
        {
            if (_toDock == null)
                return;

            if (_toDock.RootWindow.Window != _dragSourceWindow)
                _toDock.RootWindow.Window.MouseUp -= OnMouseUp;

            _dockHintDown?.Parent.RemoveChild(_dockHintDown);
            _dockHintUp?.Parent.RemoveChild(_dockHintUp);
            _dockHintLeft?.Parent.RemoveChild(_dockHintLeft);
            _dockHintRight?.Parent.RemoveChild(_dockHintRight);
            _dockHintCenter?.Parent.RemoveChild(_dockHintCenter);
            _dockHintDown = _dockHintUp = _dockHintLeft = _dockHintRight = _dockHintCenter = null;
        }

        private void UpdateRects(Float2 mousePos)
        {
            // Cache mouse position
            _mouse = mousePos;

            // Check intersection with any dock panel
            DockPanel dockPanel = null;
            if (_toMove.MasterPanel.HitTest(ref _mouse, _toMove, out var hitResults))
            {
                dockPanel = hitResults[0];

                // Prefer panel which currently has focus
                foreach (var hit in hitResults)
                {
                    if (hit.RootWindow.Window.IsFocused)
                    {
                        dockPanel = hit;
                        break;
                    }
                }

                // Prefer panel in the same window we hit earlier
                // TODO: this doesn't allow docking window into another floating window over the main window
                /*if (dockPanel?.RootWindow != _toDock?.RootWindow)
                {
                    foreach (var hit in hitResults)
                    {
                        if (hit.RootWindow == _toDock?.RootWindow)
                        {
                            dockPanel = _toDock;
                            break;
                        }
                    }
                }*/
            }
            
            if (dockPanel != _toDock)
            {
                RemoveDockHints();
                _toDock = dockPanel;
                AddDockHints();

                // Make sure the all the dock hint areas are not under other windows
                _toDock?.RootWindow.Window.BringToFront();
                //_toDock?.RootWindow.Window.Focus();

#if PLATFORM_SDL
                // Make the dragged window transparent when dock hints are visible
                _toMove.Window.Window.Opacity = _toDock == null ? 1.0f : DragWindowOpacity;
#else
                // Bring the drop source always to the top
                if (_dragSourceWindow != null)
                    _dragSourceWindow.BringToFront();
#endif
            }

            // Check dock state to use
            bool showProxyHints = _toDock != null;
            bool showBorderHints = showProxyHints;
            bool showCenterHint = showProxyHints;
            Control hoveredHintControl = null;
            Float2 hoveredSizeOverride = Float2.Zero;
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
                var size = _rectDock.Size / Platform.DpiScale;
                var offset = _toDock.PointFromScreen(_rectDock.Location);
                var borderMargin = 4.0f;
                var hintWindowsSize = HintControlSize;
                var hintWindowsSize2 = hintWindowsSize * 0.5f;
                var hintPreviewSize = new Float2(Math.Max(HintControlSize * 2, size.X * 0.5f), Math.Max(HintControlSize * 2, size.Y * 0.5f));
                var centerX = size.X * 0.5f;
                var centerY = size.Y * 0.5f;
                _rUpper = new Rectangle(centerX - hintWindowsSize2, borderMargin, hintWindowsSize, hintWindowsSize) + offset;
                _rBottom = new Rectangle(centerX - hintWindowsSize2, size.Y - hintWindowsSize - borderMargin, hintWindowsSize, hintWindowsSize) + offset;
                _rLeft = new Rectangle(borderMargin, centerY - hintWindowsSize2, hintWindowsSize, hintWindowsSize) + offset;
                _rRight = new Rectangle(size.X - hintWindowsSize - borderMargin, centerY - hintWindowsSize2, hintWindowsSize, hintWindowsSize) + offset;
                _rCenter = new Rectangle(centerX - hintWindowsSize2, centerY - hintWindowsSize2, hintWindowsSize, hintWindowsSize) + offset;

                // Hit test, and calculate the approximation for filled area when hovered over the hint
                DockState toSet = DockState.Float;
                if (showBorderHints)
                {
                    if (_rUpper.Contains(_toDock.PointFromScreen(_mouse)))
                    {
                        toSet = DockState.DockTop;
                        hoveredHintControl = _dockHintUp;
                        hoveredSizeOverride = new Float2(size.X, hintPreviewSize.Y);
                    }
                    else if (_rBottom.Contains(_toDock.PointFromScreen(_mouse)))
                    {
                        toSet = DockState.DockBottom;
                        hoveredHintControl = _dockHintDown;
                        hoveredSizeOverride = new Float2(size.X, hintPreviewSize.Y);
                    }
                    else if (_rLeft.Contains(_toDock.PointFromScreen(_mouse)))
                    {
                        toSet = DockState.DockLeft;
                        hoveredHintControl = _dockHintLeft;
                        hoveredSizeOverride = new Float2(hintPreviewSize.X, size.Y);
                    }
                    else if (_rRight.Contains(_toDock.PointFromScreen(_mouse)))
                    {
                        toSet = DockState.DockRight;
                        hoveredHintControl = _dockHintRight;
                        hoveredSizeOverride = new Float2(hintPreviewSize.X, size.Y);
                    }
                }
                if (showCenterHint && _rCenter.Contains(_toDock.PointFromScreen(_mouse)))
                {
                    toSet = DockState.DockFill;
                    hoveredHintControl = _dockHintCenter;
                    hoveredSizeOverride = new Float2(size.X, size.Y);
                }

                _toSet = toSet;
            }
            else
            {
                _toSet = DockState.Float;
            }
            
            // Update sizes and opacity of hint controls
            if (_toDock != null)
            {
                if (hoveredHintControl != _dockHintDown)
                {
                    _dockHintDown.Size = new Float2(HintControlSize);
                    _dockHintDown.BackgroundColor = Style.Current.Selection.AlphaMultiplied(0.6f);
                }
                if (hoveredHintControl != _dockHintLeft)
                {
                    _dockHintLeft.Size = new Float2(HintControlSize);
                    _dockHintLeft.BackgroundColor = Style.Current.Selection.AlphaMultiplied(0.6f);
                }
                if (hoveredHintControl != _dockHintRight)
                {
                    _dockHintRight.Size = new Float2(HintControlSize);
                    _dockHintRight.BackgroundColor = Style.Current.Selection.AlphaMultiplied(0.6f);
                }
                if (hoveredHintControl != _dockHintUp)
                {
                    _dockHintUp.Size = new Float2(HintControlSize);
                    _dockHintUp.BackgroundColor = Style.Current.Selection.AlphaMultiplied(0.6f);
                }
                if (hoveredHintControl != _dockHintCenter)
                {
                    _dockHintCenter.Size = new Float2(HintControlSize);
                    _dockHintCenter.BackgroundColor = Style.Current.Selection.AlphaMultiplied(0.6f);
                }

                if (_toSet != DockState.Float)
                {
                    if (hoveredHintControl != null)
                    {
                        hoveredHintControl.BackgroundColor = Style.Current.Selection.AlphaMultiplied(1.0f);
                        hoveredHintControl.Size = hoveredSizeOverride;
                    }
                }
            }

            // Update hint controls visibility and location
            if (showProxyHints)
            {
                if (hoveredHintControl != _dockHintDown)
                    _dockHintDown.Location = _rBottom.Location;
                if (hoveredHintControl != _dockHintLeft)
                    _dockHintLeft.Location = _rLeft.Location;
                if (hoveredHintControl != _dockHintRight)
                    _dockHintRight.Location = _rRight.Location;
                if (hoveredHintControl != _dockHintUp)
                    _dockHintUp.Location = _rUpper.Location;
                if (hoveredHintControl != _dockHintCenter)
                    _dockHintCenter.Location = _rCenter.Location;
                
                _dockHintDown.Visible = showProxyHints & showBorderHints;
                _dockHintLeft.Visible = showProxyHints & showBorderHints;
                _dockHintRight.Visible = showProxyHints & showBorderHints;
                _dockHintUp.Visible = showProxyHints & showBorderHints;
                _dockHintCenter.Visible = showProxyHints & showCenterHint;
            }
        }

        private void OnMouseUp(ref Float2 location, MouseButton button, ref bool handled)
        {
            if (button == MouseButton.Left)
                Dispose();
        }

        private void OnUpdate()
        {
            // If the engine lost focus during dragging, end the action
            if (!Engine.HasFocus)
            {
                Dispose();
                return;
            }

            var mousePos = Platform.MousePosition;
            if (_mouse != mousePos)
            {
                if (_dragSourceWindow != null)
                    _toMove.Window.Window.Position = mousePos - _dragOffset;

                UpdateRects(mousePos);
            }
        }
    }
}
