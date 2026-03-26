// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using FlaxEditor.GUI.Input;
using FlaxEditor.GUI.Tabs;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Json;

namespace FlaxEditor.GUI.Dialogs
{
    /// <summary>
    /// The base interface for the color picker dialogs.
    /// </summary>
    public interface IColorPickerDialog
    {
        /// <summary>
        /// Closes the picker (similar to value editing cancel).
        /// </summary>
        void ClosePicker();
    }

    /// <summary>
    /// Color picking dialog.
    /// </summary>
    /// <seealso cref="FlaxEditor.GUI.Dialogs.Dialog" />
    public class ColorPickerDialog : Dialog, IColorPickerDialog
    {
        private const float PickerMargin = 6.0f;
        private const float ChannelsMargin = 4.0f;
        private const float ChannelTextWidth = 12.0f;
        private const float SavedColorButtonWidth = 20.0f;
        private const float SavedColorButtonHeight = 20.0f;
        private const float TabHeight = 20;
        private const float ValueBoxesWidth = 100.0f;
        private const float HSVRGBTextWidth = 15.0f;
        private const float ColorPreviewHeight = 50.0f;
        private const int SavedColorsAmount = 10;

        private Color _initialValue;
        private Color _value;
        private bool _disableEvents;
        private bool _useDynamicEditing;
        private bool _activeEyedropper;
        private bool _canPassLastChangeEvent = true;
        private ColorValueBox.ColorPickerEvent _onChanged;
        private ColorValueBox.ColorPickerClosedEvent _onClosed;

        private ColorSelectorWithSliders _cSelector;
        private Tabs.Tabs _hsvRGBTabs;
        private Tab _RGBTab;
        private Panel _rgbPanel;
        private FloatValueBox _cRed;
        private FloatValueBox _cGreen;
        private FloatValueBox _cBlue;
        private Tab _hsvTab;
        private Panel _hsvPanel;
        private FloatValueBox _cHue;
        private FloatValueBox _cSaturation;
        private FloatValueBox _cValue;
        private TextBox _cHex;
        private FloatValueBox _cAlpha;
        private Button _cEyedropper;

        private List<Color> _savedColors = new List<Color>();
        private List<Button> _savedColorButtons = new List<Button>();

