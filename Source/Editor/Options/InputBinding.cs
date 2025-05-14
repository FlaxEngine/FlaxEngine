// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Globalization;
using System.Linq;
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
        /// The <see cref="InputTrigger"/> list to bind.
        /// </summary>
        public List<InputTrigger> InputTriggers;
        private const int MAX_TRIGGERS = 4;

        /// <summary>
        /// Initializes an empty <see cref="InputBinding"/>
        /// </summary>
        public InputBinding()
        {
            InputTriggers = new List<InputTrigger>();
        }

        /// <summary>
        /// Initializes using <see cref="string"/>, <see cref="KeyboardKeys"/>, <see cref="MouseButton"/>, or <see cref="InputTrigger"/> struct.
        /// </summary>
        public InputBinding(params object[] inputs)
        {
            InputTriggers = new List<InputTrigger>();

            if (inputs.Length == 0)
            {
                InputTriggers.Add(new InputTrigger());
                return;
            }

            for (int i = 0; i < inputs.Length && i < MAX_TRIGGERS; i++)
            {
                object input = inputs[i];
                if (
                    input is string str
                    && InputTrigger.TryParseInput(str, out var parsedStr))
                {
                    InputTriggers.Add(parsedStr);
                    continue;
                }
                if (
                    input is InputTrigger
                    && InputTrigger.TryParseInput(input.ToString(), out var parsedTrigger))
                {
                    InputTriggers.Add(parsedTrigger);
                    continue;
                }
                if (
                    input is KeyboardKeys
                    && InputTrigger.TryParseInput(input.ToString(), out var parsedKey))
                {
                    InputTriggers.Add(parsedKey);
                    continue;
                }
                if (
                    input is MouseButton
                    && InputTrigger.TryParseInput(input.ToString(), out var parsedMouse))
                {
                    InputTriggers.Add(parsedMouse);
                    continue;
                }
                if (
                    input is MouseScroll
                    && InputTrigger.TryParseInput(input.ToString(), out var parsedMouseScroll))
                {
                    InputTriggers.Add(parsedMouseScroll);
                    continue;
                }
                throw new ArgumentException($"Unsupported input type: {input.GetType().Name}");
            }
        }

        /// <summary>
        /// Initializes using input <see cref="string"/> separated by +
        /// <para>Example: "Left+Control+R" = MouseButton.Left, KeyboardKeys.Control, KeyboardKeys.R</para>
        /// </summary>
        public InputBinding(string bindingString)
        {
            if (!TryParse(bindingString, out var result))
                throw new ArgumentException($"Invalid binding string: {bindingString}");

            this = result;
        }

        /// <summary>
        /// Tries the parse the input text value to the <see cref="InputBinding"/>.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <param name="result">The result value (valid only if method returns true).</param>
        /// <returns>True if parsing succeed, otherwise false.</returns>
        public static bool TryParse(string value, out InputBinding result)
        {
            var parts = value.Split('+', StringSplitOptions.RemoveEmptyEntries);
            var triggers = new List<InputTrigger>();

            foreach (var part in parts)
            {
                if (InputTrigger.TryParseInput(part.Trim(), out var trigger))
                {
                    triggers.Add(trigger);
                }
                else
                {
                    result = new InputBinding();
                    return false;
                }
            }
            result = new InputBinding()
            {
                InputTriggers = triggers
            };
            return true;
        }

        /// <summary>
        /// Processes this input binding to check if state matches.
        /// </summary>
        /// <param name="control">The input providing control.</param>
        /// <returns>True if input has been processed, otherwise false.</returns>
        public bool Process(Control control)
        {
            if (control?.Root == null)
                return false;

            return Process(control.Root.GetKey, control.Root.GetMouseButton, control.Root.GetMouseScrollDelta());
        }

        /// <summary>
        /// Processes this input binding to check if state matches.
        /// </summary>
        /// <param name="window">The input providing window.</param>
        /// <returns>True if input has been processed, otherwise false.</returns>
        public bool Process(Window window)
        {
            return Process(window.GetKey, window.GetMouseButton, window.MouseScrollDelta);
        }

        /// <summary>
        /// Processes this input binding to check if all input triggers are currently active.
        /// </summary>
        /// <param name="getKey">Function to check if a keyboard key is currently pressed.</param>
        /// <param name="getMouse">Function to check if a mouse button is currently pressed.</param>
        /// <param name="scrollDelta">The mouse scroll delta over the last frame</param>
        /// <returns>True if all input triggers are currently pressed; otherwise, false.</returns>
        private bool Process(Func<KeyboardKeys, bool> getKey, Func<MouseButton, bool> getMouse, float scrollDelta)
        {
            foreach (var trigger in InputTriggers)
            {
                if (!trigger.IsPressed(getKey, getMouse, scrollDelta))
                    return false;
            }
            return true;
        }

        /// <inheritdoc />
        public override string ToString()
        {
            return (InputTriggers != null && InputTriggers.Count > 0)
            ? string.Join("+", InputTriggers)
            : string.Empty;
        }


        /// <inheritdoc />
        public override bool Equals(object obj)
        {
            return obj is InputBinding other && Equals(other);
        }

        /// <inheritdoc />
        public bool Equals(InputBinding other)
        {
            if (InputTriggers is null || other.InputTriggers is null)
                return false;

            if (InputTriggers.Count != other.InputTriggers.Count)
                return false;

            return new HashSet<InputTrigger>(InputTriggers).SetEquals(other.InputTriggers);
        }

        /// <inheritdoc />
        public override int GetHashCode()
        {
            // Order-independent hash
            int hash = 0;
            foreach (var trigger in InputTriggers)
                hash ^= trigger.GetHashCode();
            return hash;
        }

        /// <inheritdoc />
        public static bool operator ==(InputBinding left, InputBinding right)
        {
            return Equals(left, right);
        }

        /// <inheritdoc />
        public static bool operator !=(InputBinding left, InputBinding right)
        {
            return !Equals(left, right);
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
                if (InputBinding.TryParse(str, out var result))
                {
                    return result;
                }
                else
                {
                    throw new NotSupportedException($"InputBindingConverter cannot convert from '{str}' to InputBinding.");
                }
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
            else
            {
                throw new NotSupportedException($"InputBindingConverter cannot convert '{value}' to InputBinding.");
            }
            //return base.ConvertTo(context, culture, value, destinationType);
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
            public override bool OnMouseDown(Float2 location, MouseButton button)
            {
                if (!IsFocused)
                {
                    return false;
                }
                var trigger = new InputTrigger(button.ToString());
                if (!_binding.InputTriggers.Contains(trigger))
                {
                    _binding.InputTriggers.Add(trigger);
                }

                _text = _binding.ToString();
                return true;
            }

            /// <inheritdoc />
            public override bool OnKeyDown(KeyboardKeys key)
            {
                if (!IsFocused)
                {
                    return false;
                }
                var trigger = new InputTrigger(key.ToString());
                if (!_binding.InputTriggers.Contains(trigger))
                {
                    _binding.InputTriggers.Add(trigger);
                }

                _text = _binding.ToString();
                return true;
            }

            public override bool OnMouseWheel(Float2 location, float delta)
            {
                if (!IsFocused)
                {
                    return false;
                }
                var trigger = new InputTrigger(MouseScrollHelper.GetScrollDirection(delta).ToString());
                if (!_binding.InputTriggers.Contains(trigger))
                {
                    _binding.InputTriggers.Add(trigger);
                }

                _text = _binding.ToString();
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
            gridControl.RowFill =
            [
                1.0f,
            ];
            gridControl.ColumnFill =
            [
                0.9f,
                0.1f
            ];

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
            {
                SetValue(value.ToString());
            }
            else
            {
                SetText();
            }
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
        /// <returns>True if event has been handled, otherwise false.</returns>
        public bool Process(Editor editor, Control control)
        {
            var root = control.Root;
            var options = editor.Options.Options.Input;

            for (int i = 0; i < Bindings.Count; i++)
            {
                var binding = Bindings[i].Binder(options);
                if (binding.Process(control))
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
