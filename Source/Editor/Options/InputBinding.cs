// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Globalization;
using FlaxEditor.CustomEditors;
using FlaxEditor.CustomEditors.Elements;
using FlaxEngine;
using FlaxEngine.GUI;
using static FlaxEditor.Viewport.EditorViewport;

namespace FlaxEditor.Options
{
    /// <summary>
    /// The input binding container.
    /// </summary>
    [Serializable]
    [HideInEditor]
    [TypeConverter(typeof(InputBindingConverter))]
    [CustomEditor(typeof(InputBindingEditor))]
    public struct InputBinding : IEquatable<InputBinding>
    {
        /// <summary>
        /// The key to bind.
        /// </summary>
        public KeyboardKeys Key;

        /// <summary>
        /// The first modifier (<see cref="KeyboardKeys.None"/> if not used).
        /// </summary>
        public KeyboardKeys Modifier1;

        /// <summary>
        /// The second modifier (<see cref="KeyboardKeys.None"/> if not used).
        /// </summary>
        public KeyboardKeys Modifier2;

        /// <summary>
        /// Initializes a new instance of the <see cref="InputBinding"/> struct.
        /// </summary>
        /// <param name="key">The key.</param>
        public InputBinding(KeyboardKeys key)
        {
            Key = key;
            Modifier1 = KeyboardKeys.None;
            Modifier2 = KeyboardKeys.None;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="InputBinding"/> struct.
        /// </summary>
        /// <param name="key">The key.</param>
        /// <param name="modifier1">The first modifier.</param>
        public InputBinding(KeyboardKeys key, KeyboardKeys modifier1)
        {
            Key = key;
            Modifier1 = modifier1;
            Modifier2 = KeyboardKeys.None;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="InputBinding"/> struct.
        /// </summary>
        /// <param name="key">The key.</param>
        /// <param name="modifier1">The first modifier.</param>
        /// <param name="modifier2">The second modifier.</param>
        public InputBinding(KeyboardKeys key, KeyboardKeys modifier1, KeyboardKeys modifier2)
        {
            Key = key;
            Modifier1 = modifier1;
            Modifier2 = modifier2;
        }

        /// <summary>
        /// Parses the specified key text value.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <param name="result">The result (valid only if parsing succeed).</param>
        /// <returns>True if parsing succeed, otherwise false.</returns>
        public static bool Parse(string value, out KeyboardKeys result)
        {
            if (string.Equals(value, "Ctrl", StringComparison.OrdinalIgnoreCase))
            {
                result = KeyboardKeys.Control;
                return true;
            }

            return Enum.TryParse(value, true, out result);
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents the key enum (for UI).
        /// </summary>
        /// <param name="key">The key.</param>
        /// <returns>A <see cref="System.String" /> that represents the key.</returns>
        public static string ToString(KeyboardKeys key)
        {
            switch (key)
            {
            case KeyboardKeys.Control: return "Ctrl";
            default: return key.ToString();
            }
        }

        /// <summary>
        /// Tries the parse the input text value to the <see cref="InputBinding"/>.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <param name="result">The result value (valid only if method returns true).</param>
        /// <returns>True if parsing succeed, otherwise false.</returns>
        public static bool TryParse(string value, out InputBinding result)
        {
            result = new InputBinding();
            string[] v = value.Split('+');
            switch (v.Length)
            {
            case 3:
                if (Parse(v[2], out result.Key) &&
                    Parse(v[1], out result.Modifier1) &&
                    Parse(v[0], out result.Modifier2))
                    return true;
                break;
            case 2:
                if (Parse(v[1], out result.Key) &&
                    Parse(v[0], out result.Modifier1))
                    return true;
                break;

            case 1:
                if (Parse(v[0], out result.Key))
                    return true;
                break;
            }
            return false;
        }

        private bool ProcessModifiers(Control control)
        {
            return ProcessModifiers(control.Root.GetKey);
        }

        private bool ProcessModifiers(Window window)
        {
            return ProcessModifiers(window.GetKey);
        }

        private bool ProcessModifiers(Func<KeyboardKeys, bool> getKeyFunc)
        {
            bool ctrlPressed = getKeyFunc(KeyboardKeys.Control);
            bool shiftPressed = getKeyFunc(KeyboardKeys.Shift);
            bool altPressed = getKeyFunc(KeyboardKeys.Alt);

            bool mod1 = false;
            if (Modifier1 == KeyboardKeys.None)
                mod1 = true;
            else if (Modifier1 == KeyboardKeys.Control)
            {
                mod1 = ctrlPressed;
                ctrlPressed = false;
            }
            else if (Modifier1 == KeyboardKeys.Shift)
            {
                mod1 = shiftPressed;
                shiftPressed = false;
            }
            else if (Modifier1 == KeyboardKeys.Alt)
            {
                mod1 = altPressed;
                altPressed = false;
            }

            bool mod2 = false;
            if (Modifier2 == KeyboardKeys.None)
                mod2 = true;
            else if (Modifier2 == KeyboardKeys.Control)
            {
                mod2 = ctrlPressed;
                ctrlPressed = false;
            }
            else if (Modifier2 == KeyboardKeys.Shift)
            {
                mod2 = shiftPressed;
                shiftPressed = false;
            }
            else if (Modifier2 == KeyboardKeys.Alt)
            {
                mod2 = altPressed;
                altPressed = false;
            }

            // Check if any unhandled modifiers are not pressed
            if (mod1 && mod2)
                return !ctrlPressed && !shiftPressed && !altPressed;
            return false;
        }

        /// <summary>
        /// Processes this input binding to check if state matches.
        /// </summary>
        /// <param name="control">The input providing control.</param>
        /// <returns>True if input has been processed, otherwise false.</returns>
        public bool Process(Control control)
        {
            var root = control?.Root;
            return root != null && root.GetKey(Key) && ProcessModifiers(control);
        }

        /// <summary>
        /// Processes this input binding to check if state matches.
        /// </summary>
        /// <param name="control">The input providing control.</param>
        /// <param name="key">The input key.</param>
        /// <returns>True if input has been processed, otherwise false.</returns>
        public bool Process(Control control, KeyboardKeys key)
        {
            if (key != Key)
                return false;

            return ProcessModifiers(control);
        }

        /// <summary>
        /// Processes this input binding to check if state matches.
        /// </summary>
        /// <param name="window">The input providing window.</param>
        /// <returns>True if input has been processed, otherwise false.</returns>
        public bool Process(Window window)
        {
            return window.GetKey(Key) && ProcessModifiers(window);
        }

        /// <inheritdoc />
        public override string ToString()
        {
            string result = string.Empty;
            if (Modifier2 != KeyboardKeys.None)
            {
                result = ToString(Modifier2);
            }
            if (Modifier1 != KeyboardKeys.None)
            {
                if (result.Length != 0)
                    result += '+';
                result += ToString(Modifier1);
            }
            if (Key != KeyboardKeys.None)
            {
                if (result.Length != 0)
                    result += '+';
                result += ToString(Key);
            }
            return result;
        }

        /// <inheritdoc />
        public bool Equals(InputBinding other)
        {
            return Key == other.Key && Modifier1 == other.Modifier1 && Modifier2 == other.Modifier2;
        }

        /// <inheritdoc />
        public override bool Equals(object obj)
        {
            return obj is InputBinding other && Equals(other);
        }

        /// <inheritdoc />
        public override int GetHashCode()
        {
            return HashCode.Combine((int)Key, (int)Modifier1, (int)Modifier2);
        }

        /// <summary>
        /// Compares two values.
        /// </summary>
        public static bool operator ==(InputBinding left, InputBinding right)
        {
            return left.Equals(right);
        }

        /// <summary>
        /// Compares two values.
        /// </summary>
        public static bool operator !=(InputBinding left, InputBinding right)
        {
            return !left.Equals(right);
        }
    }

    class InputBindingConverter : TypeConverter
    {
        /// <inheritdoc />
        public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType)
        {
            if (sourceType == typeof(string))
                return true;
            return base.CanConvertFrom(context, sourceType);
        }

        /// <inheritdoc />
        public override bool CanConvertTo(ITypeDescriptorContext context, Type destinationType)
        {
            if (destinationType == typeof(string))
                return false;
            return base.CanConvertTo(context, destinationType);
        }

        /// <inheritdoc />
        public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value)
        {
            if (value is string str)
            {
                InputBinding.TryParse(str, out var result);
                return result;
            }
            return base.ConvertFrom(context, culture, value);
        }

        /// <inheritdoc />
        public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType)
        {
            if (destinationType == typeof(string))
            {
                return ((InputBinding)value).ToString();
            }
            return base.ConvertTo(context, culture, value, destinationType);
        }
    }