        /// <summary>
        /// Gets the selected color.
        /// </summary>
        public Color SelectedColor
        {
            get => _value;
            private set
            {
                if (_disableEvents || value == _value)
                    return;

                _disableEvents = true;
                _value = value;
                var clamped = Color.Min(Color.White, value);

                // Selector
                _cSelector.Color = _value;

                // RGBA
                _cRed.Value = _value.R;
                _cGreen.Value = _value.G;
                _cBlue.Value = _value.B;
                _cAlpha.Value = _value.A;

                // HSV
                var hsv = _value.ToHSV();
                _cHue.Value = hsv.X;
                _cSaturation.Value = hsv.Y * 100.0f;
                _cValue.Value = hsv.Z * 100.0f;

                // Hex
                _cHex.Text = clamped.ToHexString();

                // Dynamic value
                if (_useDynamicEditing)
                {
                    _onChanged?.Invoke(_value, true);
                }

                _disableEvents = false;
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ColorPickerDialog"/> class.
        /// </summary>
        /// <param name="initialValue">The initial value.</param>
        /// <param name="colorChanged">The color changed event.</param>
        /// <param name="pickerClosed">The close event.</param>
        /// <param name="useDynamicEditing">True if allow dynamic value editing (slider-like usage), otherwise will fire color change event only on editing end.</param>
        public ColorPickerDialog(Color initialValue, ColorValueBox.ColorPickerEvent colorChanged, ColorValueBox.ColorPickerClosedEvent pickerClosed, bool useDynamicEditing)
        : base("Pick a color!")
        {
            _initialValue = initialValue;
            _useDynamicEditing = useDynamicEditing;
            _value = Color.Transparent;
            _onChanged = colorChanged;
            _onClosed = pickerClosed;

            // Get saved colors if they exist
            if (Editor.Instance.ProjectCache.TryGetCustomData("ColorPickerSavedColors", out string savedColors))
                _savedColors = JsonSerializer.Deserialize<List<Color>>(savedColors);

            // Selector
            _cSelector = new ColorSelectorWithSliders(180, 21)
            {
                Location = new Float2(PickerMargin, PickerMargin),
                Parent = this
            };
            _cSelector.ColorChanged += x => SelectedColor = x;

            _hsvRGBTabs = new Tabs.Tabs
            {
                Location = new Float2(_cSelector.Right + 30.0f, PickerMargin),
                TabsTextHorizontalAlignment = TextAlignment.Center,
                Width = ValueBoxesWidth + HSVRGBTextWidth * 2.0f + ChannelsMargin * 2.0f,
                Height = (FloatValueBox.DefaultHeight + ChannelsMargin) * 4 + ChannelsMargin,
                Parent = this,
            };
            _hsvRGBTabs.TabsSize = new Float2(_hsvRGBTabs.Width * 0.5f, TabHeight);
            _hsvRGBTabs.SelectedTabChanged += SelectedTabChanged;

            // RGB Tab
            _RGBTab = _hsvRGBTabs.AddTab(new Tab("RGB"));
            _rgbPanel = new Panel(ScrollBars.Vertical)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                Parent = _RGBTab,
            };

            // HSV Tab
            _hsvTab = _hsvRGBTabs.AddTab(new Tab("HSV"));
            _hsvPanel = new Panel(ScrollBars.Vertical)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                Parent = _hsvTab,
            };

            // Red
            _cRed = new FloatValueBox(0, HSVRGBTextWidth + ChannelsMargin, PickerMargin, ValueBoxesWidth, 0, float.MaxValue, 0.001f)
            {
                Parent = _rgbPanel,
            };
            _cRed.ValueChanged += OnRGBAChanged;

            // Green
            _cGreen = new FloatValueBox(0, _cRed.X, _cRed.Bottom + ChannelsMargin, _cRed.Width, 0, float.MaxValue, 0.001f)
            {
                Parent = _rgbPanel,
            };
            _cGreen.ValueChanged += OnRGBAChanged;

            // Blue
            _cBlue = new FloatValueBox(0, _cRed.X, _cGreen.Bottom + ChannelsMargin, _cRed.Width, 0, float.MaxValue, 0.001f)
            {
                Parent = _rgbPanel,
            };
            _cBlue.ValueChanged += OnRGBAChanged;

            // Hue
            _cHue = new FloatValueBox(0, HSVRGBTextWidth + ChannelsMargin, PickerMargin, ValueBoxesWidth)
            {
                Parent = _hsvPanel,
                Category = Utils.ValueCategory.Angle,
            };
            _cHue.ValueChanged += OnHSVChanged;

            // Saturation
            _cSaturation = new FloatValueBox(0, _cHue.X, _cHue.Bottom + ChannelsMargin, _cHue.Width, 0, 100.0f, 0.1f)
            {
                Parent = _hsvPanel,
            };
            _cSaturation.ValueChanged += OnHSVChanged;

            // Value
            _cValue = new FloatValueBox(0, _cHue.X, _cSaturation.Bottom + ChannelsMargin, _cHue.Width, 0, float.MaxValue, 0.1f)
            {
                Parent = _hsvPanel,
            };
            _cValue.ValueChanged += OnHSVChanged;

            // Alpha
            _cAlpha = new FloatValueBox(0, _hsvRGBTabs.Left + HSVRGBTextWidth + ChannelsMargin, _hsvRGBTabs.Bottom + ChannelsMargin * 4.0f, ValueBoxesWidth, 0, float.MaxValue, 0.001f)
            {
                Parent = this,
            };
            _cAlpha.ValueChanged += OnRGBAChanged;

            // Hex
            _cHex = new TextBox(false, _hsvRGBTabs.Left + HSVRGBTextWidth + ChannelsMargin, _cAlpha.Bottom + ChannelsMargin * 2.0f, ValueBoxesWidth)
            {
                Parent = this,
            };
            _cHex.EditEnd += OnHexChanged;

            // Set valid dialog size based on UI content
            _dialogSize = Size = new Float2(_hsvRGBTabs.Right + PickerMargin, _cHex.Bottom + 40.0f + ColorPreviewHeight + PickerMargin);

            // Create saved color buttons
            CreateAllSavedColorsButtons();

            // Eyedropper button
            var style = Style.Current;
            _cEyedropper = new Button(_cSelector.BottomLeft.X, _cSelector.BottomLeft.Y - 25.0f, 25.0f, 25.0f)
            {
                TooltipText = "Eyedropper tool to pick a color directly from the screen.",
                BackgroundBrush = new SpriteBrush(Editor.Instance.Icons.Search32),
                BackgroundColor = style.Foreground,
                BackgroundColorHighlighted = style.Foreground.RGBMultiplied(0.9f),
                BorderColor = Color.Transparent,
                BorderColorHighlighted = style.BorderSelected,
                Parent = this,
            };
            _cEyedropper.Clicked += OnEyedropStart;

            // Set initial color
            SelectedColor = initialValue;
        }

