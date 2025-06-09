// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI
{
    /// <summary>
    /// Menu strip with child buttons.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    public sealed class MainMenu : ContainerControl
    {
#if PLATFORM_WINDOWS
        private bool _useCustomWindowSystem;
        private Image _icon;
        private Label _title;
        private Button _closeButton;
        private Button _minimizeButton;
        private Button _maximizeButton;
        private LocalizedString _charChromeRestore, _charChromeMaximize;
        private Window _window;
#endif
        private MainMenuButton _selected;

        /// <summary>
        /// Gets or sets the selected button (with opened context menu).
        /// </summary>
        public MainMenuButton Selected
        {
            get => _selected;
            set
            {
                if (_selected == value)
                    return;

                if (_selected != null)
                {
                    _selected.ContextMenu.VisibleChanged -= OnSelectedContextMenuVisibleChanged;
                    _selected.ContextMenu.Hide();
                }

                _selected = value;

                if (_selected != null && _selected.ContextMenu.HasChildren)
                {
                    _selected.ContextMenu.Show(_selected, new Float2(0, _selected.Height));
                    _selected.ContextMenu.VisibleChanged += OnSelectedContextMenuVisibleChanged;
                }
            }
        }

        private void OnSelectedContextMenuVisibleChanged(Control control)
        {
            if (_selected != null)
                Selected = null;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="MainMenu"/> class.
        /// </summary>
        /// <param name="mainWindow">The main window.</param>
        public MainMenu(RootControl mainWindow)
        : base(0, 0, 0, 20)
        {
            AutoFocus = false;
            AnchorPreset = AnchorPresets.HorizontalStretchTop;

#if PLATFORM_WINDOWS
            _useCustomWindowSystem = !Editor.Instance.Options.Options.Interface.UseNativeWindowSystem;
            if (_useCustomWindowSystem)
            {
                BackgroundColor = Style.Current.LightBackground;
                Height = 28;

                var windowIcon = FlaxEngine.Content.LoadAsyncInternal<Texture>(EditorAssets.WindowIcon);
                FontAsset windowIconsFont = FlaxEngine.Content.LoadAsyncInternal<FontAsset>(EditorAssets.WindowIconsFont);
                Font iconFont = windowIconsFont?.CreateFont(9);

                _window = mainWindow.RootWindow.Window;
                _window.HitTest += OnHitTest;
                _window.Closed += OnWindowClosed;

                ScriptsBuilder.GetBinariesConfiguration(out _, out _, out _, out var configuration);

                _icon = new Image
                {
                    Margin = new Margin(6, 6, 6, 6),
                    Brush = new TextureBrush(windowIcon),
                    Color = Style.Current.Foreground,
                    KeepAspectRatio = false,
                    TooltipText = string.Format("{0}\nVersion {1}\nConfiguration {3}\nGraphics {2}", _window.Title, Globals.EngineVersion, GPUDevice.Instance.RendererType, configuration),
                    Parent = this,
                };

                _title = new Label(0, 0, Width, Height)
                {
                    Text = _window.Title,
                    HorizontalAlignment = TextAlignment.Center,
                    VerticalAlignment = TextAlignment.Center,
                    ClipText = true,
                    TextColor = Style.Current.ForegroundGrey,
                    TextColorHighlighted = Style.Current.ForegroundGrey,
                    Parent = this,
                };

                _closeButton = new Button
                {
                    Text = ((char)EditorAssets.SegMDL2Icons.ChromeClose).ToString(),
                    Font = new FontReference(iconFont),
                    BackgroundColor = Color.Transparent,
                    BorderColor = Color.Transparent,
                    BorderColorHighlighted = Color.Transparent,
                    BorderColorSelected = Color.Transparent,
                    TextColor = Style.Current.Foreground,
                    Width = 46,
                    BackgroundColorHighlighted = Color.Red,
                    BackgroundColorSelected = Color.Red.RGBMultiplied(1.3f),
                    Parent = this,
                };
                _closeButton.Clicked += () => _window.Close(ClosingReason.User);

                _minimizeButton = new Button
                {
                    Text = ((char)EditorAssets.SegMDL2Icons.ChromeMinimize).ToString(),
                    Font = new FontReference(iconFont),
                    BackgroundColor = Color.Transparent,
                    BorderColor = Color.Transparent,
                    BorderColorHighlighted = Color.Transparent,
                    BorderColorSelected = Color.Transparent,
                    TextColor = Style.Current.Foreground,
                    Width = 46,
                    BackgroundColorHighlighted = Style.Current.LightBackground.RGBMultiplied(1.3f),
                    Parent = this,
                };
                _minimizeButton.Clicked += () => _window.Minimize();

                _maximizeButton = new Button
                {
                    Text = ((char)(_window.IsMaximized ? EditorAssets.SegMDL2Icons.ChromeRestore : EditorAssets.SegMDL2Icons.ChromeMaximize)).ToString(),
                    Font = new FontReference(iconFont),
                    BackgroundColor = Color.Transparent,
                    BorderColor = Color.Transparent,
                    BorderColorHighlighted = Color.Transparent,
                    BorderColorSelected = Color.Transparent,
                    TextColor = Style.Current.Foreground,
                    Width = 46,
                    BackgroundColorHighlighted = Style.Current.LightBackground.RGBMultiplied(1.3f),
                    Parent = this,
                };
                _maximizeButton.Clicked += () =>
                {
                    if (_window.IsMaximized)
                        _window.Restore();
                    else
                        _window.Maximize();
                };
                _charChromeRestore = ((char)EditorAssets.SegMDL2Icons.ChromeRestore).ToString();
                _charChromeMaximize = ((char)EditorAssets.SegMDL2Icons.ChromeMaximize).ToString();
            }
            else
#endif
            {
                BackgroundColor = Style.Current.LightBackground;
            }
        }

#if PLATFORM_WINDOWS
        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            base.Update(deltaTime);

            if (_maximizeButton != null)
            {
                _maximizeButton.Text = _window.IsMaximized ? _charChromeRestore : _charChromeMaximize;
            }
        }

        private void OnWindowClosed()
        {
            if (_window != null)
            {
                _window.HitTest = null;
                _window = null;
            }
        }

        private WindowHitCodes OnHitTest(ref Float2 mouse)
        {
            var dpiScale = _window.DpiScale;

            if (_window.IsMinimized)
                return WindowHitCodes.NoWhere;

            if (!_window.IsMaximized)
            {
                var pos = _window.ScreenToClient(mouse * dpiScale); // pos is not DPI adjusted
                var winSize = _window.Size;

                // Distance from which the mouse is considered to be on the border/corner
                float distance = 5.0f * dpiScale;

                if (pos.Y > winSize.Y - distance && pos.X < distance)
                    return WindowHitCodes.BottomLeft;

                if (pos.X > winSize.X - distance && pos.Y > winSize.Y - distance)
                    return WindowHitCodes.BottomRight;

                if (pos.Y < distance && pos.X < distance)
                    return WindowHitCodes.TopLeft;

                if (pos.Y < distance && pos.X > winSize.X - distance)
                    return WindowHitCodes.TopRight;

                if (pos.X > winSize.X - distance)
                    return WindowHitCodes.Right;

                if (pos.X < distance)
                    return WindowHitCodes.Left;

                if (pos.Y < distance)
                    return WindowHitCodes.Top;

                if (pos.Y > winSize.Y - distance)
                    return WindowHitCodes.Bottom;
            }

            var mousePos = PointFromScreen(mouse * dpiScale);
            var controlUnderMouse = GetChildAt(mousePos);
            var isMouseOverSth = controlUnderMouse != null && controlUnderMouse != _title;
            var rb = GetRightButton();
            if (rb != null && _minimizeButton != null && new Rectangle(rb.UpperRight, _minimizeButton.BottomLeft - rb.UpperRight).Contains(ref mousePos) && !isMouseOverSth)
                return WindowHitCodes.Caption;

            return WindowHitCodes.Client;
        }
#endif

        /// <summary>
        /// Return the rightmost button.
        /// </summary>
        /// <returns>Rightmost button, null if there is no <see cref="MainMenuButton"/></returns>
        private MainMenuButton GetRightButton()
        {
            MainMenuButton b = null;
            foreach (var control in Children)
            {
                if (b == null && control is MainMenuButton)
                    b = (MainMenuButton)control;

                if (control is MainMenuButton && control.Right > b.Right)
                    b = (MainMenuButton)control;
            }
            return b;
        }

        /// <summary>
        /// Adds the button.
        /// </summary>
        /// <param name="text">The button text.</param>
        /// <returns>Created button control.</returns>
        public MainMenuButton AddButton(string text)
        {
            return AddChild(new MainMenuButton(text));
        }

        /// <summary>
        /// Gets or adds a button.
        /// </summary>
        /// <param name="text">The button text</param>
        /// <returns>The existing or created button control.</returns>
        public MainMenuButton GetOrAddButton(string text)
        {
            MainMenuButton result = GetButton(text);
            if (result == null)
                result = AddButton(text);
            return result;
        }

        /// <summary>
        /// Gets the button.
        /// </summary>
        /// <param name="text">The button text.</param>
        /// <returns>The button or null if missing.</returns>
        public MainMenuButton GetButton(string text)
        {
            MainMenuButton result = null;
            for (int i = 0; i < Children.Count; i++)
            {
                if (Children[i] is MainMenuButton button && string.Equals(button.Text, text, StringComparison.OrdinalIgnoreCase))
                {
                    result = button;
                    break;
                }
            }
            return result;
        }

        /// <inheritdoc />
        public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            if (base.OnMouseDoubleClick(location, button))
                return true;

#if PLATFORM_WINDOWS
            var child = GetChildAtRecursive(location);
            if (_useCustomWindowSystem && child is not Button && child is not MainMenuButton)
            {
                if (_window.IsMaximized)
                    _window.Restore();
                else
                    _window.Maximize();
            }
#endif

            return true;
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            if (base.OnKeyDown(key))
                return true;

            // Fallback to the edit window for shortcuts
            var editor = Editor.Instance;
            return editor.Windows.EditWin.InputActions.Process(editor, this, key);
        }

        /// <inheritdoc />
        protected override void PerformLayoutAfterChildren()
        {
            float x = 0;

#if PLATFORM_WINDOWS
            if (_useCustomWindowSystem)
            {
                // Icon
                _icon.X = x;
                _icon.Size = new Float2(Height);
                x += _icon.Width;
            }
#endif

            // Arrange controls
            MainMenuButton rightMostButton = null;
            for (int i = 0; i < _children.Count; i++)
            {
                var c = _children[i];
                if (c is MainMenuButton b && c.Visible)
                {
                    b.Bounds = new Rectangle(x, 0, b.Width, Height);

                    if (rightMostButton == null)
                        rightMostButton = b;
                    else if (rightMostButton.X < b.X)
                        rightMostButton = b;

                    x += b.Width;
                }
            }

#if PLATFORM_WINDOWS
            if (_useCustomWindowSystem)
            {
                // Buttons
                _closeButton.Height = Height;
                _closeButton.X = Width - _closeButton.Width;
                _maximizeButton.Height = Height;
                _maximizeButton.X = _closeButton.X - _maximizeButton.Width;
                _minimizeButton.Height = Height;
                _minimizeButton.X = _maximizeButton.X - _minimizeButton.Width;

                // Title
                _title.Bounds = new Rectangle(x + 2, 0, _minimizeButton.Left - x - 4, Height);
                //_title.Text = _title.Width < 300.0f ? Editor.Instance.ProjectInfo.Name : _window.Title;
            }
#endif
        }

#if PLATFORM_WINDOWS
        /// <inheritdoc />
        public override void OnDestroy()
        {
            base.OnDestroy();

            if (_window != null)
            {
                _window.Closed -= OnWindowClosed;
                OnWindowClosed();
            }
        }
#endif
    }
}
