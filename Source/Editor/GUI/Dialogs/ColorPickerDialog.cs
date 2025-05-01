// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.GUI.Input;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Json;
using System.Collections.Generic;

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
        private const float ButtonsWidth = 60.0f;
        private const float PickerMargin = 6.0f;
        private const float EyedropperMargin = 8.0f;
        private const float RGBAMargin = 12.0f;
        private const float HSVMargin = 0.0f;
        private const float ChannelsMargin = 4.0f;
        private const float ChannelTextWidth = 12.0f;
        private const float SavedColorButtonWidth = 20.0f;
        private const float SavedColorButtonHeight = 20.0f;

        private Color _initialValue;
        private Color _value;
        private bool _disableEvents;
        private bool _useDynamicEditing;
        private bool _activeEyedropper;
        private bool _canPassLastChangeEvent = true;
        private ColorValueBox.ColorPickerEvent _onChanged;
        private ColorValueBox.ColorPickerClosedEvent _onClosed;

        private ColorSelectorWithSliders _cSelector;
        private FloatValueBox _cRed;
        private FloatValueBox _cGreen;
        private FloatValueBox _cBlue;
        private FloatValueBox _cAlpha;
        private FloatValueBox _cHue;
        private FloatValueBox _cSaturation;
        private FloatValueBox _cValue;
        private TextBox _cHex;
        private Button _cCancel;
        private Button _cOK;
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
            _cSelector = new ColorSelectorWithSliders(180, 18)
            {
                Location = new Float2(PickerMargin, PickerMargin),
                Parent = this
            };
            _cSelector.ColorChanged += x => SelectedColor = x;

            // Red
            _cRed = new FloatValueBox(0, _cSelector.Right + PickerMargin + RGBAMargin + ChannelTextWidth, PickerMargin, 100, 0, float.MaxValue, 0.001f)
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

            // Hue
            _cHue = new FloatValueBox(0, PickerMargin + HSVMargin + ChannelTextWidth, _cSelector.Bottom + PickerMargin, 100, 0, 360)
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
            _dialogSize = Size = new Float2(_cRed.Right + PickerMargin, 300);

            // Hex
            const float hexTextBoxWidth = 80;
            _cHex = new TextBox(false, Width - hexTextBoxWidth - PickerMargin, _cSelector.Bottom + PickerMargin, hexTextBoxWidth)
            {
                Parent = this
            };
            _cHex.EditEnd += OnHexChanged;

            // Cancel
            _cCancel = new Button(Width - ButtonsWidth - PickerMargin, Height - Button.DefaultHeight - PickerMargin, ButtonsWidth)
            {
                Text = "Cancel",
                Parent = this
            };
            _cCancel.Clicked += OnCancel;

            // OK
            _cOK = new Button(_cCancel.Left - ButtonsWidth - PickerMargin, _cCancel.Y, ButtonsWidth)
            {
                Text = "Ok",
                Parent = this
            };
            _cOK.Clicked += OnSubmit;

            // Create saved color buttons
            CreateAllSaveButtons();

            // Eyedropper button
            var style = Style.Current;
            _cEyedropper = new Button(_cOK.X - EyedropperMargin, _cHex.Bottom + PickerMargin)
            {
                TooltipText = "Eyedropper tool to pick a color directly from the screen",
                BackgroundBrush = new SpriteBrush(Editor.Instance.Icons.Search32),
                BackgroundColor = style.Foreground,
                BackgroundColorHighlighted = style.Foreground.RGBMultiplied(0.9f),
                BorderColor = Color.Transparent,
                BorderColorHighlighted = style.BorderSelected,
                Parent = this,
            };
            _cEyedropper.Clicked += OnEyedropStart;
            _cEyedropper.Height = (_cValue.Bottom - _cEyedropper.Y) * 0.5f;
            _cEyedropper.Width = _cEyedropper.Height;
            _cEyedropper.X -= _cEyedropper.Width;

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

                // create new + button
                if (_savedColorButtons.Count < 8)
                {
                    var savedColorButton = new Button(PickerMargin * (_savedColorButtons.Count + 1) + SavedColorButtonWidth * _savedColorButtons.Count, Height - SavedColorButtonHeight - PickerMargin, SavedColorButtonWidth, SavedColorButtonHeight)
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

        private void OnColorPicked(Color32 colorPicked)
        {
            if (_activeEyedropper)
            {
                _activeEyedropper = false;
                SelectedColor = colorPicked;
                ScreenUtilities.PickColorDone -= OnColorPicked;
            }
        }

        private void OnEyedropStart()
        {
            _activeEyedropper = true;
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

            // RGBA
            var rgbaR = new Rectangle(_cRed.Left - ChannelTextWidth, _cRed.Y, 10000, _cRed.Height);
            Render2D.DrawText(style.FontMedium, "R", rgbaR, textColor, TextAlignment.Near, TextAlignment.Center);
            rgbaR.Location.Y = _cGreen.Y;
            Render2D.DrawText(style.FontMedium, "G", rgbaR, textColor, TextAlignment.Near, TextAlignment.Center);
            rgbaR.Location.Y = _cBlue.Y;
            Render2D.DrawText(style.FontMedium, "B", rgbaR, textColor, TextAlignment.Near, TextAlignment.Center);
            rgbaR.Location.Y = _cAlpha.Y;
            Render2D.DrawText(style.FontMedium, "A", rgbaR, textColor, TextAlignment.Near, TextAlignment.Center);

            // HSV left
            var hsvHl = new Rectangle(_cHue.Left - ChannelTextWidth, _cHue.Y, 10000, _cHue.Height);
            Render2D.DrawText(style.FontMedium, "H", hsvHl, textColor, TextAlignment.Near, TextAlignment.Center);
            hsvHl.Location.Y = _cSaturation.Y;
            Render2D.DrawText(style.FontMedium, "S", hsvHl, textColor, TextAlignment.Near, TextAlignment.Center);
            hsvHl.Location.Y = _cValue.Y;
            Render2D.DrawText(style.FontMedium, "V", hsvHl, textColor, TextAlignment.Near, TextAlignment.Center);

            // HSV right
            var hsvHr = new Rectangle(_cHue.Right + 2, _cHue.Y, 10000, _cHue.Height);
            Render2D.DrawText(style.FontMedium, "°", hsvHr, textColor, TextAlignment.Near, TextAlignment.Center);
            hsvHr.Location.Y = _cSaturation.Y;
            Render2D.DrawText(style.FontMedium, "%", hsvHr, textColor, TextAlignment.Near, TextAlignment.Center);
            hsvHr.Location.Y = _cValue.Y;
            Render2D.DrawText(style.FontMedium, "%", hsvHr, textColor, TextAlignment.Near, TextAlignment.Center);

            // Hex
            var hex = new Rectangle(_cHex.Left - 26, _cHex.Y, 10000, _cHex.Height);
            Render2D.DrawText(style.FontMedium, "Hex", hex, textColor, TextAlignment.Near, TextAlignment.Center);

            // Color difference
            var newRect = new Rectangle(_cOK.X - 3, _cHex.Bottom + PickerMargin, 130, 0);
            newRect.Size.Y = 50;
            Render2D.FillRectangle(newRect, Color.White);
            var smallRectSize = 10;
            var numHor = Mathf.FloorToInt(newRect.Width / smallRectSize);
            var numVer = Mathf.FloorToInt(newRect.Height / smallRectSize);
            // Draw checkerboard for background of color to help with transparency
            for (int i = 0; i < numHor; i++)
            {
                for (int j = 0; j < numVer; j++)
                {
                    if ((i + j) % 2 == 0)
                    {
                        var rect = new Rectangle(newRect.X + smallRectSize * i, newRect.Y + smallRectSize * j, new Float2(smallRectSize));
                        Render2D.FillRectangle(rect, Color.Gray);
                    }
                }
            }
            Render2D.FillRectangle(newRect, _value);
        }

        /// <inheritdoc />
        protected override void OnShow()
        {
            // Auto cancel on lost focus
#if !PLATFORM_LINUX
            ((WindowRootControl)Root).Window.LostFocus += OnWindowLostFocus;
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
                ScreenUtilities.PickColorDone -= OnColorPicked;
                return true;
            }

            return base.OnKeyDown(key);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (base.OnMouseUp(location, button))
            {
                return true;
            }

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

            CreateAllSaveButtons();

            // Save new colors
            var savedColors = JsonSerializer.Serialize(_savedColors, typeof(List<Color>));
            Editor.Instance.ProjectCache.SetCustomData("ColorPickerSavedColors", savedColors);
        }

        private void CreateAllSaveButtons()
        {
            // Create saved color buttons
            for (int i = 0; i < _savedColors.Count; i++)
            {
                var savedColor = _savedColors[i];
                var savedColorButton = new Button(PickerMargin * (i + 1) + SavedColorButtonWidth * i, Height - SavedColorButtonHeight - PickerMargin, SavedColorButtonWidth, SavedColorButtonHeight)
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
            if (_savedColors.Count < 8)
            {
                var savedColorButton = new Button(PickerMargin * (_savedColors.Count + 1) + SavedColorButtonWidth * _savedColors.Count, Height - SavedColorButtonHeight - PickerMargin, SavedColorButtonWidth, SavedColorButtonHeight)
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

        private void OnWindowLostFocus()
        {
            // Auto apply color on defocus
            var autoAcceptColorPickerChange = Editor.Instance.Options.Options.Interface.AutoAcceptColorPickerChange;
            if (_useDynamicEditing && _initialValue != _value && _canPassLastChangeEvent && autoAcceptColorPickerChange)
            {
                _canPassLastChangeEvent = false;
                _onChanged?.Invoke(_value, false);
            }

            OnCancel();
        }

        /// <inheritdoc />
        public override void OnSubmit()
        {
            if (_disableEvents)
                return;
            _disableEvents = true;

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