        private void OnSavedColorButtonClicked(Button button)
        {
            if (button.Tag == null)
            {
                // Prevent setting same color 2 times... because why...
                foreach (var color in _savedColors)
                {
                    if (color == _value)
                    {
                        return;
                    }
                }

                // Set color of button to current value
                button.BackgroundColor = _value;
                button.BackgroundColorHighlighted = _value;
                button.BackgroundColorSelected = _value.RGBMultiplied(0.8f);
                button.Text = "";
                button.Tag = _value;

                // Save new colors
                _savedColors.Add(_value);
                var savedColors = JsonSerializer.Serialize(_savedColors, typeof(List<Color>));
                Editor.Instance.ProjectCache.SetCustomData("ColorPickerSavedColors", savedColors);

                // Create new + button
                if (_savedColorButtons.Count < SavedColorsAmount)
                {
                    var savedColorButton = new Button(PickerMargin * (_savedColorButtons.Count + 1) + SavedColorButtonWidth * _savedColorButtons.Count, _cHex.Bottom + 40.0f + ColorPreviewHeight * 0.5f, SavedColorButtonWidth, SavedColorButtonHeight)
                    {
                        Text = "+",
                        Parent = this,
                        TooltipText = "Save Color.",
                        Tag = null,
                    };
                    savedColorButton.ButtonClicked += (b) => OnSavedColorButtonClicked(b);
                    _savedColorButtons.Add(savedColorButton);
                }
            }
            else
            {
                SelectedColor = (Color)button.Tag;
            }
        }

        private void SelectedTabChanged(Tabs.Tabs tabs)
        {
            if (_rgbPanel == null || _hsvPanel == null)
                return;

            switch (tabs.SelectedTabIndex)
            {
                // RGB
                case 0:
                    _rgbPanel.Visible = true;
                    _hsvPanel.Visible = false;
                    break;
                // HSV
                case 1:
                    _rgbPanel.Visible = false;
                    _hsvPanel.Visible = true;
                    break;
            }
        }

        private void OnColorPicked(Color32 colorPicked)
        {
            if (_activeEyedropper)
            {
                _activeEyedropper = false;
                _cEyedropper.BackgroundColor = _cEyedropper.BackgroundColorHighlighted = Style.Current.Foreground;
                SelectedColor = colorPicked;
                ScreenUtilities.PickColorDone -= OnColorPicked;
            }
        }

        private void OnEyedropStart()
        {
            _activeEyedropper = true;
            _cEyedropper.BackgroundColor = _cEyedropper.BackgroundColorHighlighted = Style.Current.BackgroundHighlighted;
            ScreenUtilities.PickColor();
            ScreenUtilities.PickColorDone += OnColorPicked;
        }

        private void OnRGBAChanged()
        {
            if (_disableEvents)
                return;

            SelectedColor = new Color(_cRed.Value, _cGreen.Value, _cBlue.Value, _cAlpha.Value);
        }

        private void OnHSVChanged()
        {
            if (_disableEvents)
                return;

            _cHue.Value = Mathf.Wrap(_cHue.Value, 0f, 360f);
            SelectedColor = Color.FromHSV(_cHue.Value, _cSaturation.Value / 100.0f, _cValue.Value / 100.0f, _cAlpha.Value);
        }

        private void OnHexChanged()
        {
            if (_disableEvents)
                return;

            if (Color.TryParseHex(_cHex.Text, out Color color))
                SelectedColor = color;
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            base.Update(deltaTime);

            // Update eye dropper tool
            if (_activeEyedropper)
            {
                Float2 mousePosition = Platform.MousePosition;
                SelectedColor = ScreenUtilities.GetColorAt(mousePosition);
            }
        }

