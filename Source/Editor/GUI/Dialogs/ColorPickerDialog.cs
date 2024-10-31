// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.Input;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Json;
using System.Collections.Generic;
using System.Linq;

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
        private const float ChannelsMargin = 4.0f;
        private const float ChannelTextWidth = 12.0f;
        private const float SavedColorButtonWidth = 20.0f;
        private const float SavedColorButtonHeight = 20.0f;
        private const float ColorPreviewWidth = 120.0f;
        private const float ColorPreviewHeight = 50.0f;
        private const float OldNewColorPreviewDisplayRatio = 0.3335f;
        private const float ColorWheelSize = 180.0f;
        private const float SaturationAlphaSlidersThickness = 20.0f;

        private const int maxSavedColorsAmount = 16; // Will begin new row when half is reached.

        private const float SmallMargin = 6.0f;
        private const float MediumMargin = 15.0f;
        private const float LargeMargin = 25.0f;
      
        private Margin _pickerMargin = new Margin(SmallMargin, SmallMargin, SmallMargin, SmallMargin);
        private Margin _SliderMargin = new Margin(0, LargeMargin + MediumMargin, 0, SmallMargin);
        private Margin _textBoxBlockMargin = new Margin(LargeMargin + 1.5f, SmallMargin + 1.5f, SmallMargin + 1.5f, SmallMargin + 1.5f);
        private Margin _colorPreviewMargin = new Margin(0, LargeMargin, 0, 0);

        private Color _initialValue;
        private Color _value;
        private bool _disableEvents;
        private bool _useDynamicEditing;
        private bool _isEyedropperActive;
        private ColorValueBox.ColorPickerEvent _onChanged;
        private ColorValueBox.ColorPickerClosedEvent _onClosed;

        private ColorSelectorWithSliders _cSelector;
        private Rectangle oldColorRect; // Needs to be defined here so that it can be accesed in OnMouseUp
        private FloatValueBox _cRed;
        private FloatValueBox _cGreen;
        private FloatValueBox _cBlue;
        private FloatValueBox _cAlpha;
        private FloatValueBox _cHue;
        private FloatValueBox _cSaturation;
        private FloatValueBox _cValue;
        private TextBox _cHex;
        private Button _cEyedropper;
        private Button _cOldPreviewButton;

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
                    _onChanged?.Invoke(_value, true);

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
            _cSelector = new ColorSelectorWithSliders(ColorWheelSize, SaturationAlphaSlidersThickness)
            {
                Location = new Float2(_pickerMargin.Left + _pickerMargin.Left, _pickerMargin.Top + _pickerMargin.Top),
                Parent = this
            };
            _cSelector.ColorChanged += x => SelectedColor = x;

            // Red
            _cRed = new FloatValueBox(0, _cSelector.Right + _SliderMargin.Right, _pickerMargin.Top + _pickerMargin.Top, 100, 0, float.MaxValue, 0.001f)
            {
                Parent = this
            };
            _cRed.ValueChanged += OnRGBAChanged;

            // Green
            _cGreen = new FloatValueBox(0, _cRed.X, _cRed.Bottom + ChannelsMargin, _cRed.Width, 0, float.MaxValue, 0.001f)
            {
                Parent = this
            };
            _cGreen.ValueChanged += OnRGBAChanged;

            // Blue
            _cBlue = new FloatValueBox(0, _cRed.X, _cGreen.Bottom + ChannelsMargin, _cRed.Width, 0, float.MaxValue, 0.001f)
            {
                Parent = this
            };
            _cBlue.ValueChanged += OnRGBAChanged;

            // Alpha
            _cAlpha = new FloatValueBox(0, _cRed.X, _cBlue.Bottom + ChannelsMargin, _cRed.Width, 0, float.MaxValue, 0.001f)
            {
                Parent = this
            };
            _cAlpha.ValueChanged += OnRGBAChanged;

            // Hex
            const float hexTextBoxWidth = 80;
            _cHex = new TextBox(false, _cRed.X + 20, _cAlpha.Bottom + _textBoxBlockMargin.Top, hexTextBoxWidth)
            {
                Parent = this
            };
            _cHex.EditEnd += OnHexChanged;

            // Hue
            _cHue = new FloatValueBox(0, _cSelector.Right + _SliderMargin.Right, _cHex.Bottom + _textBoxBlockMargin.Top, 100)
            {
                Parent = this
            };
            _cHue.ValueChanged += OnHSVChanged;

            // Saturation
            _cSaturation = new FloatValueBox(0, _cHue.X, _cHue.Bottom + ChannelsMargin, _cHue.Width, 0, 100.0f, 0.1f)
            {
                Parent = this
            };
            _cSaturation.ValueChanged += OnHSVChanged;

            // Value
            _cValue = new FloatValueBox(0, _cHue.X, _cSaturation.Bottom + ChannelsMargin, _cHue.Width, 0, float.MaxValue, 0.1f)
            {
                Parent = this
            };
            _cValue.ValueChanged += OnHSVChanged;

            // Set valid dialog size based on UI content
            _dialogSize = Size = new Float2(_cRed.Right + _pickerMargin.Right + 20, 300); // +20 to account for hsv math symbols

            CreateSavedColorButtons();

            // Eyedropper button
            var style = Style.Current;
            _cEyedropper = new Button(_cSelector.BottomLeft.X, _cSelector.BottomLeft.Y)
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
            _cEyedropper.Width = _cEyedropper.Height;
            _cEyedropper.Location = new Float2(_cEyedropper.Location.X, _cEyedropper.Location.Y - _cEyedropper.Height);

            // Set initial color
            SelectedColor = initialValue;
        }

        private void OnSavedColorButtonClicked(Button button)
        {
            if (button.Tag == null)
            {
                // Prevent setting same color 2 times... cause why...
                if (_savedColors.Any(c => c == _value))
                    return;
                
                // Set color of button to current value;
                button.BackgroundColor = _value;
                button.BackgroundColorHighlighted = _value;
                button.BackgroundColorSelected = _value.RGBMultiplied(0.8f);
                button.Text = "";
                button.Tag = _value;

                // Save new colors
                _savedColors.Add(_value);
                var savedColors = JsonSerializer.Serialize(_savedColors, typeof(List<Color>));
                Editor.Instance.ProjectCache.SetCustomData("ColorPickerSavedColors", savedColors);

                float savedColorsYPosition = _cValue.Bottom - _textBoxBlockMargin.Bottom + 50;

                // Create new + button
                if (_savedColorButtons.Count < maxSavedColorsAmount)
                {
                    bool savedGreaterThanHalfMax = _savedColors.Count + 1 > maxSavedColorsAmount / 2;

                    float buttonYPosition = savedGreaterThanHalfMax ? savedColorsYPosition + SavedColorButtonHeight + SmallMargin : savedColorsYPosition;
                    float buttonXPositionOffset = savedGreaterThanHalfMax ? (SavedColorButtonWidth + SmallMargin) * Mathf.Abs(maxSavedColorsAmount / 2 - _savedColors.Count) : (SavedColorButtonWidth + SmallMargin) * _savedColors.Count;

                    var savedColorButton = new Button(_cSelector.Location.X + buttonXPositionOffset, buttonYPosition, SavedColorButtonWidth, SavedColorButtonHeight)
                    {
                        Text = "+",
                        Parent = this,
                        TooltipText = "Save the currently selected color.",
                        Tag = null,
                    };
                    savedColorButton.ButtonClicked += (b) => OnSavedColorButtonClicked(b);
                    _savedColorButtons.Add(savedColorButton);
                }
            }
            else
                SelectedColor = (Color)button.Tag;
        }

        private void OnColorPicked(Color32 colorPicked)
        {
            if (_isEyedropperActive)
            {
                _isEyedropperActive = false;

                // Reset colors
                _cEyedropper.BackgroundColor = Style.Current.Foreground;
                _cEyedropper.BorderColor = Color.Transparent;

                SelectedColor = colorPicked;
                ScreenUtilities.PickColorDone -= OnColorPicked;
            }
        }

        private void OnEyedropStart()
        {
            _isEyedropperActive = true;

            // Provide some visual feedback that the eyedropper is active by changing button colors
            _cEyedropper.BackgroundColor = _cEyedropper.BackgroundColorSelected;
            _cEyedropper.BorderColor = _cEyedropper.BorderColorSelected;

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
            if (_isEyedropperActive)
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

            // RGBA
            var rgbaR = new Rectangle(_cRed.Left - ChannelTextWidth, _cRed.Y, 10000, _cRed.Height);
            Render2D.DrawText(style.FontMedium, "R", rgbaR, textColor, TextAlignment.Near, TextAlignment.Center);
            rgbaR.Location.Y = _cGreen.Y;
            Render2D.DrawText(style.FontMedium, "G", rgbaR, textColor, TextAlignment.Near, TextAlignment.Center);
            rgbaR.Location.Y = _cBlue.Y;
            Render2D.DrawText(style.FontMedium, "B", rgbaR, textColor, TextAlignment.Near, TextAlignment.Center);
            rgbaR.Location.Y = _cAlpha.Y;
            Render2D.DrawText(style.FontMedium, "A", rgbaR, textColor, TextAlignment.Near, TextAlignment.Center);

            // HSV letters (left)
            var hsvHl = new Rectangle(_cHue.Left - ChannelTextWidth, _cHue.Y, 10000, _cHue.Height);
            Render2D.DrawText(style.FontMedium, "H", hsvHl, textColor, TextAlignment.Near, TextAlignment.Center);
            hsvHl.Location.Y = _cSaturation.Y;
            Render2D.DrawText(style.FontMedium, "S", hsvHl, textColor, TextAlignment.Near, TextAlignment.Center);
            hsvHl.Location.Y = _cValue.Y;
            Render2D.DrawText(style.FontMedium, "V", hsvHl, textColor, TextAlignment.Near, TextAlignment.Center);

            // HSV math symbols (right)
            var hsvHr = new Rectangle(_cHue.Right + 2, _cHue.Y, 10000, _cHue.Height);
            Render2D.DrawText(style.FontMedium, "°", hsvHr, textColor, TextAlignment.Near, TextAlignment.Center);
            hsvHr.Location.Y = _cSaturation.Y;
            Render2D.DrawText(style.FontMedium, "%", hsvHr, textColor, TextAlignment.Near, TextAlignment.Center);
            hsvHr.Location.Y = _cValue.Y;
            Render2D.DrawText(style.FontMedium, "%", hsvHr, textColor, TextAlignment.Near, TextAlignment.Center);

            // Hex
            var hex = new Rectangle(_cHex.Left - 26, _cHex.Y, 10000, _cHex.Height);
            Render2D.DrawText(style.FontMedium, "Hex", hex, textColor, TextAlignment.Near, TextAlignment.Center);

            // Old and New color preview
            float oldNewColorPreviewYPosition = _cValue.Bottom - _textBoxBlockMargin.Bottom + 50;

            oldColorRect = new Rectangle(Width - ColorPreviewWidth - _pickerMargin.Right - 20, oldNewColorPreviewYPosition, ColorPreviewWidth * OldNewColorPreviewDisplayRatio, 0);
            oldColorRect.Size.Y = ColorPreviewHeight;

            var newColorRect = new Rectangle(oldColorRect.Right, oldNewColorPreviewYPosition, ColorPreviewWidth * (1 - OldNewColorPreviewDisplayRatio), 0);
            newColorRect.Size.Y = ColorPreviewHeight;

            // Generate the button for right click detection here because we don't know position of oldColorRect before
            _cOldPreviewButton = new Button(oldColorRect.Location, oldColorRect.Size)
            {
                TooltipText = "Right click to use this color.",
                Tag = "OldColorPreview",
                BackgroundColor = Color.Transparent,
                BackgroundColorHighlighted = Color.Transparent,
                BackgroundColorSelected = Color.Transparent,
                BorderColor = Color.Transparent,
                BorderColorHighlighted = Color.Transparent,
                BorderColorSelected = Color.Transparent,
                Parent = this
            };

            const int smallRectSize = 10;

            // Only generate and draw checkerboard if old or new color has transparency.
            if (_initialValue.A < 1 || _value.A < 1)
            {
                // Checkerboard background
                var alphaBackground = new Rectangle(oldColorRect.Left, oldNewColorPreviewYPosition, ColorPreviewWidth, ColorPreviewHeight);

                // Draw checkerboard for background of color preview to help with transparency
                Render2D.FillRectangle(alphaBackground, Color.White);
                
                var numHor = Mathf.FloorToInt(ColorPreviewWidth / smallRectSize);
                var numVer = Mathf.FloorToInt(oldColorRect.Height / smallRectSize);
                for (int i = 0; i < numHor; i++)
                {
                    for (int j = 0; j < numVer; j++)
                    {
                        if ((i + j) % 2 == 0)
                        {
                            var rect = new Rectangle(oldColorRect.X + smallRectSize * i, oldColorRect.Y + smallRectSize * j, new Float2(smallRectSize));
                            Render2D.FillRectangle(rect, Color.Gray);
                        }
                    }
                }
            }
            
            Vector2 textOffset = new Vector2(0, -15);
            Render2D.DrawText(style.FontMedium, "Old", Color.White, oldColorRect.UpperLeft + textOffset);
            Render2D.DrawText(style.FontMedium, "New", Color.White, newColorRect.UpperLeft + textOffset);

            Render2D.FillRectangle(oldColorRect, _initialValue);
            Render2D.FillRectangle(newColorRect, _value);

            // Small separators between Old and New
            Float2 separatorOffset = new Vector2(0, smallRectSize);
            Render2D.DrawLine(oldColorRect.UpperRight, oldColorRect.UpperRight + separatorOffset / 2, Style.Current.Background, 2);
            Render2D.DrawLine(oldColorRect.BottomRight, oldColorRect.BottomRight - separatorOffset / 2, Style.Current.Background, 2);
        }

        /// <inheritdoc />
        protected override void OnShow()
        {
            // Auto cancel on lost focus
#if !PLATFORM_LINUX
            ((WindowRootControl)Root).Window.LostFocus += OnCancel;
#endif

            base.OnShow();
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            if (_isEyedropperActive && key == KeyboardKeys.Escape)
            {
                // Cancel eye dropping
                _isEyedropperActive = false;
                _cEyedropper.BackgroundColor = Style.Current.Foreground;
                _cEyedropper.BorderColor = Color.Transparent;
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

            if (button == MouseButton.Right && oldColorRect.Contains(location))
            {
                var menu = new ContextMenu.ContextMenu();
                var useButton = menu.AddButton("Use");
                useButton.Clicked += () => OnResetToOldPreviewPressed(useButton);
                _disableEvents = true;
                menu.Show(this, location);
                menu.VisibleChanged += (c) => _disableEvents = false;
                return true;
            }

            return false;
        }

        private void OnResetToOldPreviewPressed(ContextMenuButton button)
        {
            _cSelector.Color = _initialValue;
        }

        private void OnSavedColorReplace(Button button)
        {
            // Prevent setting same color 2 times... because why...
            foreach (var color in _savedColors)
            {
                if (color == _value)
                    return;
            }

            // Set new Color in spot
            for (int i = 0; i < _savedColors.Count; i++)
            {
                var color = _savedColors[i];
                if (color == (Color)button.Tag)
                    color = _value;
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

            CreateSavedColorButtons();

            // Save new colors
            var savedColors = JsonSerializer.Serialize(_savedColors, typeof(List<Color>));
            Editor.Instance.ProjectCache.SetCustomData("ColorPickerSavedColors", savedColors);
        }

        private void CreateSavedColorButtons()
        {
            float savedColorsYPosition = _cValue.Bottom - _textBoxBlockMargin.Bottom + 50;

            // Generated buttons saved colors
            for (int i = 0; i < _savedColors.Count; i++)
            {
                bool savedGreaterThanHalfMax = i + 1 > maxSavedColorsAmount / 2;

                float buttonYPosition = savedGreaterThanHalfMax ? savedColorsYPosition + SavedColorButtonHeight + SmallMargin : savedColorsYPosition;
                float buttonXPositionOffset = savedGreaterThanHalfMax ? (SavedColorButtonWidth + SmallMargin) * Mathf.Abs(maxSavedColorsAmount / 2 - i) : (SavedColorButtonWidth + SmallMargin) * i;

                var savedColor = _savedColors[i];           
                var savedColorButton = new Button(_cSelector.Location.X + buttonXPositionOffset, buttonYPosition, SavedColorButtonWidth, SavedColorButtonHeight)
                {
                    Parent = this,
                    Tag = savedColor,
                    BackgroundColor = savedColor,
                    BackgroundColorHighlighted = savedColor,
                    TooltipText = $"Saved color {i + 1}.",
                    BackgroundColorSelected = savedColor.RGBMultiplied(0.8f),
                };
                savedColorButton.ButtonClicked += (b) => OnSavedColorButtonClicked(b);
                _savedColorButtons.Add(savedColorButton);
            }

            // Create "+" button to save color
            if (_savedColors.Count < maxSavedColorsAmount)
            {
                bool savedGreaterThanHalfMax = _savedColors.Count + 1 > maxSavedColorsAmount / 2;

                float buttonYPosition = savedGreaterThanHalfMax ? savedColorsYPosition + SavedColorButtonHeight + SmallMargin : savedColorsYPosition;
                float buttonXPositionOffset = savedGreaterThanHalfMax ? (SavedColorButtonWidth + SmallMargin) * Mathf.Abs(maxSavedColorsAmount / 2 - _savedColors.Count) : (SavedColorButtonWidth + SmallMargin) * _savedColors.Count;

                var savedColorButton = new Button(_cSelector.Location.X + buttonXPositionOffset, buttonYPosition, SavedColorButtonWidth, SavedColorButtonHeight)
                {
                    Text = "+",
                    Parent = this,
                    TooltipText = $"Saved color {_savedColors.Count}.",
                    Tag = null,
                };
                savedColorButton.ButtonClicked += (b) => OnSavedColorButtonClicked(b);
                _savedColorButtons.Add(savedColorButton);
            }
        }

        /// <inheritdoc />
        public override void OnCancel()
        {
            if (_disableEvents)
                return;

            OnSubmit();

            base.OnCancel();
        }

        /// <inheritdoc />
        public override void OnSubmit()
        {     
            if (_disableEvents)
                return;
            _disableEvents = true;

            // Send color event if modified
            if (_value != _initialValue)
                _onChanged?.Invoke(_value, false);

            // Ensure mouse cursor is reset to default
            Cursor = CursorType.Default;

            base.OnSubmit(); 
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            OnSubmit();

            base.OnDestroy();
        }

        /// <inheritdoc />
        public void ClosePicker()
        {
            OnSubmit();
        }
    }
}
