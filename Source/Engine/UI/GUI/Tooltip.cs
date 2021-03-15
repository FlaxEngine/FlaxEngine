// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// The tooltip popup.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    [HideInEditor]
    public class Tooltip : ContainerControl
    {
        private float _timeToPopupLeft;
        private Control _lastTarget;
        private Control _showTarget;
        private string _currentText;
        private Window _window;

        /// <summary>
        /// Gets or sets the time in seconds that mouse have to be over the target to show the tooltip.
        /// </summary>
        public float TimeToShow { get; set; } = 0.3f; // 300ms by default

        /// <summary>
        /// Gets or sets the maximum width of the tooltip. Used to wrap text that overflows and ensure that tooltip stays readable.
        /// </summary>
        public float MaxWidth { get; set; } = 500.0f;

        /// <summary>
        /// Gets the tooltip window.
        /// </summary>
        public Window Window => _window;

        /// <summary>
        /// Initializes a new instance of the <see cref="Tooltip"/> class.
        /// </summary>
        public Tooltip()
        : base(0, 0, 300, 24)
        {
            Visible = false;
            AutoFocus = false;
        }

        /// <summary>
        /// Shows tooltip over given control.
        /// </summary>
        /// <param name="target">The parent control to attach to it.</param>
        /// <param name="location">Popup menu origin location in parent control coordinates.</param>
        /// <param name="targetArea">Tooltip target area of interest.</param>
        public void Show(Control target, Vector2 location, Rectangle targetArea)
        {
            if (target == null)
                throw new ArgumentNullException();

            // Ensure to be closed
            Hide();

            // Block showing tooltips when application is not focused
            if (!Platform.HasFocus)
                return;

            // Unlock and perform controls update
            UnlockChildrenRecursive();
            PerformLayout();

            // Calculate popup direction and initial location
            var parentWin = target.Root;
            if (parentWin == null)
                return;
            float dpiScale = target.RootWindow.DpiScale;
            Vector2 dpiSize = Size * dpiScale;
            Vector2 locationWS = target.PointToWindow(location);
            Vector2 locationSS = parentWin.PointToScreen(locationWS);
            Vector2 screenSize = Platform.VirtualDesktopSize;
            Vector2 rightBottomLocationSS = locationSS + dpiSize;
            if (screenSize.Y < rightBottomLocationSS.Y)
            {
                // Direction: up
                locationSS.Y -= dpiSize.Y;
            }
            if (screenSize.X < rightBottomLocationSS.X)
            {
                // Direction: left
                locationSS.X -= dpiSize.X;
            }
            _showTarget = target;

            // Create window
            var desc = CreateWindowSettings.Default;
            desc.StartPosition = WindowStartPosition.Manual;
            desc.Position = locationSS;
            desc.Size = dpiSize;
            desc.Fullscreen = false;
            desc.HasBorder = false;
            desc.SupportsTransparency = false;
            desc.ShowInTaskbar = false;
            desc.ActivateWhenFirstShown = false;
            desc.AllowInput = true;
            desc.AllowMinimize = false;
            desc.AllowMaximize = false;
            desc.AllowDragAndDrop = false;
            desc.IsTopmost = true;
            desc.IsRegularWindow = false;
            desc.HasSizingFrame = false;
            _window = Platform.CreateWindow(ref desc);
            if (_window == null)
                throw new InvalidOperationException("Failed to create tooltip window.");

            // Attach to the window and focus
            Parent = _window.GUI;
            Visible = true;
            _window.Show();
            _showTarget.OnTooltipShown(this);
        }

        /// <summary>
        /// Hides the popup.
        /// </summary>
        public void Hide()
        {
            if (!Visible)
                return;

            // Unlink
            IsLayoutLocked = true;
            Parent = null;

            // Close window
            if (_window)
            {
                var win = _window;
                _window = null;
                win.Close();
            }

            // Hide
            Visible = false;
        }

        /// <summary>
        /// Called when mouse enters a control.
        /// </summary>
        /// <param name="target">The target.</param>
        public void OnMouseEnterControl(Control target)
        {
            _lastTarget = target;
            _timeToPopupLeft = TimeToShow;
        }

        /// <summary>
        /// Called when mouse is over a control.
        /// </summary>
        /// <param name="target">The target.</param>
        /// <param name="dt">The delta time.</param>
        public void OnMouseOverControl(Control target, float dt)
        {
            if (!Visible)
            {
                _lastTarget = target;
                _timeToPopupLeft -= dt;

                if (_timeToPopupLeft <= 0.0f)
                {
                    if (_lastTarget.OnShowTooltip(out _currentText, out Vector2 location, out Rectangle area))
                    {
                        Show(_lastTarget, location, area);
                    }
                }
            }
        }

        /// <summary>
        /// Called when mouse leaves a control.
        /// </summary>
        /// <param name="target">The target.</param>
        public void OnMouseLeaveControl(Control target)
        {
            _lastTarget = null;
        }

        private void UpdateWindowSize()
        {
            if (_window)
            {
                _window.ClientSize = Size * _window.DpiScale;
            }
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            // Auto hide if mouse leaves control area
            Vector2 mousePos = Input.MouseScreenPosition;
            Vector2 location = _showTarget.PointFromScreen(mousePos);
            if (!_showTarget.OnTestTooltipOverControl(ref location))
            {
                // Mouse left or sth
                Hide();
            }

            base.Update(deltaTime);
        }

        /// <inheritdoc />
        public override void Draw()
        {
            var style = Style.Current;

            // Background
            Render2D.FillRectangle(new Rectangle(Vector2.Zero, Size), Color.Lerp(style.BackgroundSelected, style.Background, 0.6f));
            Render2D.FillRectangle(new Rectangle(1.1f, 1.1f, Width - 2, Height - 2), style.Background);

            // Tooltip text
            Render2D.DrawText(
                              style.FontMedium,
                              _currentText,
                              GetClientArea(),
                              style.Foreground,
                              TextAlignment.Center,
                              TextAlignment.Center,
                              TextWrapping.WrapWords
                             );
        }

        /// <inheritdoc />
        protected override void PerformLayoutBeforeChildren()
        {
            base.PerformLayoutBeforeChildren();

            var prevSize = Size;
            var style = Style.Current;

            // Calculate size of the tooltip
            var size = Vector2.Zero;
            if (style != null && style.FontMedium && !string.IsNullOrEmpty(_currentText))
            {
                var layout = TextLayoutOptions.Default;
                layout.Bounds = new Rectangle(0, 0, MaxWidth, 10000000);
                layout.HorizontalAlignment = TextAlignment.Center;
                layout.VerticalAlignment = TextAlignment.Center;
                layout.TextWrapping = TextWrapping.WrapWords;
                var items = style.FontMedium.ProcessText(_currentText, ref layout);
                for (int i = 0; i < items.Length; i++)
                {
                    ref var item = ref items[i];
                    size.X = Mathf.Max(size.X, item.Size.X + 8.0f);
                    size.Y += item.Size.Y;
                }
                //size.X += style.FontMedium.MeasureText(_currentText).X;
            }
            Size = size + new Vector2(24.0f);

            // Check if is visible size get changed
            if (Visible && prevSize != Size)
            {
                UpdateWindowSize();
            }
        }

        /// <inheritdoc />
        public override bool OnShowTooltip(out string text, out Vector2 location, out Rectangle area)
        {
            base.OnShowTooltip(out text, out location, out area);

            // It's better not to show tooltip for a tooltip.
            // It would be kind of tooltipness.
            return false;
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            Hide();

            base.OnDestroy();
        }
    }
}
