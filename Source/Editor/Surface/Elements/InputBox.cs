// Copyright (c) Wojciech Figat. All rights reserved.

#if USE_LARGE_WORLDS
using Real = System.Double;
using RealValueBox = FlaxEditor.GUI.Input.DoubleValueBox;
#else
using Real = System.Single;
using RealValueBox = FlaxEditor.GUI.Input.FloatValueBox;
#endif

using System;
using System.Collections.Generic;
using System.Linq;
using FlaxEditor.CustomEditors.Editors;
using FlaxEditor.GUI;
using FlaxEditor.GUI.Input;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Utilities;
using Newtonsoft.Json;
using JsonSerializer = FlaxEngine.Json.JsonSerializer;

namespace FlaxEditor.Surface.Elements
{
    /// <summary>
    /// The handler for the input box default value editing. Used to display the default to the UI.
    /// </summary>
    [HideInEditor]
    public interface IDefaultValueEditor
    {
        /// <summary>
        /// Checks if the handles supports the given type.
        /// </summary>
        /// <param name="box">The input box that uses this editor.</param>
        /// <param name="type">The type to check.</param>
        /// <returns>True if can create UI for this type editing, otherwise false.</returns>
        bool CanUse(InputBox box, ref ScriptType type);

        /// <summary>
        /// Creates the UI for the value editing.
        /// </summary>
        /// <param name="box">The input box that uses this editor.</param>
        /// <param name="bounds">The control bounds (control can be smaller but cannot be bigger).</param>
        /// <returns>The root control of the created UI</returns>
        Control Create(InputBox box, ref Rectangle bounds);

        /// <summary>
        /// Returns true if given control is valid root editor for the value.
        /// </summary>
        /// <param name="box">The input box that uses this editor.</param>
        /// <param name="control">The root control of the editor UI.</param>
        /// <returns><c>true</c> if the specified control is valid; otherwise, <c>false</c>.</returns>
        bool IsValid(InputBox box, Control control);

        /// <summary>
        /// Updates the default value editor UI value.
        /// </summary>
        /// <param name="box">The input box that uses this editor.</param>
        /// <param name="control">The root control of the editor UI.</param>
        void UpdateDefaultValue(InputBox box, Control control);

        /// <summary>
        /// Updates the UI after box attributes change.
        /// </summary>
        /// <param name="box">The input box that uses this editor.</param>
        /// <param name="attributes">The custom attributes collection.</param>
        /// <param name="control">The root control of the editor UI.</param>
        void UpdateAttributes(InputBox box, object[] attributes, Control control);
    }

    class BooleanDefaultValueEditor : IDefaultValueEditor
    {
        public bool CanUse(InputBox box, ref ScriptType type)
        {
            return type.Type == typeof(bool);
        }

        public Control Create(InputBox box, ref Rectangle bounds)
        {
            var value = BoolValue.Get(box.ParentNode, box.Archetype, box.Value);
            var control = new CheckBox(bounds.X, bounds.Y, value, bounds.Height)
            {
                Parent = box.Parent,
                Tag = box,
            };
            control.StateChanged += OnCheckboxStateChanged;
            return control;
        }

        public bool IsValid(InputBox box, Control control)
        {
            return control is CheckBox;
        }

        public void UpdateDefaultValue(InputBox box, Control control)
        {
            if (control is CheckBox checkBox)
                checkBox.Checked = BoolValue.Get(box.ParentNode, box.Archetype, box.Value);
        }

        public void UpdateAttributes(InputBox box, object[] attributes, Control control)
        {
        }

        private void OnCheckboxStateChanged(CheckBox control)
        {
            var box = (InputBox)control.Tag;
            box.Value = control.Checked;
        }
    }

    class IntegerDefaultValueEditor : IDefaultValueEditor
    {
        public bool CanUse(InputBox box, ref ScriptType type)
        {
            return type.Type == typeof(int);
        }

        public Control Create(InputBox box, ref Rectangle bounds)
        {
            var value = IntegerValue.Get(box.ParentNode, box.Archetype, box.Value);
            var width = 40;
            var control = new IntValueBox(value, bounds.X, bounds.Y, width + 12, int.MinValue, int.MaxValue, 0.01f)
            {
                Height = bounds.Height,
                Parent = box.Parent,
                Tag = box,
            };
            control.BoxValueChanged += OnValueChanged;
            return control;
        }

        public bool IsValid(InputBox box, Control control)
        {
            return control is IntValueBox;
        }

        public void UpdateDefaultValue(InputBox box, Control control)
        {
            if (control is IntValueBox intValue)
                intValue.Value = IntegerValue.Get(box.ParentNode, box.Archetype, box.Value);
        }

        public void UpdateAttributes(InputBox box, object[] attributes, Control control)
        {
        }

        private void OnValueChanged(ValueBox<int> control)
        {
            var box = (InputBox)control.Tag;
            if (box.HasCustomValueAccess)
                box.Value = control.Value;
            else
                IntegerValue.Set(box.ParentNode, box.Archetype, control.Value);
        }
    }

    class UnsignedIntegerDefaultValueEditor : IDefaultValueEditor
    {
        public bool CanUse(InputBox box, ref ScriptType type)
        {
            return type.Type == typeof(uint);
        }

        public Control Create(InputBox box, ref Rectangle bounds)
        {
            var value = UnsignedIntegerValue.Get(box.ParentNode, box.Archetype, box.Value);
            var width = 40;
            var control = new UIntValueBox(value, bounds.X, bounds.Y, width + 12, uint.MinValue, uint.MaxValue, 0.01f)
            {
                Height = bounds.Height,
                Parent = box.Parent,
                Tag = box,
            };
            control.BoxValueChanged += OnValueChanged;
            return control;
        }

        public bool IsValid(InputBox box, Control control)
        {
            return control is UIntValueBox;
        }

        public void UpdateDefaultValue(InputBox box, Control control)
        {
            if (control is UIntValueBox intValue)
                intValue.Value = UnsignedIntegerValue.Get(box.ParentNode, box.Archetype, box.Value);
        }

        public void UpdateAttributes(InputBox box, object[] attributes, Control control)
        {
        }

        private void OnValueChanged(ValueBox<uint> control)
        {
            var box = (InputBox)control.Tag;
            if (box.HasCustomValueAccess)
                box.Value = control.Value;
            else
                UnsignedIntegerValue.Set(box.ParentNode, box.Archetype, control.Value);
        }
    }

