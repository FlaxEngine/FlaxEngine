// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using FlaxEditor.GUI.Input;
using FlaxEngine;
using FlaxEngine.GUI;

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
        private const float RGBAMargin = 12.0f;
        private const float HSVMargin = 0.0f;
        private const float ChannelsMargin = 4.0f;
        private const float ChannelTextWidth = 12.0f;

        private Color _initialValue;
        private Color _value;
        private bool _disableEvents;
        private bool _useDynamicEditing;
        private bool _isClosing;
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

        /// <summary>
        /// Gets the selected color.
        /// </summary>
        /// <value>
        /// The color.
        /// </value>
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

            // Selector
            _cSelector = new ColorSelectorWithSliders(180, 18)
            {
                Location = new Vector2(PickerMargin, PickerMargin),
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
            _dialogSize = Size = new Vector2(_cRed.Right + PickerMargin, 300);

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
            _cCancel.Clicked += OnCancelClicked;

            // OK
            _cOK = new Button(_cCancel.Left - ButtonsWidth - PickerMargin, _cCancel.Y, ButtonsWidth)
            {
                Text = "Ok",
                Parent = this
            };
            _cOK.Clicked += OnOkClicked;

            // Set initial color
            SelectedColor = initialValue;
        }

        private void OnOkClicked()
        {
            if (_isClosing)
                return;
            _isClosing = true;

            // Send color event if modified
            if (_value != _initialValue)
            {
                _onChanged?.Invoke(_value, false);
            }

            Close(DialogResult.OK);
        }

        private void OnCancelClicked()
        {
            if (_isClosing)
                return;
            _isClosing = true;

            // Restore color if modified
            if (_useDynamicEditing && _initialValue != _value)
            {
                _onChanged?.Invoke(_initialValue, false);
            }

            Close(DialogResult.Cancel);
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
            var newRect = new Rectangle(_cOK.X, _cHex.Bottom + PickerMargin, _cCancel.Right - _cOK.Left, 0);
            newRect.Size.Y = _cValue.Bottom - newRect.Y;
            Render2D.FillRectangle(newRect, _value * _value.A);
        }

        /// <inheritdoc />
        protected override void OnShow()
        {
            // Auto cancel on lost focus
            ((WindowRootControl)Root).Window.LostFocus += OnCancelClicked;

            base.OnShow();
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
            OnCancelClicked();
        }
    }
}