        /// <inheritdoc />
        public override void Draw()
        {
            var style = Style.Current;
            var textColor = style.Foreground;

            base.Draw();

            switch (_hsvRGBTabs.SelectedTabIndex)
            {
                // RGB
                case 0:
                    var rgbRect = new Rectangle(_hsvRGBTabs.Left + PickerMargin, _hsvRGBTabs.Top + TabHeight + PickerMargin, 10000, _cRed.Height);
                    Render2D.DrawText(style.FontMedium, "R", rgbRect, textColor, TextAlignment.Near, TextAlignment.Center);
                    rgbRect.Location.Y += _cRed.Height + ChannelsMargin;
                    Render2D.DrawText(style.FontMedium, "G", rgbRect, textColor, TextAlignment.Near, TextAlignment.Center);
                    rgbRect.Location.Y += _cRed.Height + ChannelsMargin;
                    Render2D.DrawText(style.FontMedium, "B", rgbRect, textColor, TextAlignment.Near, TextAlignment.Center);
                    break;
                // HSV
                case 1:
                    // Left
                    var hsvLeftRect = new Rectangle(_hsvRGBTabs.Left + PickerMargin, _hsvRGBTabs.Top + TabHeight + PickerMargin, 10000, _cHue.Height);
                    Render2D.DrawText(style.FontMedium, "H", hsvLeftRect, textColor, TextAlignment.Near, TextAlignment.Center);
                    hsvLeftRect.Location.Y += _cHue.Height + ChannelsMargin;
                    Render2D.DrawText(style.FontMedium, "S", hsvLeftRect, textColor, TextAlignment.Near, TextAlignment.Center);
                    hsvLeftRect.Location.Y += _cHue.Height + ChannelsMargin;
                    Render2D.DrawText(style.FontMedium, "V", hsvLeftRect, textColor, TextAlignment.Near, TextAlignment.Center);

                    // Right
                    var hsvRightRect = new Rectangle(_hsvRGBTabs.Right - HSVRGBTextWidth, _hsvRGBTabs.Top + TabHeight + PickerMargin, ChannelTextWidth, _cHue.Height);
                    Render2D.DrawText(style.FontMedium, "°", hsvRightRect, textColor, TextAlignment.Near, TextAlignment.Center);
                    hsvRightRect.Location.Y += _cHue.Height + ChannelsMargin;
                    Render2D.DrawText(style.FontMedium, "%", hsvRightRect, textColor, TextAlignment.Near, TextAlignment.Center);
                    hsvRightRect.Location.Y += _cHue.Height + ChannelsMargin;
                    Render2D.DrawText(style.FontMedium, "%", hsvRightRect, textColor, TextAlignment.Near, TextAlignment.Center);
                    break;
            }

            // A
            var alphaHexRect = new Rectangle(_hsvRGBTabs.Left + PickerMargin, _cAlpha.Top, ChannelTextWidth, _cAlpha.Height);
            Render2D.DrawText(style.FontMedium, "A", alphaHexRect, textColor, TextAlignment.Near, TextAlignment.Center);

            // Hex
            alphaHexRect.Y += _cAlpha.Height + ChannelsMargin * 2.0f;
            alphaHexRect.X -= 5.0f; // "Hex" is two characters wider than the other labels so we need to adjust for that
            Render2D.DrawText(style.FontMedium, "Hex", alphaHexRect, textColor, TextAlignment.Far, TextAlignment.Center);

            // Color difference
            var differenceRect = new Rectangle(_hsvRGBTabs.Left, _cHex.Bottom + 40.0f, _hsvRGBTabs.Width, ColorPreviewHeight);

            // Draw checkerboard for background of color to help with transparency
            Render2D.FillRectangle(differenceRect, Color.White);
            var smallRectSize = 10;
            var numHor = Mathf.CeilToInt(differenceRect.Width / smallRectSize);
            var numVer = Mathf.CeilToInt(differenceRect.Height / smallRectSize);
            Render2D.PushClip(differenceRect);
            for (int i = 0; i < numHor; i++)
            {
                for (int j = 0; j < numVer; j++)
                {
                    if ((i + j) % 2 == 0)
                    {
                        var rect = new Rectangle(differenceRect.X + smallRectSize * i, differenceRect.Y + smallRectSize * j, new Float2(smallRectSize));
                        Render2D.FillRectangle(rect, Color.Gray);
                    }
                }
            }
            Render2D.PopClip();
            Render2D.FillRectangle(differenceRect, _value);
        }