    class FloatingDefaultValueEditor : IDefaultValueEditor
    {
        public bool CanUse(InputBox box, ref ScriptType type)
        {
            return type.Type == typeof(float) || type.Type == typeof(double);
        }

        public Control Create(InputBox box, ref Rectangle bounds)
        {
            var value = FloatValue.Get(box.ParentNode, box.Archetype, box.Value);
            var width = 40;
            var control = new FloatValueBox(value, bounds.X, bounds.Y, width + 12, float.MinValue, float.MaxValue, 0.01f)
            {
                Height = bounds.Height,
                Parent = box.Parent,
                Tag = box,
            };
            control.BoxValueChanged += OnValueChanged;
            return control;
        }

        public bool IsValid(InputBox box, Control control)
        {
            return control is FloatValueBox;
        }

        public void UpdateDefaultValue(InputBox box, Control control)
        {
            if (control is FloatValueBox floatValue)
                floatValue.Value = FloatValue.Get(box.ParentNode, box.Archetype, box.Value);
        }

        public void UpdateAttributes(InputBox box, object[] attributes, Control control)
        {
        }

        private void OnValueChanged(ValueBox<float> control)
        {
            var box = (InputBox)control.Tag;
            if (box.HasCustomValueAccess)
                box.Value = control.Value;
            else
                FloatValue.Set(box.ParentNode, box.Archetype, control.Value);
        }
    }

    class StringDefaultValueEditor : IDefaultValueEditor
    {
        public bool CanUse(InputBox box, ref ScriptType type)
        {
            return type.Type == typeof(string) && (box.Attributes == null || box.Attributes.All(x => x.GetType() != typeof(TypeReferenceAttribute)));
        }

        public Control Create(InputBox box, ref Rectangle bounds)
        {
            var value = box.Value as string;
            var control = new TextBox(false, bounds.X, bounds.Y, 50)
            {
                Text = value,
                Height = bounds.Height,
                Parent = box.Parent,
                Tag = box,
            };
            control.TextBoxEditEnd += OnTextBoxTextChanged;
            return control;
        }

        public bool IsValid(InputBox box, Control control)
        {
            return control is TextBox;
        }

        public void UpdateDefaultValue(InputBox box, Control control)
        {
            if (control is TextBox textBox)
            {
                textBox.Text = box.Value as string;
            }
        }

        public void UpdateAttributes(InputBox box, object[] attributes, Control control)
        {
        }

        private void OnTextBoxTextChanged(TextBoxBase control)
        {
            var box = (InputBox)control.Tag;
            box.Value = control.Text;
        }
    }

    class Vector2DefaultValueEditor : IDefaultValueEditor
    {
        public bool CanUse(InputBox box, ref ScriptType type)
        {
            return type.Type == typeof(Vector2);
        }

        public Control Create(InputBox box, ref Rectangle bounds)
        {
            var value = GetValue(box);
            var width = 30;
            var control = new ContainerControl(bounds.X, bounds.Y, (width + 2) * 2 - 2, bounds.Height)
            {
                ClipChildren = false,
                AutoFocus = false,
                Parent = box.Parent,
                Tag = box,
            };
            var floatX = new RealValueBox(value.X, 0, 0, width, float.MinValue, float.MaxValue, 0.0f)
            {
                Height = bounds.Height,
                Parent = control,
            };
            floatX.BoxValueChanged += OnValueChanged;
            var floatY = new RealValueBox(value.Y, width + 2, 0, width, float.MinValue, float.MaxValue, 0.0f)
            {
                Height = bounds.Height,
                Parent = control,
            };
            floatY.BoxValueChanged += OnValueChanged;
            return control;
        }

        public bool IsValid(InputBox box, Control control)
        {
            return control is ContainerControl vec2
                   && vec2.ChildrenCount == 2
                   && vec2.Children[0] is RealValueBox
                   && vec2.Children[1] is RealValueBox;
        }

        public void UpdateDefaultValue(InputBox box, Control control)
        {
            if (control is ContainerControl vec2
                && vec2.Tag as Type == typeof(Vector2)
                && vec2.ChildrenCount == 2
                && vec2.Children[0] is RealValueBox x
                && vec2.Children[1] is RealValueBox y)
            {
                var value = GetValue(box);
                x.Value = value.X;
                y.Value = value.Y;
            }
        }

        public void UpdateAttributes(InputBox box, object[] attributes, Control control)
        {
        }

        private Vector2 GetValue(InputBox box)
        {
            return new Vector2(SurfaceUtils.GetDouble4(box.Value));
        }

        private void OnValueChanged(ValueBox<Real> valueBox)
        {
            var control = valueBox.Parent;
            var box = (InputBox)control.Tag;
            var x = ((RealValueBox)control.Children[0]).Value;
            var y = ((RealValueBox)control.Children[1]).Value;
            box.Value = new Vector2(x, y);
        }
    }

    class Vector3DefaultValueEditor : IDefaultValueEditor
    {
        public bool CanUse(InputBox box, ref ScriptType type)
        {
            return type.Type == typeof(Vector3);
        }

        public Control Create(InputBox box, ref Rectangle bounds)
        {
            var value = GetValue(box);
            var width = 30;
            var control = new ContainerControl(bounds.X, bounds.Y, (width + 2) * 3 - 2, bounds.Height)
            {
                ClipChildren = false,
                AutoFocus = false,
                Parent = box.Parent,
                Tag = box,
            };
            var floatX = new RealValueBox(value.X, 0, 0, width, float.MinValue, float.MaxValue, 0.0f)
            {
                Height = bounds.Height,
                Parent = control,
            };
            floatX.BoxValueChanged += OnValueChanged;
            var floatY = new RealValueBox(value.Y, width + 2, 0, width, float.MinValue, float.MaxValue, 0.0f)
            {
                Height = bounds.Height,
                Parent = control,
            };
            floatY.BoxValueChanged += OnValueChanged;
            var floatZ = new RealValueBox(value.Z, width * 2 + 4, 0, width, float.MinValue, float.MaxValue, 0.0f)
            {
                Height = bounds.Height,
                Parent = control,
            };
            floatZ.BoxValueChanged += OnValueChanged;
            return control;
        }

        public bool IsValid(InputBox box, Control control)
        {
            return control is ContainerControl vec3
                   && vec3.ChildrenCount == 3
                   && vec3.Children[0] is RealValueBox
                   && vec3.Children[1] is RealValueBox
                   && vec3.Children[2] is RealValueBox;
        }