    class InputBindingEditor : CustomEditor
    {
        private CustomElement<InputBindingBox> _element;

        private class InputBindingBox : TextBox
        {
            private InputBinding _binding;

            /// <inheritdoc />
            protected override void OnEditBegin()
            {
                base.OnEditBegin();

                // Reset
                _text = string.Empty;
                _binding = new InputBinding();
            }

            /// <inheritdoc />
            public override bool OnCharInput(char c)
            {
                // Skip text
                return true;
            }

            /// <inheritdoc />
            public override bool OnKeyDown(KeyboardKeys key)
            {
                // Skip already added keys
                if (_binding.Key == key || _binding.Modifier1 == key || _binding.Modifier2 == key)
                    return true;

                switch (key)
                {
                // Skip
                case KeyboardKeys.Spacebar: break;

                // Modifiers
                case KeyboardKeys.Control:
                case KeyboardKeys.Shift:
                    if (_binding.Modifier1 == KeyboardKeys.None)
                    {
                        _binding.Modifier1 = key;
                        _text = _binding.ToString();
                    }
                    else if (_binding.Modifier2 == KeyboardKeys.None)
                    {
                        _binding.Modifier2 = key;
                        _text = _binding.ToString();
                    }
                    break;

                // Keys
                default:
                    if (_binding.Key == KeyboardKeys.None)
                    {
                        _binding.Key = key;
                        _text = _binding.ToString();
                        Defocus();
                    }
                    break;
                }
                return true;
            }
        }

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            var grid = layout.CustomContainer<GridPanel>();
            var gridControl = grid.CustomControl;
            gridControl.ClipChildren = false;
            gridControl.Height = TextBox.DefaultHeight;
            gridControl.RowFill = new[]
            {
                1.0f,
            };
            gridControl.ColumnFill = new[]
            {
                0.9f,
                0.1f
            };

