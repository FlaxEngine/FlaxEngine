// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.GUI.Docking;
using FlaxEditor.Options;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI;

/// <summary>
/// Represents the title bar of the window with buttons.
/// </summary>
/// <seealso cref="FlaxEngine.GUI.ContainerControl" />
public class WindowDecorations : ContainerControl
{
    private Image _icon;
    private Label _title;
    private Button _closeButton;
    private Button _minimizeButton;
    private Button _maximizeButton;
    private LocalizedString _charChromeRestore, _charChromeMaximize;
    private Window _window;

    /// <summary>
    /// The title label in the title bar.
    /// </summary>
    public Label Title => _title;
    
    /// <summary>
    /// The icon used in the title bar.
    /// </summary>
    public Image Icon => _icon;
    
    /// <summary>
    /// The tooltip shown when hovering over the icon.
    /// </summary>
    public string IconTooltipText
    {
        get => _icon?.TooltipText ?? null;
        set
        {
            if (_icon != null)
                _icon.TooltipText = value;
        }
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="WindowDecorations"/> class.
    /// </summary>
    /// <param name="window">The window.</param>
    /// <param name="iconOnly">When set, omit drawing title and buttons.</param>
    public WindowDecorations(RootControl window, bool iconOnly = false)
    : base(0, 0, 0, 20)
    {
        _window = window.RootWindow.Window;
        
        AutoFocus = false;
        AnchorPreset = AnchorPresets.HorizontalStretchTop;
        BackgroundColor = Color.Transparent;

        var windowIcon = FlaxEngine.Content.LoadAsyncInternal<Texture>(EditorAssets.WindowIcon);

        _icon = new Image
        {
            Margin = new Margin(4, 4, 4, 4),
            Brush = new TextureBrush(windowIcon),
            Color = Style.Current.Foreground,
            BackgroundColor = Style.Current.LightBackground,
            KeepAspectRatio = false,
            Parent = this,
        };

        if (!iconOnly)
        {
            _icon.Margin = new Margin(6, 6, 6, 6);
            Height = 28;
            
            _window.HitTest += OnHitTest;
            _window.Closed += OnWindowClosed;
            
            FontAsset windowIconsFont = FlaxEngine.Content.LoadAsyncInternal<FontAsset>(EditorAssets.WindowIconsFont);
            Font iconFont = windowIconsFont?.CreateFont(9);
            
            _title = new Label(0, 0, Width, Height)
            {
                Text = _window.Title,
                HorizontalAlignment = TextAlignment.Center,
                VerticalAlignment = TextAlignment.Center,
                ClipText = true,
                TextColor = Style.Current.ForegroundGrey,
                TextColorHighlighted = Style.Current.ForegroundGrey,
                BackgroundColor = Style.Current.LightBackground,
                Parent = this,
            };

            _closeButton = new Button
            {
                Text = ((char)EditorAssets.SegMDL2Icons.ChromeClose).ToString(),
                Font = new FontReference(iconFont),
                BackgroundColor = Style.Current.LightBackground,
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
                BackgroundColor = Style.Current.LightBackground,
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
                BackgroundColor = Style.Current.LightBackground,
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
    }
    
    /// <inheritdoc />
    public override void Update(float deltaTime)
    {
        base.Update(deltaTime);

        if (_maximizeButton != null)
        {
            var maximizeText = _window.IsMaximized ? _charChromeRestore : _charChromeMaximize;
            if (_maximizeButton.Text != maximizeText)
                _maximizeButton.Text = maximizeText;
        }
    }

    private void OnWindowClosed()
    {
        if (_window != null)
        {
            _window.HitTest -= OnHitTest;
            _window = null;
        }
    }

    /// <summary>
    /// Perform hit test on the window.
    /// </summary>
    /// <param name="mouse">The mouse position</param>
    /// <returns>The hit code for given position.</returns>
    protected virtual WindowHitCodes OnHitTest(ref Float2 mouse)
    {
        if (_window.IsMinimized)
            return WindowHitCodes.NoWhere;

        var dpiScale = _window.DpiScale;
        var pos = _window.ScreenToClient(mouse * dpiScale); // pos is not DPI adjusted
        if (!_window.IsMaximized)
        {
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
        
        var controlUnderMouse = GetChildAt(pos, control => control != _title);
        if (_title.Bounds.Contains(pos) && controlUnderMouse == null)
            return WindowHitCodes.Caption;

        return WindowHitCodes.Client;
    }
    
    /// <inheritdoc />
    public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
    {
        // These may not work with main window due to SDL not passing mouse events
        // when interacting with hit tests on caption area...
        
        if (Title.Bounds.Contains(location) && button == MouseButton.Left)
        {
            if (_window.IsMaximized)
                _window.Restore();
            else
                _window.Maximize();
            return true;
        }
        else if (Icon.Bounds.Contains(location) && button == MouseButton.Left)
        {
            _window.Close(ClosingReason.User);
            return true;
        }
        return base.OnMouseDoubleClick(location, button);
    }

    /// <inheritdoc />
    protected override void PerformLayoutAfterChildren()
    {
        // Calculate extents for title bounds area excluding the icon and main menu area
        float x = 0;
        
        // Icon
        if (_icon != null)
        {
            _icon.X = x;
            _icon.Size = new Float2(Height);
            x += _icon.Width;
        }

        // Main menu if present
        if (Parent.GetChild<MainMenu>() is MainMenu mainMenu)
        {
            for (int i = 0; i < mainMenu.Children.Count; i++)
            {
                var c = mainMenu.Children[i];
                if (c is MainMenuButton b && c.Visible)
                {
                    b.Bounds = new Rectangle(x, 0, b.Width, Height);
                    x += b.Width;
                }
            }
        }
     
        // Buttons
        float rightMostButtonX = Width;
        if (_closeButton != null)
        {
            _closeButton.Height = Height;
            _closeButton.X = rightMostButtonX - _closeButton.Width;
            rightMostButtonX = _closeButton.X;
        }
        if (_maximizeButton != null)
        {
            _maximizeButton.Height = Height;
            _maximizeButton.X = rightMostButtonX - _maximizeButton.Width;
            rightMostButtonX = _maximizeButton.X;
        }
        if (_minimizeButton != null)
        {
            _minimizeButton.Height = Height;
            _minimizeButton.X = rightMostButtonX - _minimizeButton.Width;
            rightMostButtonX = _minimizeButton.X;
        }

        // Title
        if (_title != null)
        {
            _title.Text = _window.Title;
            _title.Bounds = new Rectangle(x, 0, rightMostButtonX - x, Height);
        }
    }
    
    /// <inheritdoc />
    public override void Draw()
    {
        base.Draw();
        DrawBorders();
    }

    /// <summary>
    /// Draw borders around the window.
    /// </summary>
    public virtual void DrawBorders()
    {
        var win = RootWindow.Window;
        if (win.IsMaximized)
            return;
        
        const float thickness = 1.0f;
        Color color = Editor.Instance.UI.StatusBar.StatusColor;
        Rectangle rect = new Rectangle(thickness * 0.5f, thickness * 0.5f, Parent.Width - thickness, Parent.Height - thickness);
        Render2D.DrawRectangle(rect, color);
    }

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
}