        public void UpdateDefaultValue(InputBox box, Control control)
        {
            if (control is ContainerControl vec3
                && vec3.ChildrenCount == 3
                && vec3.Children[0] is RealValueBox x
                && vec3.Children[1] is RealValueBox y
                && vec3.Children[2] is RealValueBox z)
            {
                var value = GetValue(box);
                x.Value = value.X;
                y.Value = value.Y;
                z.Value = value.Z;
            }
        }

        public void UpdateAttributes(InputBox box, object[] attributes, Control control)
        {
        }

        private Vector3 GetValue(InputBox box)
        {
            return new Vector3(SurfaceUtils.GetDouble4(box.Value));
        }

        private void OnValueChanged(ValueBox<Real> valueBox)
        {
            var control = valueBox.Parent;
            var box = (InputBox)control.Tag;
            var x = ((RealValueBox)control.Children[0]).Value;
            var y = ((RealValueBox)control.Children[1]).Value;
            var z = ((RealValueBox)control.Children[2]).Value;
            box.Value = new Vector3(x, y, z);
        }
    }

    class Vector4DefaultValueEditor : IDefaultValueEditor
    {
        public bool CanUse(InputBox box, ref ScriptType type)
        {
            return type.Type == typeof(Vector4);
        }

        public Control Create(InputBox box, ref Rectangle bounds)
        {
            var value = GetValue(box);
            var width = 20;
            var control = new ContainerControl(bounds.X, bounds.Y, (width + 2) * 4 - 2, bounds.Height)
            {
                ClipChildren = false,
                AutoFocus = false,
                Parent = box.Parent,
                Tag = box,
            };
            var floatX = new RealValueBox(value.X, 0, 0, width, float.MinValue, float.MaxValue, 0.0f)
            {
                Height = bounds.Height,
                Parent = control,
            };
            floatX.BoxValueChanged += OnValueChanged;
            var floatY = new RealValueBox(value.Y, width + 2, 0, width, float.MinValue, float.MaxValue, 0.0f)
            {
                Height = bounds.Height,
                Parent = control,
            };
            floatY.BoxValueChanged += OnValueChanged;
            var floatZ = new RealValueBox(value.Z, width * 2 + 4, 0, width, float.MinValue, float.MaxValue, 0.0f)
            {
                Height = bounds.Height,
                Parent = control,
            };
            floatZ.BoxValueChanged += OnValueChanged;
            var floatW = new RealValueBox(value.W, width * 3 + 6, 0, width, float.MinValue, float.MaxValue, 0.0f)
            {
                Height = bounds.Height,
                Parent = control,
            };
            floatW.BoxValueChanged += OnValueChanged;
            return control;
        }

        public bool IsValid(InputBox box, Control control)
        {
            return control is ContainerControl vec4
                   && vec4.ChildrenCount == 4
                   && vec4.Children[0] is RealValueBox
                   && vec4.Children[1] is RealValueBox
                   && vec4.Children[2] is RealValueBox
                   && vec4.Children[3] is RealValueBox;
        }

        public void UpdateDefaultValue(InputBox box, Control control)
        {
            if (control is ContainerControl vec4
                && vec4.ChildrenCount == 4
                && vec4.Children[0] is RealValueBox x
                && vec4.Children[1] is RealValueBox y
                && vec4.Children[2] is RealValueBox z
                && vec4.Children[3] is RealValueBox w)
            {
                var value = GetValue(box);
                x.Value = value.X;
                y.Value = value.Y;
                z.Value = value.Z;
                w.Value = value.W;
            }
        }

        public void UpdateAttributes(InputBox box, object[] attributes, Control control)
        {
        }

        private Vector4 GetValue(InputBox box)
        {
            return (Vector4)SurfaceUtils.GetDouble4(box.Value);
        }

        private void OnValueChanged(ValueBox<Real> valueBox)
        {
            var control = valueBox.Parent;
            var box = (InputBox)control.Tag;
            var x = ((RealValueBox)control.Children[0]).Value;
            var y = ((RealValueBox)control.Children[1]).Value;
            var z = ((RealValueBox)control.Children[2]).Value;
            var w = ((RealValueBox)control.Children[3]).Value;
            box.Value = new Vector4(x, y, z, w);
        }
    }

    class Float2DefaultValueEditor : IDefaultValueEditor
    {
        public bool CanUse(InputBox box, ref ScriptType type)
        {
            return type.Type == typeof(Float2);
        }

        public Control Create(InputBox box, ref Rectangle bounds)
        {
            var value = GetValue(box);
            var width = 30;
            var control = new ContainerControl(bounds.X, bounds.Y, (width + 2) * 2 - 2, bounds.Height)
            {
                ClipChildren = false,
                AutoFocus = false,
                Parent = box.Parent,
                Tag = box,
            };
            var floatX = new FloatValueBox(value.X, 0, 0, width, float.MinValue, float.MaxValue, 0.0f)
            {
                Height = bounds.Height,
                Parent = control,
            };
            floatX.BoxValueChanged += OnValueChanged;
            var floatY = new FloatValueBox(value.Y, width + 2, 0, width, float.MinValue, float.MaxValue, 0.0f)
            {
                Height = bounds.Height,
                Parent = control,
            };
            floatY.BoxValueChanged += OnValueChanged;
            return control;
        }

        public bool IsValid(InputBox box, Control control)
        {
            return control is ContainerControl vec2
                   && vec2.ChildrenCount == 2
                   && vec2.Children[0] is FloatValueBox
                   && vec2.Children[1] is FloatValueBox;
        }

        public void UpdateDefaultValue(InputBox box, Control control)
        {
            if (control is ContainerControl vec2
                && vec2.Tag as Type == typeof(Float2)
                && vec2.ChildrenCount == 2
                && vec2.Children[0] is FloatValueBox x
                && vec2.Children[1] is FloatValueBox y)
            {
                var value = GetValue(box);
                x.Value = value.X;
                y.Value = value.Y;
            }
        }

        public void UpdateAttributes(InputBox box, object[] attributes, Control control)
        {
        }

        private Float2 GetValue(InputBox box)
        {
            return new Float2(SurfaceUtils.GetDouble4(box.Value));
        }

