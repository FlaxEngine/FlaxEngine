// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI.Docking
{
    /// <summary>
    /// Floating Window Dock Panel control.
    /// </summary>
    /// <seealso cref="DockPanel" />
    public class FloatWindowDockPanel : DockPanel
    {
        private class FloatWindowDecorations : WindowDecorations
        {
            private FloatWindowDockPanel _panel;

            public FloatWindowDecorations(FloatWindowDockPanel panel)
            : base(panel.RootWindow)
            {
                _panel = panel;
            }

            /// <inheritdoc />
            public override bool OnMouseDown(Float2 location, MouseButton button)
            {
                if (Title.Bounds.Contains(location) && button == MouseButton.Left)
                {
                    _panel.BeginDrag();
                    return true;
                }
                return base.OnMouseDown(location, button);
            }

#if !PLATFORM_WINDOWS
            /// <inheritdoc />
            protected override WindowHitCodes OnHitTest(ref Float2 mouse)
            {
                var hit = base.OnHitTest(ref mouse);
                if (hit == WindowHitCodes.Caption)
                {
                    // Override the system behaviour when interacting with the caption area
                    hit = WindowHitCodes.Client;
                }
                return hit;
            }
#endif
        }

        private MasterDockPanel _masterPanel;
        private WindowRootControl _window;

        /// <summary>
        /// Gets the master panel.
        /// </summary>
        public MasterDockPanel MasterPanel => _masterPanel;

        /// <summary>
        /// Gets the window.
        /// </summary>
        public WindowRootControl Window => _window;

        /// <summary>
        /// Initializes a new instance of the <see cref="FloatWindowDockPanel"/> class.
        /// </summary>
        /// <param name="masterPanel">The master panel.</param>
        /// <param name="window">The window.</param>
        public FloatWindowDockPanel(MasterDockPanel masterPanel, RootControl window)
        : base(null)
        {
            _masterPanel = masterPanel;
            _window = (WindowRootControl)window;

            // Link
            _masterPanel.FloatingPanels.Add(this);
            Parent = window;
            _window.Window.Closing += OnClosing;
            _window.Window.LeftButtonHit += OnLeftButtonHit;

            if (Utilities.Utils.UseCustomWindowDecorations())
            {
                var decorations = Parent.AddChild(new FloatWindowDecorations(this));
                decorations.SetAnchorPreset(AnchorPresets.HorizontalStretchTop, false);
            }
        }

        /// <inheritdoc />
        protected override void PerformLayoutBeforeChildren()
        {
            base.PerformLayoutBeforeChildren();

            var decorations = Parent.GetChild<FloatWindowDecorations>();
            if (decorations != null)
            {
                // Apply offset for the title bar
                foreach (var child in Children)
                    child.Bounds = child.Bounds with { Y = decorations.Height, Height = Parent.Height - decorations.Height };
            }
        }

        /// <summary>
        /// Begin drag operation on the window
        /// </summary>
        public void BeginDrag()
        {
            // Filter invalid events
            if (_window == null)
                return;

            // Create docking hint window
            Window dragSourceWindow = null;
#if !PLATFORM_SDL
            dragSourceWindow = _window?.Window;
#endif
            WindowDragHelper.StartDragging(this, dragSourceWindow);
        }

        /// <summary>
        /// Creates a floating window.
        /// </summary>
        /// <param name="parent">Parent window handle.</param>
        /// <param name="location">Client area location.</param>
        /// <param name="size">Window client area size.</param>
        /// <param name="startPosition">Window start position.</param>
        /// <param name="title">Initial window title.</param>
        internal static Window CreateFloatWindow(RootControl parent, Float2 location, Float2 size, WindowStartPosition startPosition, string title)
        {
            // Setup initial window settings
            var settings = CreateWindowSettings.Default;
            settings.Parent = (parent as WindowRootControl)?.Window;
            settings.Title = title;
            settings.Size = size;
            settings.Position = location;
            settings.MinimumSize = new Float2(100, 100);
            settings.MaximumSize = Float2.Zero; // Unlimited size
            settings.Fullscreen = false;
            settings.HasBorder = true;
#if PLATFORM_SDL
            settings.SupportsTransparency = true;
#else
            settings.SupportsTransparency = false;
#endif
            settings.ActivateWhenFirstShown = true;
            settings.AllowInput = true;
            settings.AllowMinimize = true;
            settings.AllowMaximize = true;
            settings.AllowDragAndDrop = true;
            settings.IsTopmost = false;
            settings.Type = WindowType.Regular;
            settings.HasSizingFrame = true;
            settings.ShowAfterFirstPaint = false;
            settings.ShowInTaskbar = true;
            settings.StartPosition = startPosition;

            if (Utilities.Utils.UseCustomWindowDecorations())
            {
                settings.HasBorder = false;
                //settings.HasSizingFrame = false;
            }

            // Create window
            return Platform.CreateWindow(ref settings);
        }

        private bool OnLeftButtonHit(WindowHitCodes hitTest)
        {
            if (hitTest == WindowHitCodes.Caption)
            {
                BeginDrag();
                return true;
            }

            return false;
        }

        private void OnClosing(ClosingReason reason, ref bool cancel)
        {
            // Close all docked windows
            while (Tabs.Count > 0)
            {
                if (Tabs[0].Close(reason))
                {
                    // Cancel
                    cancel = true;
                    return;
                }
            }

            // Unlink
            _window.Window.Closing -= OnClosing;
            _window.Window.LeftButtonHit = null;
            _window = null;

            // Remove object
            FlaxEngine.Assertions.Assert.IsTrue(TabsCount == 0 && ChildPanelsCount == 0);
            Dispose();
        }

        /// <inheritdoc />
        public override bool IsFloating => true;

        /// <inheritdoc />
        public override DockState TryGetDockState(out float splitterValue)
        {
            splitterValue = 0.5f;
            return DockState.Float;
        }

        /// <inheritdoc />
        protected override void OnLastTabRemoved()
        {
            if (ChildPanelsCount > 0)
                return;
            // Close window
            _window?.Close();
        }

        /// <inheritdoc />
        protected override void OnSelectedTabChanged()
        {
            base.OnSelectedTabChanged();

            if (_window != null && SelectedTab != null)
            {
                _window.Title = SelectedTab.Title;
                var decorations = Parent.GetChild<FloatWindowDecorations>();
                if (decorations != null)
                    decorations.PerformLayout();
            }
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _masterPanel?.FloatingPanels.Remove(this);

            base.OnDestroy();
        }
    }
}
