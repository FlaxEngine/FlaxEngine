using System;
using FlaxEngine;
using FlaxEngine.GUI;

/// <summary>
/// In-game HUD: title, FPS/stats readout, hint text and a quit button.
/// Pure-code UI (no assets), sized for phone screens, touch-friendly.
/// </summary>
public class DemoPanel : ContainerControl
{
    private Label _titleLabel;
    private Label _statsLabel;
    private Label _hintLabel;
    private Button _quitButton;
    private float _statsTimer;

    public DemoPanel()
    {
        AnchorPreset = AnchorPresets.StretchAll;
        Offsets = Margin.Zero;
        AutoFocus = false;

        // Title
        _titleLabel = new Label
        {
            Text = "Flax Mobile",
            AnchorPreset = AnchorPresets.TopCenter,
            Offsets = new Margin(-220, 440, 40, 60),
            TextColor = Color.White,
            HorizontalAlignment = TextAlignment.Center,
        };
        AddChild(_titleLabel);

        // Stats (FPS, resolution, touch count)
        _statsLabel = new Label
        {
            Text = "FPS: --",
            AnchorPreset = AnchorPresets.TopLeft,
            Offsets = new Margin(16, 380, 20, 34),
            TextColor = new Color(0.85f, 0.9f, 1.0f),
        };
        AddChild(_statsLabel);

        // Hint
        _hintLabel = new Label
        {
            Text = "Drag to orbit - Pinch/second finger to zoom",
            AnchorPreset = AnchorPresets.BottomCenter,
            Offsets = new Margin(-260, 520, -70, 44),
            TextColor = new Color(0.9f, 0.9f, 0.9f, 0.9f),
            HorizontalAlignment = TextAlignment.Center,
        };
        AddChild(_hintLabel);

        // Quit button (touch friendly)
        _quitButton = new Button
        {
            Text = "Exit",
            AnchorPreset = AnchorPresets.BottomRight,
            Offsets = new Margin(-130, 110, -80, 54),
        };
        _quitButton.Clicked += () => Engine.RequestExit();
        AddChild(_quitButton);
    }

    /// <summary>
    /// Shows this panel (called on scene start).
    /// </summary>
    public void Show()
    {
        Visible = true;
    }

    /// <inheritdoc />
    public override void Update(float deltaTime)
    {
        base.Update(deltaTime);

        // Refresh stats twice a second
        _statsTimer += deltaTime;
        if (_statsTimer > 0.5f)
        {
            _statsTimer = 0f;
            _statsLabel.Text = $"FPS: {Engine.FramesPerSecond:0}  |  {Screen.Size.X:0}x{Screen.Size.Y:0}\n" +
                               "Flax Engine";
        }
    }
}