        private void OnValueChanged(ValueBox<float> valueBox)
        {
            var control = valueBox.Parent;
            var box = (InputBox)control.Tag;
            var x = ((FloatValueBox)control.Children[0]).Value;
            var y = ((FloatValueBox)control.Children[1]).Value;
            box.Value = new Float2(x, y);
        }
    }

    class Float3DefaultValueEditor : IDefaultValueEditor
    {
        public bool CanUse(InputBox box, ref ScriptType type)
        {
            return type.Type == typeof(Float3);
        }

        public Control Create(InputBox box, ref Rectangle bounds)
        {
            var value = GetValue(box);
            var width = 30;
            var control = new ContainerControl(bounds.X, bounds.Y, (width + 2) * 3 - 2, bounds.Height)
            {
                ClipChildren = false,
                AutoFocus = false,
                Parent = box.Parent,
                Tag = box,
            };
            var floatX = new FloatValueBox(value.X, 0, 0, width, float.MinValue, float.MaxValue, 0.0f)
            {
                Height = bounds.Height,
                Parent = control,
            };
            floatX.BoxValueChanged += OnValueChanged;
            var floatY = new FloatValueBox(value.Y, width + 2, 0, width, float.MinValue, float.MaxValue, 0.0f)
            {
                Height = bounds.Height,
                Parent = control,
            };
            floatY.BoxValueChanged += OnValueChanged;
            var floatZ = new FloatValueBox(value.Z, width * 2 + 4, 0, width, float.MinValue, float.MaxValue, 0.0f)
            {
                Height = bounds.Height,
                Parent = control,
            };
            floatZ.BoxValueChanged += OnValueChanged;
            return control;
        }

        public bool IsValid(InputBox box, Control control)
        {
            return control is ContainerControl vec3
                   && vec3.ChildrenCount == 3
                   && vec3.Children[0] is FloatValueBox
                   && vec3.Children[1] is FloatValueBox
                   && vec3.Children[2] is FloatValueBox;
        }

        public void UpdateDefaultValue(InputBox box, Control control)
        {
            if (control is ContainerControl vec3
                && vec3.ChildrenCount == 3
                && vec3.Children[0] is FloatValueBox x
                && vec3.Children[1] is FloatValueBox y
                && vec3.Children[2] is FloatValueBox z)
            {
                var value = GetValue(box);
                x.Value = value.X;
                y.Value = value.Y;
                z.Value = value.Z;
            }
        }

        public void UpdateAttributes(InputBox box, object[] attributes, Control control)
        {
        }

        private Float3 GetValue(InputBox box)
        {
            return new Float3(SurfaceUtils.GetDouble4(box.Value));
        }

        private void OnValueChanged(ValueBox<float> valueBox)
        {
            var control = valueBox.Parent;
            var box = (InputBox)control.Tag;
            var x = ((FloatValueBox)control.Children[0]).Value;
            var y = ((FloatValueBox)control.Children[1]).Value;
            var z = ((FloatValueBox)control.Children[2]).Value;
            box.Value = new Float3(x, y, z);
        }
    }

    class Float4DefaultValueEditor : IDefaultValueEditor
    {
        public bool CanUse(InputBox box, ref ScriptType type)
        {
            return type.Type == typeof(Float4);
        }

        public Control Create(InputBox box, ref Rectangle bounds)
        {
            var value = GetValue(box);
            var width = 20;
            var control = new ContainerControl(bounds.X, bounds.Y, (width + 2) * 4 - 2, bounds.Height)
            {
                ClipChildren = false,
                AutoFocus = false,
                Parent = box.Parent,
                Tag = box,
            };
            var floatX = new FloatValueBox(value.X, 0, 0, width, float.MinValue, float.MaxValue, 0.0f)
            {
                Height = bounds.Height,
                Parent = control,
            };
            floatX.BoxValueChanged += OnValueChanged;
            var floatY = new FloatValueBox(value.Y, width + 2, 0, width, float.MinValue, float.MaxValue, 0.0f)
            {
                Height = bounds.Height,
                Parent = control,
            };
            floatY.BoxValueChanged += OnValueChanged;
            var floatZ = new FloatValueBox(value.Z, width * 2 + 4, 0, width, float.MinValue, float.MaxValue, 0.0f)
            {
                Height = bounds.Height,
                Parent = control,
            };
            floatZ.BoxValueChanged += OnValueChanged;
            var floatW = new FloatValueBox(value.W, width * 3 + 6, 0, width, float.MinValue, float.MaxValue, 0.0f)
            {
                Height = bounds.Height,
                Parent = control,
            };
            floatW.BoxValueChanged += OnValueChanged;
            return control;
        }

        public bool IsValid(InputBox box, Control control)
        {
            return control is ContainerControl vec4
                   && vec4.ChildrenCount == 4
                   && vec4.Children[0] is FloatValueBox
                   && vec4.Children[1] is FloatValueBox
                   && vec4.Children[2] is FloatValueBox
                   && vec4.Children[3] is FloatValueBox;
        }

        public void UpdateDefaultValue(InputBox box, Control control)
        {
            if (control is ContainerControl vec4
                && vec4.ChildrenCount == 4
                && vec4.Children[0] is FloatValueBox x
                && vec4.Children[1] is FloatValueBox y
                && vec4.Children[2] is FloatValueBox z
                && vec4.Children[3] is FloatValueBox w)
            {
                var value = GetValue(box);
                x.Value = value.X;
                y.Value = value.Y;
                z.Value = value.Z;
                w.Value = value.W;
            }
        }

        public void UpdateAttributes(InputBox box, object[] attributes, Control control)
        {
        }

        private Float4 GetValue(InputBox box)
        {
            return (Float4)SurfaceUtils.GetDouble4(box.Value);
        }

        private void OnValueChanged(ValueBox<float> valueBox)
        {
            var control = valueBox.Parent;
            var box = (InputBox)control.Tag;
            var x = ((FloatValueBox)control.Children[0]).Value;
            var y = ((FloatValueBox)control.Children[1]).Value;
            var z = ((FloatValueBox)control.Children[2]).Value;
            var w = ((FloatValueBox)control.Children[3]).Value;
            box.Value = new Float4(x, y, z, w);
        }
    }

    class Double2DefaultValueEditor : IDefaultValueEditor
    {
        public bool CanUse(InputBox box, ref ScriptType type)
        {
            return type.Type == typeof(Double2);
        }