        /// <inheritdoc />
        protected override void OnShow()
        {
            // Apply changes on lost focus
#if !PLATFORM_LINUX
            ((WindowRootControl)Root).Window.LostFocus += OnSubmit;
#endif

            base.OnShow();
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            if (_activeEyedropper && key == KeyboardKeys.Escape)
            {
                // Cancel eye dropping
                _activeEyedropper = false;
                _cEyedropper.BackgroundColor = _cEyedropper.BackgroundColorHighlighted = Style.Current.Foreground;
                ScreenUtilities.PickColorDone -= OnColorPicked;
                return true;
            }

            return base.OnKeyDown(key);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (base.OnMouseUp(location, button))
                return true;

            var child = GetChildAtRecursive(location);
            if (button == MouseButton.Right && child is Button b && b.Tag is Color c)
            {
                // Show menu
                var menu = new ContextMenu.ContextMenu();
                var replaceButton = menu.AddButton("Replace");
                replaceButton.Clicked += () => OnSavedColorReplace(b);
                var deleteButton = menu.AddButton("Delete");
                deleteButton.Clicked += () => OnSavedColorDelete(b);
                _disableEvents = true;
                menu.Show(this, location);
                menu.VisibleChanged += (c) => _disableEvents = false;
                return true;
            }

            return false;
        }

        private void OnSavedColorReplace(Button button)
        {
            // Prevent setting same color 2 times... because why...
            foreach (var color in _savedColors)
            {
                if (color == _value)
                {
                    return;
                }
            }

            // Set new Color in spot
            for (int i = 0; i < _savedColors.Count; i++)
            {
                var color = _savedColors[i];
                if (color == (Color)button.Tag)
                {
                    color = _value;
                }
            }

            // Set color of button to current value;
            button.BackgroundColor = _value;
            button.BackgroundColorHighlighted = _value;
            button.Text = "";
            button.Tag = _value;

            // Save new colors
            var savedColors = JsonSerializer.Serialize(_savedColors, typeof(List<Color>));
            Editor.Instance.ProjectCache.SetCustomData("ColorPickerSavedColors", savedColors);
        }

        private void OnSavedColorDelete(Button button)
        {
            _savedColors.Remove((Color)button.Tag);

            foreach (var b in _savedColorButtons)
            {
                Children.Remove(b);
            }
            _savedColorButtons.Clear();

            CreateAllSavedColorsButtons();

            // Save new colors
            var savedColors = JsonSerializer.Serialize(_savedColors, typeof(List<Color>));
            Editor.Instance.ProjectCache.SetCustomData("ColorPickerSavedColors", savedColors);
        }

        private void CreateAllSavedColorsButtons()
        {
            // Create saved color buttons
            for (int i = 0; i < _savedColors.Count; i++)
            {
                var savedColor = _savedColors[i];
                var savedColorButton = new Button(PickerMargin * (i + 1) + SavedColorButtonWidth * i, _cHex.Bottom + 40.0f + ColorPreviewHeight * 0.5f, SavedColorButtonWidth, SavedColorButtonHeight)
                {
                    Parent = this,
                    Tag = savedColor,
                    BackgroundColor = savedColor,
                    BackgroundColorHighlighted = savedColor,
                    BackgroundColorSelected = savedColor.RGBMultiplied(0.8f),
                };
                savedColorButton.ButtonClicked += OnSavedColorButtonClicked;
                _savedColorButtons.Add(savedColorButton);
            }
            if (_savedColors.Count < SavedColorsAmount)
            {
                var savedColorButton = new Button(PickerMargin * (_savedColors.Count + 1) + SavedColorButtonWidth * _savedColors.Count, _cHex.Bottom + 40.0f + ColorPreviewHeight * 0.5f, SavedColorButtonWidth, SavedColorButtonHeight)
                {
                    Text = "+",
                    Parent = this,
                    TooltipText = "Save Color.",
                    Tag = null,
                };
                savedColorButton.ButtonClicked += OnSavedColorButtonClicked;
                _savedColorButtons.Add(savedColorButton);
            }
        }

        /// <inheritdoc />
        public override void OnSubmit()
        {
            if (_disableEvents)
                return;
            _disableEvents = true;

            // Ensure the cursor is restored
            Cursor = CursorType.Default;

            // Send color event if modified
            if (_value != _initialValue)
            {
                _onChanged?.Invoke(_value, false);
            }

            base.OnSubmit();
        }

        /// <inheritdoc />
        public override void OnCancel()
        {
            if (_disableEvents)
                return;
            _disableEvents = true;

            // Ensure the cursor is restored
            Cursor = CursorType.Default;

            // Restore color if modified
            if (_useDynamicEditing && _initialValue != _value && _canPassLastChangeEvent)
            {
                _canPassLastChangeEvent = false;
                _onChanged?.Invoke(_initialValue, false);
            }

            base.OnCancel();
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _onClosed?.Invoke();

            base.OnDestroy();
        }

        /// <inheritdoc />
        public void ClosePicker()
        {
            OnCancel();
        }
    }
}