            _element = grid.Custom<InputBindingBox>();
            SetText();
            _element.CustomControl.WatermarkText = "Type a binding";
            _element.CustomControl.EditEnd += OnValueChanged;

            var button = grid.Button("X");
            button.Button.TooltipText = "Remove binding";
            button.Button.Clicked += OnXButtonClicked;
        }

        private void OnXButtonClicked()
        {
            SetValue(new InputBinding());
        }

        private void OnValueChanged()
        {
            if (InputBinding.TryParse(_element.CustomControl.Text, out var value))
                SetValue(value);
            else
                SetText();
        }

        private void SetText()
        {
            _element.CustomControl.Text = ((InputBinding)Values[0]).ToString();
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            SetText();
        }
    }

    /// <summary>
    /// The input actions processing helper that handles input bindings configuration layer.
    /// </summary>
    public class InputActionsContainer
    {
        /// <summary>
        /// The binding.
        /// </summary>
        public struct Binding
        {
            /// <summary>
            /// The binded options callback.
            /// </summary>
            public Func<InputOptions, InputBinding> Binder;

            /// <summary>
            /// The action callback.
            /// </summary>
            public Action Callback;

            /// <summary>
            /// Initializes a new instance of the <see cref="Binding"/> struct.
            /// </summary>
            /// <param name="binder">The input binding options getter (can read from editor options or use constant binding).</param>
            /// <param name="callback">The callback to invoke on user input.</param>
            public Binding(Func<InputOptions, InputBinding> binder, Action callback)
            {
                Binder = binder;
                Callback = callback;
            }
        }

        /// <summary>
        /// List of all available bindings.
        /// </summary>
        public List<Binding> Bindings;

        /// <summary>
        /// Initializes a new instance of the <see cref="InputActionsContainer"/> class.
        /// </summary>
        public InputActionsContainer()
        {
            Bindings = new List<Binding>();
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="InputActionsContainer"/> class.
        /// </summary>
        /// <param name="bindings">The input bindings collection.</param>
        public InputActionsContainer(params Binding[] bindings)
        {
            Bindings = new List<Binding>(bindings);
        }

        /// <summary>
        /// Adds the specified binding.
        /// </summary>
        /// <param name="binding">The input binding.</param>
        public void Add(Binding binding)
        {
            Bindings.Add(binding);
        }

        /// <summary>
        /// Adds the specified binding.
        /// </summary>
        /// <param name="binder">The input binding options getter (can read from editor options or use constant binding).</param>
        /// <param name="callback">The callback to invoke on user input.</param>
        public void Add(Func<InputOptions, InputBinding> binder, Action callback)
        {
            Bindings.Add(new Binding(binder, callback));
        }

        /// <summary>
        /// Adds the specified bindings.
        /// </summary>
        /// <param name="bindings">The input bindings collection.</param>
        public void Add(params Binding[] bindings)
        {
            Bindings.AddRange(bindings);
        }

        /// <summary>
        /// Processes the specified key input and tries to invoke first matching callback for the current user input state.
        /// </summary>
        /// <param name="editor">The editor instance.</param>
        /// <param name="control">The input providing control.</param>
        /// <param name="key">The input key.</param>
        /// <returns>True if event has been handled, otherwise false.</returns>
        public bool Process(Editor editor, Control control, KeyboardKeys key)
        {
            var root = control.Root;
            var options = editor.Options.Options.Input;

            for (int i = 0; i < Bindings.Count; i++)
            {
                var binding = Bindings[i].Binder(options);
                if (binding.Process(control, key))
                {
                    Bindings[i].Callback();
                    return true;
                }
            }

            return false;
        }

        /// <summary>
        /// Invokes a specific binding.
        /// </summary>
        /// <param name="editor">The editor instance.</param>
        /// <param name="binding">The binding to execute.</param>
        /// <returns>True if event has been handled, otherwise false.</returns>
        public bool Invoke(Editor editor, InputBinding binding)
        {
            if (binding == new InputBinding())
                return false;
            var options = editor.Options.Options.Input;
            for (int i = 0; i < Bindings.Count; i++)
            {
                if (Bindings[i].Binder(options) == binding)
                {
                    Bindings[i].Callback();
                    return true;
                }
            }
            return false;
        }
    }
}
