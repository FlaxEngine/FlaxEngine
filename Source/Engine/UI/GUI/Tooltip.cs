// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

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
        /// The horizontal alignment of the text.
        /// </summary>
        public TextAlignment HorizontalTextAlignment = TextAlignment.Center;

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
        public void Show(Control target, Float2 location, Rectangle targetArea)
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
            var dpiScale = target.RootWindow.DpiScale;
            var dpiSize = Size * dpiScale;
            var locationWS = target.PointToWindow(location);
            var locationSS = parentWin.PointToScreen(locationWS);
            _showTarget = target;
            WrapPosition(ref locationSS);

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
            desc.AllowInput = false;
            desc.AllowMinimize = false;
            desc.AllowMaximize = false;
            desc.AllowDragAndDrop = false;
            desc.IsTopmost = true;
            desc.IsRegularWindow = false;
            desc.HasSizingFrame = false;
            desc.ShowAfterFirstPaint = true;
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
            if (!Visible && _timeToPopupLeft > 0.0f)
            {
                _lastTarget = target;
                _timeToPopupLeft -= dt;

                if (_timeToPopupLeft <= 0.0f)
                {
                    if (_lastTarget.OnShowTooltip(out _currentText, out var location, out var area))
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
            if (Visible)
                Hide();
            _lastTarget = null;
        }

        private void UpdateWindowSize()
        {
            if (_window)
            {
                _window.ClientSize = Size * _window.DpiScale;
            }
        }

        private void WrapPosition(ref Float2 locationSS, float flipOffset = 0.0f)
        {
            if (_showTarget?.RootWindow == null)
                return;

            // Calculate popup direction
            var dpiScale = _showTarget.RootWindow.DpiScale;
            var dpiSize = Size * dpiScale;
            var monitorBounds = Platform.GetMonitorBounds(locationSS);
            var rightBottomMonitorBounds = monitorBounds.BottomRight;
            var rightBottomLocationSS = locationSS + dpiSize;

            // Prioritize tooltip placement within parent window, fall back to virtual desktop
            if (rightBottomMonitorBounds.Y < rightBottomLocationSS.Y)
            {
                // Direction: up
                locationSS.Y -= dpiSize.Y + flipOffset;
            }
            if (rightBottomMonitorBounds.X < rightBottomLocationSS.X)
            {
                // Direction: left
                locationSS.X -= dpiSize.X + flipOffset * 2;
            }
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            var mousePos = Input.MouseScreenPosition;
            var location = _showTarget.PointFromScreen(mousePos);
            if (!_showTarget.OnTestTooltipOverControl(ref location))
            {
                // Auto hide if mouse leaves control area
                Hide();
            }
            else
            {
                // Position tooltip when mouse moves
                WrapPosition(ref mousePos, 10);
                if (_window)
                    _window.Position = mousePos + new Float2(15, 10);
            }

            base.Update(deltaTime);
        }

        /// <inheritdoc />
        public override void Draw()
        {
            var style = Style.Current;

            // Background
            Render2D.FillRectangle(new Rectangle(Float2.Zero, Size), Color.Lerp(style.BackgroundSelected, style.Background, 0.6f));
            Render2D.FillRectangle(new Rectangle(1.1f, 1.1f, Width - 2, Height - 2), style.Background);

            // Padding for text
            var textRect = GetClientArea();
            float textX = HorizontalTextAlignment switch
            {
                TextAlignment.Near => 15,
                TextAlignment.Center => 5,
                TextAlignment.Far => -5,
                _ => throw new ArgumentOutOfRangeException()
            };
            textRect.X += textX;
            textRect.Width -= 10;

            // Tooltip text
            Render2D.DrawText(
                              style.FontMedium,
                              _currentText,
                              textRect,
                              style.Foreground,
                              HorizontalTextAlignment,
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
            var size = Float2.Zero;
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
            Size = size + new Float2(24.0f);

            // Check if is visible size get changed
            if (Visible && prevSize != Size)
            {
                UpdateWindowSize();
            }
        }

        /// <inheritdoc />
        public override bool OnShowTooltip(out string text, out Float2 location, out Rectangle area)
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
