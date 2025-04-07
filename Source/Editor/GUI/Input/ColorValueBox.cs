// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.GUI.Dialogs;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI.Input
{
    /// <summary>
    /// Color value editor with picking support.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.Control" />
    [HideInEditor]
    public class ColorValueBox : Control
    {
        private bool _isMouseDown;

        /// <summary>
        /// Delegate function used for the color picker events handling.
        /// </summary>
        /// <param name="color">The selected color.</param>
        /// <param name="sliding">True if user is using a slider, otherwise false.</param>
        public delegate void ColorPickerEvent(Color color, bool sliding);

        /// <summary>
        /// Delegate function used for the color picker close event handling.
        /// </summary>
        public delegate void ColorPickerClosedEvent();

        /// <summary>
        /// Delegate function used to handle showing color picking dialog.
        /// </summary>
        /// <param name="targetControl">The GUI control that invokes the picker.</param>
        /// <param name="initialValue">The initial value.</param>
        /// <param name="colorChanged">The color changed event.</param>
        /// <param name="pickerClosed">The color editing end event.</param>
        /// <param name="useDynamicEditing">True if allow dynamic value editing (slider-like usage), otherwise will fire color change event only on editing end.</param>
        /// <returns>The created color picker dialog or null if failed.</returns>
        public delegate IColorPickerDialog ShowPickColorDialogDelegate(Control targetControl, Color initialValue, ColorPickerEvent colorChanged, ColorPickerClosedEvent pickerClosed = null, bool useDynamicEditing = true);

        /// <summary>
        /// Shows picking color dialog (see <see cref="ShowPickColorDialogDelegate"/>).
        /// </summary>
        public static ShowPickColorDialogDelegate ShowPickColorDialog;

        /// <summary>
        /// The current opened dialog.
        /// </summary>
        protected IColorPickerDialog _currentDialog;

        /// <summary>
        /// True if slider is in use.
        /// </summary>
        protected bool _isSliding;

        /// <summary>
        /// The value.
        /// </summary>
        protected Color _value;

        /// <summary>
        /// Enables live preview of the selected value from the picker. Otherwise will update the value only when user confirms it on dialog closing.
        /// </summary>
        public bool UseDynamicEditing = true;

        /// <summary>
        /// Occurs when value gets changed.
        /// </summary>
        public event Action ValueChanged;

        /// <summary>
        /// Occurs when value gets changed.
        /// </summary>
        public event Action<ColorValueBox> ColorValueChanged;

        /// <summary>
        /// Gets or sets the color value.
        /// </summary>
        public Color Value
        {
            get => _value;
            set
            {
                if (_value != value)
                {
                    _value = value;
                    OnValueChanged();
                }
            }
        }

        /// <summary>
        /// Gets a value indicating whether user is using a slider.
        /// </summary>
        public bool IsSliding => _isSliding;

        /// <summary>
        /// Initializes a new instance of the <see cref="ColorValueBox"/> class.
        /// </summary>
        public ColorValueBox()
        : base(0, 0, 32, 18)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ColorValueBox"/> class.
        /// </summary>
        /// <param name="value">The initial value.</param>
        /// <param name="x">The x location</param>
        /// <param name="y">The y location</param>
        public ColorValueBox(Color value, float x, float y)
        : base(x, y, 32, 18)
        {
            _value = value;
        }

        /// <summary>
        /// Called when value gets changed.
        /// </summary>
        protected virtual void OnValueChanged()
        {
            ValueChanged?.Invoke();
            ColorValueChanged?.Invoke(this);
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            var style = Style.Current;
            var r = new Rectangle(0, 0, Width, Height);

            Render2D.FillRectangle(r, _value);
            Render2D.DrawRectangle(r, IsMouseOver || IsNavFocused ? style.BackgroundSelected : Color.Black);
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            _isMouseDown = true;
            return base.OnMouseDown(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (_isMouseDown)
            {
                _isMouseDown = false;
                Focus();
                OnSubmit();
            }
            return true;
        }

        /// <inheritdoc />
        public override void OnSubmit()
        {
            base.OnSubmit();

            // Show color picker dialog
            _currentDialog = ShowPickColorDialog?.Invoke(this, _value, OnColorChanged, OnPickerClosed, UseDynamicEditing);
        }

        private void OnColorChanged(Color color, bool sliding)
        {
            // Force send ValueChanged event is sliding state gets modified by the color picker (e.g the color picker window closing event)
            if (_isSliding != sliding)
            {
                _isSliding = sliding;
                _value = color;
                OnValueChanged();
            }
            else
            {
                Value = color;
            }
        }

        private void OnPickerClosed()
        {
            _currentDialog = null;
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            if (_currentDialog != null)
            {
                _currentDialog.ClosePicker();
                _currentDialog = null;
            }

            base.OnDestroy();
        }
    }
}