        public Control Create(InputBox box, ref Rectangle bounds)
        {
            var value = GetValue(box);
            var width = 30;
            var control = new ContainerControl(bounds.X, bounds.Y, (width + 2) * 2 - 2, bounds.Height)
            {
                ClipChildren = false,
                AutoFocus = false,
                Parent = box.Parent,
                Tag = box,
            };
            var floatX = new DoubleValueBox(value.X, 0, 0, width, float.MinValue, float.MaxValue, 0.0f)
            {
                Height = bounds.Height,
                Parent = control,
            };
            floatX.BoxValueChanged += OnValueChanged;
            var floatY = new DoubleValueBox(value.Y, width + 2, 0, width, float.MinValue, float.MaxValue, 0.0f)
            {
                Height = bounds.Height,
                Parent = control,
            };
            floatY.BoxValueChanged += OnValueChanged;
            return control;
        }

        public bool IsValid(InputBox box, Control control)
        {
            return control is ContainerControl vec2
                   && vec2.ChildrenCount == 2
                   && vec2.Children[0] is DoubleValueBox
                   && vec2.Children[1] is DoubleValueBox;
        }

        public void UpdateDefaultValue(InputBox box, Control control)
        {
            if (control is ContainerControl vec2
                && vec2.Tag as Type == typeof(Double2)
                && vec2.ChildrenCount == 2
                && vec2.Children[0] is DoubleValueBox x
                && vec2.Children[1] is DoubleValueBox y)
            {
                var value = GetValue(box);
                x.Value = value.X;
                y.Value = value.Y;
            }
        }

        public void UpdateAttributes(InputBox box, object[] attributes, Control control)
        {
        }

        private Double2 GetValue(InputBox box)
        {
            return new Double2(SurfaceUtils.GetDouble4(box.Value));
        }

        private void OnValueChanged(ValueBox<double> valueBox)
        {
            var control = valueBox.Parent;
            var box = (InputBox)control.Tag;
            var x = ((DoubleValueBox)control.Children[0]).Value;
            var y = ((DoubleValueBox)control.Children[1]).Value;
            box.Value = new Double2(x, y);
        }
    }

    class Double3DefaultValueEditor : IDefaultValueEditor
    {
        public bool CanUse(InputBox box, ref ScriptType type)
        {
            return type.Type == typeof(Double3);
        }

        public Control Create(InputBox box, ref Rectangle bounds)
        {
            var value = GetValue(box);
            var width = 30;
            var control = new ContainerControl(bounds.X, bounds.Y, (width + 2) * 3 - 2, bounds.Height)
            {
                ClipChildren = false,
                AutoFocus = false,
                Parent = box.Parent,
                Tag = box,
            };
            var floatX = new DoubleValueBox(value.X, 0, 0, width, float.MinValue, float.MaxValue, 0.0f)
            {
                Height = bounds.Height,
                Parent = control,
            };
            floatX.BoxValueChanged += OnValueChanged;
            var floatY = new DoubleValueBox(value.Y, width + 2, 0, width, float.MinValue, float.MaxValue, 0.0f)
            {
                Height = bounds.Height,
                Parent = control,
            };
            floatY.BoxValueChanged += OnValueChanged;
            var floatZ = new DoubleValueBox(value.Z, width * 2 + 4, 0, width, float.MinValue, float.MaxValue, 0.0f)
            {
                Height = bounds.Height,
                Parent = control,
            };
            floatZ.BoxValueChanged += OnValueChanged;
            return control;
        }

        public bool IsValid(InputBox box, Control control)
        {
            return control is ContainerControl vec3
                   && vec3.ChildrenCount == 3
                   && vec3.Children[0] is DoubleValueBox
                   && vec3.Children[1] is DoubleValueBox
                   && vec3.Children[2] is DoubleValueBox;
        }

        public void UpdateDefaultValue(InputBox box, Control control)
        {
            if (control is ContainerControl vec3
                && vec3.ChildrenCount == 3
                && vec3.Children[0] is DoubleValueBox x
                && vec3.Children[1] is DoubleValueBox y
                && vec3.Children[2] is DoubleValueBox z)
            {
                var value = GetValue(box);
                x.Value = value.X;
                y.Value = value.Y;
                z.Value = value.Z;
            }
        }

        public void UpdateAttributes(InputBox box, object[] attributes, Control control)
        {
        }

        private Double3 GetValue(InputBox box)
        {
            return new Double3(SurfaceUtils.GetDouble4(box.Value));
        }

        private void OnValueChanged(ValueBox<double> valueBox)
        {
            var control = valueBox.Parent;
            var box = (InputBox)control.Tag;
            var x = ((FloatValueBox)control.Children[0]).Value;
            var y = ((FloatValueBox)control.Children[1]).Value;
            var z = ((FloatValueBox)control.Children[2]).Value;
            box.Value = new Double3(x, y, z);
        }
    }

    class Double4DefaultValueEditor : IDefaultValueEditor
    {
        public bool CanUse(InputBox box, ref ScriptType type)
        {
            return type.Type == typeof(Double4);
        }

        public Control Create(InputBox box, ref Rectangle bounds)
        {
            var value = GetValue(box);
            var width = 20;
            var control = new ContainerControl(bounds.X, bounds.Y, (width + 2) * 4 - 2, bounds.Height)
            {
                ClipChildren = false,
                AutoFocus = false,
                Parent = box.Parent,
                Tag = box,
            };
            var floatX = new DoubleValueBox(value.X, 0, 0, width, float.MinValue, float.MaxValue, 0.0f)
            {
                Height = bounds.Height,
                Parent = control,
            };
            floatX.BoxValueChanged += OnValueChanged;
            var floatY = new DoubleValueBox(value.Y, width + 2, 0, width, float.MinValue, float.MaxValue, 0.0f)
            {
                Height = bounds.Height,
                Parent = control,
            };
            floatY.BoxValueChanged += OnValueChanged;
            var floatZ = new DoubleValueBox(value.Z, width * 2 + 4, 0, width, float.MinValue, float.MaxValue, 0.0f)
            {
                Height = bounds.Height,
                Parent = control,
            };
            floatZ.BoxValueChanged += OnValueChanged;
            var floatW = new DoubleValueBox(value.W, width * 3 + 6, 0, width, float.MinValue, float.MaxValue, 0.0f)
            {
                Height = bounds.Height,
                Parent = control,
            };
            floatW.BoxValueChanged += OnValueChanged;
            return control;
        }

