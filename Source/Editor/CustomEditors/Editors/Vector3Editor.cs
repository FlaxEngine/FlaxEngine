// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System.Linq;
using FlaxEditor.CustomEditors.Elements;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Default implementation of the inspector used to edit Vector3 value type properties.
    /// </summary>
    [CustomEditor(typeof(Vector3)), DefaultEditor]
    public class Vector3Editor :
#if USE_LARGE_WORLDS
    Double3Editor
#else
    Float3Editor
#endif
    {
    }

    /// <summary>
    /// Default implementation of the inspector used to edit Float3 value type properties.
    /// </summary>
    [CustomEditor(typeof(Float3)), DefaultEditor]
    public class Float3Editor : CustomEditor
    {
        /// <summary>
        /// The X component editor.
        /// </summary>
        protected FloatValueElement XElement;

        /// <summary>
        /// The Y component editor.
        /// </summary>
        protected FloatValueElement YElement;

        /// <summary>
        /// The Z component editor.
        /// </summary>
        protected FloatValueElement ZElement;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <summary>
        /// If true, when one value is changed, the other 2 will change as well.
        /// </summary>
        protected bool ChangeValuesTogether = false;
        private bool _xChanged;
        private bool _yChanged;
        private bool _zChanged;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            var grid = layout.CustomContainer<UniformGridPanel>();
            var gridControl = grid.CustomControl;
            gridControl.ClipChildren = false;
            gridControl.Height = TextBox.DefaultHeight;
            gridControl.SlotsHorizontally = 3;
            gridControl.SlotsVertically = 1;

            LimitAttribute limit = null;
            var attributes = Values.GetAttributes();
            if (attributes != null)
            {
                limit = (LimitAttribute)attributes.FirstOrDefault(x => x is LimitAttribute);
            }

            XElement = grid.FloatValue();
            XElement.SetLimits(limit);
            XElement.ValueBox.ValueChanged += OnXValueChanged;
            XElement.ValueBox.SlidingEnd += ClearToken;

            YElement = grid.FloatValue();
            YElement.SetLimits(limit);
            YElement.ValueBox.ValueChanged += OnYValueChanged;
            YElement.ValueBox.SlidingEnd += ClearToken;

            ZElement = grid.FloatValue();
            ZElement.SetLimits(limit);
            ZElement.ValueBox.ValueChanged += OnZValueChanged;
            ZElement.ValueBox.SlidingEnd += ClearToken;
        }

        private void OnXValueChanged()
        {
            if (IsSetBlocked)
                return;
            if (ChangeValuesTogether)
            {
                _xChanged = true;
                _yChanged = false;
                _zChanged = false;
            }
            OnValueChanged();
        }
        
        private void OnYValueChanged()
        {
            if (IsSetBlocked)
                return;
            if (ChangeValuesTogether)
            {
                _xChanged = false;
                _yChanged = true;
                _zChanged = false;
            }
            OnValueChanged();
        }
        
        private void OnZValueChanged()
        {
            if (IsSetBlocked)
                return;
            if (ChangeValuesTogether)
            {
                _xChanged = false;
                _yChanged = false;
                _zChanged = true;
            }
            OnValueChanged();
        }
        
        private void OnValueChanged()
        {
            if (IsSetBlocked)
                return;

            var xValue = XElement.ValueBox.Value;
            var yValue = YElement.ValueBox.Value;
            var zValue = ZElement.ValueBox.Value;

            if (ChangeValuesTogether)
            {
                var adder = 0.0f;
                if (_xChanged)
                {
                    adder = xValue - ((Float3)Values[0]).X;
                    yValue += adder;
                    zValue += adder;
                }

                if (_yChanged)
                {
                    adder = yValue - ((Float3)Values[0]).Y;
                    xValue += adder;
                    zValue += adder;
                }
                
                if (_zChanged)
                {
                    adder = zValue - ((Float3)Values[0]).Z;
                    xValue += adder;
                    yValue += adder;
                }
            }

            var isSliding = XElement.IsSliding || YElement.IsSliding || ZElement.IsSliding;
            var token = isSliding ? this : null;
            var value = new Float3(xValue, yValue, zValue);
            object v = Values[0];
            if (v is Vector3)
                v = (Vector3)value;
            else if (v is Float3)
                v = (Float3)value;
            else if (v is Double3)
                v = (Double3)value;
            SetValue(v, token);
            
            if (ChangeValuesTogether)
            {
                _xChanged = false;
                _yChanged = false;
                _zChanged = false;
            }
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            if (HasDifferentValues)
            {
                // TODO: support different values for ValueBox<T>
            }
            else
            {
                var value = Float3.Zero;
                if (Values[0] is Vector3 asVector3)
                    value = asVector3;
                else if (Values[0] is Float3 asFloat3)
                    value = asFloat3;
                else if (Values[0] is Double3 asDouble3)
                    value = asDouble3;
                XElement.ValueBox.Value = value.X;
                YElement.ValueBox.Value = value.Y;
                ZElement.ValueBox.Value = value.Z;
            }
        }
    }

    /// <summary>
    /// Default implementation of the inspector used to edit Double3 value type properties.
    /// </summary>
    [CustomEditor(typeof(Double3)), DefaultEditor]
    public class Double3Editor : CustomEditor
    {
        /// <summary>
        /// The X component editor.
        /// </summary>
        protected DoubleValueElement XElement;

        /// <summary>
        /// The Y component editor.
        /// </summary>
        protected DoubleValueElement YElement;

        /// <summary>
        /// The Z component editor.
        /// </summary>
        protected DoubleValueElement ZElement;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            var grid = layout.CustomContainer<UniformGridPanel>();
            var gridControl = grid.CustomControl;
            gridControl.ClipChildren = false;
            gridControl.Height = TextBox.DefaultHeight;
            gridControl.SlotsHorizontally = 3;
            gridControl.SlotsVertically = 1;

            LimitAttribute limit = null;
            var attributes = Values.GetAttributes();
            if (attributes != null)
            {
                limit = (LimitAttribute)attributes.FirstOrDefault(x => x is LimitAttribute);
            }

            XElement = grid.DoubleValue();
            XElement.SetLimits(limit);
            XElement.ValueBox.ValueChanged += OnValueChanged;
            XElement.ValueBox.SlidingEnd += ClearToken;

            YElement = grid.DoubleValue();
            YElement.SetLimits(limit);
            YElement.ValueBox.ValueChanged += OnValueChanged;
            YElement.ValueBox.SlidingEnd += ClearToken;

            ZElement = grid.DoubleValue();
            ZElement.SetLimits(limit);
            ZElement.ValueBox.ValueChanged += OnValueChanged;
            ZElement.ValueBox.SlidingEnd += ClearToken;
        }

        private void OnValueChanged()
        {
            if (IsSetBlocked)
                return;

            var isSliding = XElement.IsSliding || YElement.IsSliding || ZElement.IsSliding;
            var token = isSliding ? this : null;
            var value = new Double3(XElement.ValueBox.Value, YElement.ValueBox.Value, ZElement.ValueBox.Value);
            object v = Values[0];
            if (v is Vector3)
                v = (Vector3)value;
            else if (v is Float3)
                v = (Float3)value;
            else if (v is Double3)
                v = (Double3)value;
            SetValue(v, token);
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            if (HasDifferentValues)
            {
                // TODO: support different values for ValueBox<T>
            }
            else
            {
                var value = Double3.Zero;
                if (Values[0] is Vector3 asVector3)
                    value = asVector3;
                else if (Values[0] is Float3 asFloat3)
                    value = asFloat3;
                else if (Values[0] is Double3 asDouble3)
                    value = asDouble3;
                XElement.ValueBox.Value = value.X;
                YElement.ValueBox.Value = value.Y;
                ZElement.ValueBox.Value = value.Z;
            }
        }
    }

    /// <summary>
    /// Default implementation of the inspector used to edit Int3 value type properties.
    /// </summary>
    [CustomEditor(typeof(Int3)), DefaultEditor]
    public class Int3Editor : CustomEditor
    {
        /// <summary>
        /// The X component editor.
        /// </summary>
        protected IntegerValueElement XElement;

        /// <summary>
        /// The Y component editor.
        /// </summary>
        protected IntegerValueElement YElement;

        /// <summary>
        /// The Z component editor.
        /// </summary>
        protected IntegerValueElement ZElement;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            var grid = layout.CustomContainer<UniformGridPanel>();
            var gridControl = grid.CustomControl;
            gridControl.ClipChildren = false;
            gridControl.Height = TextBox.DefaultHeight;
            gridControl.SlotsHorizontally = 3;
            gridControl.SlotsVertically = 1;

            LimitAttribute limit = null;
            var attributes = Values.GetAttributes();
            if (attributes != null)
            {
                limit = (LimitAttribute)attributes.FirstOrDefault(x => x is LimitAttribute);
            }

            XElement = grid.IntegerValue();
            XElement.SetLimits(limit);
            XElement.IntValue.ValueChanged += OnValueChanged;
            XElement.IntValue.SlidingEnd += ClearToken;

            YElement = grid.IntegerValue();
            YElement.SetLimits(limit);
            YElement.IntValue.ValueChanged += OnValueChanged;
            YElement.IntValue.SlidingEnd += ClearToken;

            ZElement = grid.IntegerValue();
            ZElement.SetLimits(limit);
            ZElement.IntValue.ValueChanged += OnValueChanged;
            ZElement.IntValue.SlidingEnd += ClearToken;
        }

        private void OnValueChanged()
        {
            if (IsSetBlocked)
                return;

            var isSliding = XElement.IsSliding || YElement.IsSliding || ZElement.IsSliding;
            var token = isSliding ? this : null;
            var value = new Int3(XElement.IntValue.Value, YElement.IntValue.Value, ZElement.IntValue.Value);
            SetValue(value, token);
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            if (HasDifferentValues)
            {
                // TODO: support different values for ValueBox<T>
            }
            else
            {
                var value = (Int3)Values[0];
                XElement.IntValue.Value = value.X;
                YElement.IntValue.Value = value.Y;
                ZElement.IntValue.Value = value.Z;
            }
        }
    }
}