        public bool IsValid(InputBox box, Control control)
        {
            return control is ContainerControl vec4
                   && vec4.ChildrenCount == 4
                   && vec4.Children[0] is DoubleValueBox
                   && vec4.Children[1] is DoubleValueBox
                   && vec4.Children[2] is DoubleValueBox
                   && vec4.Children[3] is DoubleValueBox;
        }

        public void UpdateDefaultValue(InputBox box, Control control)
        {
            if (control is ContainerControl vec4
                && vec4.ChildrenCount == 4
                && vec4.Children[0] is DoubleValueBox x
                && vec4.Children[1] is DoubleValueBox y
                && vec4.Children[2] is DoubleValueBox z
                && vec4.Children[3] is DoubleValueBox w)
            {
                var value = GetValue(box);
                x.Value = value.X;
                y.Value = value.Y;
                z.Value = value.Z;
                w.Value = value.W;
            }
        }

        public void UpdateAttributes(InputBox box, object[] attributes, Control control)
        {
        }

        private Double4 GetValue(InputBox box)
        {
            return SurfaceUtils.GetDouble4(box.Value);
        }

        private void OnValueChanged(ValueBox<double> valueBox)
        {
            var control = valueBox.Parent;
            var box = (InputBox)control.Tag;
            var x = ((DoubleValueBox)control.Children[0]).Value;
            var y = ((DoubleValueBox)control.Children[1]).Value;
            var z = ((DoubleValueBox)control.Children[2]).Value;
            var w = ((DoubleValueBox)control.Children[3]).Value;
            box.Value = new Double4(x, y, z, w);
        }
    }

    class QuaternionDefaultValueEditor : IDefaultValueEditor
    {
        public bool CanUse(InputBox box, ref ScriptType type)
        {
            return type.Type == typeof(Quaternion);
        }

        public Control Create(InputBox box, ref Rectangle bounds)
        {
            var value = GetValue(box).EulerAngles;
            var width = 20;
            var control = new ContainerControl(bounds.X, bounds.Y, (width + 2) * 3 - 2, bounds.Height)
            {
                ClipChildren = false,
                AutoFocus = false,
                Parent = box.Parent,
                Tag = box,
            };
            var floatX = new FloatValueBox(value.X, 0, 0, width, float.MinValue, float.MaxValue, 0.0f)
            {
                Height = bounds.Height,
                Parent = control,
            };
            floatX.BoxValueChanged += OnValueChanged;
            var floatY = new FloatValueBox(value.Y, width + 2, 0, width, float.MinValue, float.MaxValue, 0.0f)
            {
                Height = bounds.Height,
                Parent = control,
            };
            floatY.BoxValueChanged += OnValueChanged;
            var floatZ = new FloatValueBox(value.Z, width * 2 + 4, 0, width, float.MinValue, float.MaxValue, 0.0f)
            {
                Height = bounds.Height,
                Parent = control,
            };
            floatZ.BoxValueChanged += OnValueChanged;
            return control;
        }

        public bool IsValid(InputBox box, Control control)
        {
            return control is ContainerControl vec3
                   && vec3.ChildrenCount == 3
                   && vec3.Children[0] is FloatValueBox
                   && vec3.Children[1] is FloatValueBox
                   && vec3.Children[2] is FloatValueBox;
        }

        public void UpdateDefaultValue(InputBox box, Control control)
        {
            if (control is ContainerControl quat
                && quat.ChildrenCount == 3
                && quat.Children[0] is FloatValueBox x
                && quat.Children[1] is FloatValueBox y
                && quat.Children[2] is FloatValueBox z)
            {
                var value = GetValue(box).EulerAngles;
                x.Value = value.X;
                y.Value = value.Y;
                z.Value = value.Z;
            }
        }

        public void UpdateAttributes(InputBox box, object[] attributes, Control control)
        {
        }

        private Quaternion GetValue(InputBox box)
        {
            var value = Quaternion.Identity;
            var v = box.Value;
            if (v is Quaternion quat)
                value = quat;
            else if (v is Transform transform)
                value = transform.Orientation;
            return value;
        }

        private void OnValueChanged(ValueBox<float> valueBox)
        {
            var control = valueBox.Parent;
            var box = (InputBox)control.Tag;
            var x = ((FloatValueBox)control.Children[0]).Value;
            var y = ((FloatValueBox)control.Children[1]).Value;
            var z = ((FloatValueBox)control.Children[2]).Value;
            box.Value = Quaternion.Euler(x, y, z);
        }
    }

    class ColorDefaultValueEditor : IDefaultValueEditor
    {
        public bool CanUse(InputBox box, ref ScriptType type)
        {
            return type.Type == typeof(Color);
        }

        public Control Create(InputBox box, ref Rectangle bounds)
        {
            var value = GetValue(box);
            var control = new ColorValueBox(value, bounds.X, bounds.Y)
            {
                Height = bounds.Height,
                Parent = box.Parent,
                Tag = box,
            };
            control.ColorValueChanged += OnValueChanged;
            return control;
        }

        public bool IsValid(InputBox box, Control control)
        {
            return control is ColorValueBox;
        }

        public void UpdateDefaultValue(InputBox box, Control control)
        {
            if (control is ColorValueBox colorValueBox)
            {
                colorValueBox.Value = GetValue(box);
            }
        }

        public void UpdateAttributes(InputBox box, object[] attributes, Control control)
        {
        }

        private Vector4 GetValue(InputBox box)
        {
            return (Color)(Float4)SurfaceUtils.GetDouble4(box.Value, 1.0f);
        }

        private void OnValueChanged(ColorValueBox control)
        {
            var box = (InputBox)control.Tag;
            box.Value = control.Value;
        }
    }

    class EnumDefaultValueEditor : IDefaultValueEditor
    {
        public bool CanUse(InputBox box, ref ScriptType type)
        {
            return type.IsEnum;
        }

        public Control Create(InputBox box, ref Rectangle bounds)
        {
            var value = GetValue(box);
            var control = new EnumComboBox(box.CurrentType.Type)
            {
                Location = new Float2(bounds.X, bounds.Y),
                Size = new Float2(60.0f, bounds.Height),
                EnumTypeValue = value ?? box.CurrentType.CreateInstance(),
                Parent = box.Parent,
                Tag = box,
            };
            control.EnumValueChanged += OnValueChanged;
            return control;
        }

        private void OnValueChanged(EnumComboBox control)
        {
            var box = (InputBox)control.Tag;
            box.Value = control.EnumTypeValue;
        }

        public bool IsValid(InputBox box, Control control)
        {
            return control is EnumComboBox;
        }

        public void UpdateDefaultValue(InputBox box, Control control)
        {
            if (control is EnumComboBox enumComboBox)
            {
                enumComboBox.EnumTypeValue = GetValue(box);
            }
        }

        public void UpdateAttributes(InputBox box, object[] attributes, Control control)
        {
        }

        private object GetValue(InputBox box)
        {
            var value = box.CurrentType.CreateInstance();
            var v = box.Value;
            if (v != null && v.GetType().IsEnum)
                value = v;
            return value;
        }
    }

    class TypeDefaultValueEditor : IDefaultValueEditor
    {
        public bool CanUse(InputBox box, ref ScriptType type)
        {
            if (type.Type == typeof(string) && box.Attributes != null && box.Attributes.Any(x => x.GetType() == typeof(TypeReferenceAttribute)))
                return true;
            return type.Type == typeof(Type) || type.Type == typeof(ScriptType);
        }

        public Control Create(InputBox box, ref Rectangle bounds)
        {
            var value = GetValue(box);
            var control = new TypePickerControl
            {
                Location = new Float2(bounds.X, bounds.Y),
                Size = new Float2(60.0f, bounds.Height),
                ValueTypeName = value,
                Parent = box.Parent,
                Tag = box,
            };
            if (box.CurrentType.Type == typeof(Type))
                control.CheckValid = type => type.Type != null;
            control.TypePickerValueChanged += OnValueChanged;
            return control;
        }

        private void OnValueChanged(TypePickerControl control)
        {
            var box = (InputBox)control.Tag;
            var v = box.Value;
            if (v is string)
                box.Value = control.ValueTypeName;
            else if (v is Type)
                box.Value = TypeUtils.GetType(control.Value);
            else
                box.Value = control.Value;
        }

        public bool IsValid(InputBox box, Control control)
        {
            return control is TypePickerControl;
        }

        public void UpdateDefaultValue(InputBox box, Control control)
        {
            if (control is TypePickerControl typePickerControl)
            {
                typePickerControl.ValueTypeName = GetValue(box);
            }
        }

        public void UpdateAttributes(InputBox box, object[] attributes, Control control)
        {
            var typeReference = (TypeReferenceAttribute)attributes.FirstOrDefault(x => x.GetType() == typeof(TypeReferenceAttribute));
            var type = typeReference != null ? TypeUtils.GetType(typeReference.TypeName) : ScriptType.Null;
            ((TypePickerControl)control).Type = type ? type : ScriptType.Object;
        }

        private string GetValue(InputBox box)
        {
            var v = box.Value;
            if (v is Type asType)
                return asType.FullName;
            if (v is ScriptType asScriptType)
                return asScriptType.TypeName;
            if (v is string asString)
                return asString;
            return string.Empty;
        }
    }

    /// <summary>
    /// Visject Surface input box element.
    /// </summary>
    /// <seealso cref="FlaxEditor.Surface.Elements.Box" />
    [HideInEditor]
    public class InputBox : Box
    {
        private Control _defaultValueEditor;
        private IDefaultValueEditor _editor;
        private int _valueIndex;
        private Func<InputBox, object> _customValueGetter;
        private Action<InputBox, object> _customValueSetter;
        private bool _isWatchingForValueChange;

        /// <summary>
        /// The handlers for the input box default value editing. Used to display the default to the UI.
        /// </summary>
        public static readonly List<IDefaultValueEditor> DefaultValueEditors = new List<IDefaultValueEditor>()
        {
            new BooleanDefaultValueEditor(),
            new IntegerDefaultValueEditor(),
            new UnsignedIntegerDefaultValueEditor(),
            new FloatingDefaultValueEditor(),
            new StringDefaultValueEditor(),
            new Vector2DefaultValueEditor(),
            new Vector3DefaultValueEditor(),
            new Vector4DefaultValueEditor(),
            new Float2DefaultValueEditor(),
            new Float3DefaultValueEditor(),
            new Float4DefaultValueEditor(),
            new Double2DefaultValueEditor(),
            new Double3DefaultValueEditor(),
            new Double4DefaultValueEditor(),
            new QuaternionDefaultValueEditor(),
            new ColorDefaultValueEditor(),
            new EnumDefaultValueEditor(),
            new TypeDefaultValueEditor(),
        };

        /// <summary>
        /// Gets the control that is used to edit default value (optional).
        /// </summary>
        public Control DefaultValueEditor => _defaultValueEditor;

        /// <summary>
        /// Gets or sets the value.
        /// </summary>
        public object Value
        {
            get => _customValueGetter != null ? _customValueGetter(this) : ParentNode.Values[_valueIndex];
            set
            {
                if (_customValueSetter != null)
                    _customValueSetter(this, value);
                else
                    ParentNode.SetValue(_valueIndex, value);
            }
        }

        /// <summary>
        /// Returns true if node has custom value access.
        /// </summary>
        public bool HasCustomValueAccess => _customValueGetter != null;

        /// <summary>
        /// Returns true if node has value.
        /// </summary>
        public bool HasValue => _valueIndex != -1 || _customValueGetter != null;

        /// <inheritdoc />
        public InputBox(SurfaceNode parentNode, NodeElementArchetype archetype)
        : base(parentNode, archetype, archetype.Position)
        {
            // Check if use inlined default value editor
            _valueIndex = Archetype.ValueIndex;
            if (_valueIndex != -1)
            {
                _isWatchingForValueChange = true;
                ParentNode.ValuesChanged += UpdateDefaultValue;
            }
        }

        /// <summary>
        /// Updates the default value editor UI value.
        /// </summary>
        public void UpdateDefaultValue()
        {
            if (_defaultValueEditor != null && _currentType.Type != null)
            {
                _editor.UpdateDefaultValue(this, _defaultValueEditor);
            }
        }

        /// <summary>
        /// Sets the custom accessor callbacks for the box value.
        /// </summary>
        /// <param name="getter">The function to call to get value for the box.</param>
        /// <param name="setter">The function to call to set value for the box.</param>
        public void UseCustomValueAccess(Func<InputBox, object> getter, Action<InputBox, object> setter)
        {
            _customValueGetter = getter;
            _customValueSetter = setter;
            if (Connections.Count == 0)
                CreateDefaultEditor();
            if (!_isWatchingForValueChange)
            {
                _isWatchingForValueChange = true;
                ParentNode.ValuesChanged += UpdateDefaultValue;
            }
        }

        /// <inheritdoc />
        public override bool IsOutput => false;

        /// <inheritdoc />
        protected override void OnLocationChanged()
        {
            base.OnLocationChanged();

            if (_defaultValueEditor != null)
            {
                _defaultValueEditor.Location = new Float2(X + Width + 8 + Style.Current.FontSmall.MeasureText(Text).X, Y);
            }
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            // Box
            Surface.Style.DrawBox(this);

            // Draw text
            var style = Style.Current;
            var rect = new Rectangle(Width + 4, 0, 1410, Height);
            Render2D.DrawText(style.FontSmall, Text, rect, Enabled ? style.Foreground : style.ForegroundDisabled, TextAlignment.Near, TextAlignment.Center);
        }

        /// <inheritdoc />
        protected override void OnCurrentTypeChanged()
        {
            base.OnCurrentTypeChanged();

            if (_defaultValueEditor != null && !(_editor.IsValid(this, _defaultValueEditor) && _editor.CanUse(this, ref _currentType)))
            {
                _defaultValueEditor.Dispose();
                _defaultValueEditor = null;
                _editor = null;
            }

            if (Connections.Count == 0)
            {
                CreateDefaultEditor();
            }
        }

        /// <inheritdoc />
        public override void OnConnectionsChanged()
        {
            bool showEditor = Connections.Count == 0 && HasValue;
            if (showEditor)
            {
                CreateDefaultEditor();
            }

            if (_defaultValueEditor != null)
            {
                _defaultValueEditor.Enabled = showEditor;
                _defaultValueEditor.Visible = showEditor;
            }
        }

        /// <inheritdoc />
        protected override void OnAttributesChanged()
        {
            OnCurrentTypeChanged();
            _editor?.UpdateAttributes(this, _attributes ?? Utils.GetEmptyArray<object>(), _defaultValueEditor);
        }

        /// <inheritdoc />
        public override void OnSurfaceCanEditChanged(bool canEdit)
        {
            base.OnSurfaceCanEditChanged(canEdit);

            if (_defaultValueEditor != null)
            {
                _defaultValueEditor.Enabled = canEdit;
            }
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Right && HasValue)
            {
                var menu = new FlaxEditor.GUI.ContextMenu.ContextMenu();
                menu.AddButton("Copy value", OnCopyValue);
                var paste = menu.AddButton("Paste value", OnPasteValue);
                try
                {
                    GetClipboardValue(out _, false);
                }
                catch
                {
                    paste.Enabled = false;
                }

                menu.Show(this, location);
                return true;
            }

            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            if (_defaultValueEditor != null)
            {
                _defaultValueEditor.Dispose();
                _defaultValueEditor = null;
                _editor = null;
            }

            base.OnDestroy();
        }

        private bool GetClipboardValue(out object result, bool deserialize)
        {
            result = null;
            var text = Clipboard.Text;
            if (string.IsNullOrEmpty(text))
                return false;

            object obj;
            var type = CurrentType;
            if (ScriptType.FlaxObject.IsAssignableFrom(type))
            {
                // Object reference
                if (text.Length != 32)
                    return false;
                JsonSerializer.ParseID(text, out var id);
                obj = FlaxEngine.Object.Find<FlaxEngine.Object>(ref id);
            }
            else
            {
                // Default
                obj = JsonConvert.DeserializeObject(text, TypeUtils.GetType(type), JsonSerializer.Settings);
            }

            if (obj == null || type.IsInstanceOfType(obj))
            {
                result = obj;
                return true;
            }

            return false;
        }

        private void OnCopyValue()
        {
            var value = Value;
            try
            {
                string text;
                if (value == null)
                {
                    // Missing value
                    var type = CurrentType;
                    if (type.Type == typeof(bool))
                        text = "false";
                    else if (type.Type == typeof(byte) || type.Type == typeof(sbyte) || type.Type == typeof(char) || type.Type == typeof(short) || type.Type == typeof(ushort) || type.Type == typeof(int) || type.Type == typeof(uint) || type.Type == typeof(long) || type.Type == typeof(ulong))
                        text = "0";
                    else if (type.Type == typeof(float) || type.Type == typeof(double))
                        text = "0.0";
                    else if (type.Type == typeof(Vector2) || type.Type == typeof(Vector3) || type.Type == typeof(Vector4) || type.Type == typeof(Color))
                        text = JsonSerializer.Serialize(TypeUtils.GetDefaultValue(type));
                    else if (type.Type == typeof(string))
                        text = "";
                    else
                        text = "null";
                }
                else if (value is FlaxEngine.Object asObject)
                {
                    // Object reference
                    text = JsonSerializer.GetStringID(asObject);
                }
                else
                {
                    // Default
                    text = JsonSerializer.Serialize(value);
                }
                Clipboard.Text = text;
            }
            catch (Exception ex)
            {
                Editor.LogWarning(ex);
                Editor.LogError("Cannot copy property. See log for more info.");
            }
        }

        private void OnPasteValue()
        {
            try
            {
                if (GetClipboardValue(out var value, true))
                    Value = value;
            }
            catch (Exception ex)
            {
                Editor.LogWarning(ex);
                Editor.LogError("Cannot paste property value. See log for more info.");
            }
        }

        /// <summary>
        /// Creates the default value editor control.
        /// </summary>
        private void CreateDefaultEditor()
        {
            if (_defaultValueEditor != null || !HasValue)
                return;

            for (int i = 0; i < DefaultValueEditors.Count; i++)
            {
                if (DefaultValueEditors[i].CanUse(this, ref _currentType))
                {
                    var bounds = new Rectangle(X + Width + 8 + Style.Current.FontSmall.MeasureText(Text).X, Y, 90, Height);
                    _editor = DefaultValueEditors[i];
                    try
                    {
                        _defaultValueEditor = _editor.Create(this, ref bounds);
                        if (_attributes != null)
                            _editor.UpdateAttributes(this, _attributes, _defaultValueEditor);
                    }
                    catch (Exception ex)
                    {
                        Editor.LogWarning(ex);
                        _editor = null;
                    }
                    break;
                }
            }
        }
    }
}
