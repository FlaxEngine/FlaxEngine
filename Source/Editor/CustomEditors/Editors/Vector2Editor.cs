// Copyright (c) Wojciech Figat. All rights reserved.

using System.Linq;
using FlaxEditor.CustomEditors.Elements;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Default implementation of the inspector used to edit Vector2 value type properties.
    /// </summary>
    [CustomEditor(typeof(Vector2)), DefaultEditor]
    public class Vector2Editor :
#if USE_LARGE_WORLDS
    Double2Editor
#else
    Float2Editor
#endif
    {
    }

    /// <summary>
    /// Default implementation of the inspector used to edit Float2 value type properties.
    /// </summary>
    [CustomEditor(typeof(Float2)), DefaultEditor]
    public class Float2Editor : CustomEditor
    {
        /// <summary>
        /// The X component editor.
        /// </summary>
        protected FloatValueElement XElement;

        /// <summary>
        /// The Y component editor.
        /// </summary>
        protected FloatValueElement YElement;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            var grid = layout.UniformGrid();
            var gridControl = grid.CustomControl;
            gridControl.ClipChildren = false;
            gridControl.Height = TextBox.DefaultHeight;
            gridControl.SlotsHorizontally = 2;
            gridControl.SlotsVertically = 1;

            LimitAttribute limit = null;
            var attributes = Values.GetAttributes();
            if (attributes != null)
            {
                limit = (LimitAttribute)attributes.FirstOrDefault(x => x is LimitAttribute);
            }

            XElement = grid.FloatValue();
            XElement.SetLimits(limit);
            XElement.ValueBox.ValueChanged += OnValueChanged;
            XElement.ValueBox.SlidingEnd += ClearToken;

            YElement = grid.FloatValue();
            YElement.SetLimits(limit);
            YElement.ValueBox.ValueChanged += OnValueChanged;
            YElement.ValueBox.SlidingEnd += ClearToken;
        }

        private void OnValueChanged()
        {
            if (IsSetBlocked || Values == null)
                return;

            var isSliding = XElement.IsSliding || YElement.IsSliding;
            var token = isSliding ? this : null;
            var value = new Float2(XElement.ValueBox.Value, YElement.ValueBox.Value);
            object v = Values[0];
            if (v is Vector2)
                v = (Vector2)value;
            else if (v is Float2)
                v = (Float2)value;
            else if (v is Double2)
                v = (Double2)value;
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
                var value = Float2.Zero;
                if (Values[0] is Vector2 asVector2)
                    value = asVector2;
                else if (Values[0] is Float2 asFloat2)
                    value = asFloat2;
                else if (Values[0] is Double2 asDouble2)
                    value = asDouble2;
                XElement.ValueBox.Value = value.X;
                YElement.ValueBox.Value = value.Y;
            }
        }
    }

    /// <summary>
    /// Default implementation of the inspector used to edit Double2 value type properties.
    /// </summary>
    [CustomEditor(typeof(Double2)), DefaultEditor]
    public class Double2Editor : CustomEditor
    {
        /// <summary>
        /// The X component editor.
        /// </summary>
        protected DoubleValueElement XElement;

        /// <summary>
        /// The Y component editor.
        /// </summary>
        protected DoubleValueElement YElement;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            var grid = layout.UniformGrid();
            var gridControl = grid.CustomControl;
            gridControl.ClipChildren = false;
            gridControl.Height = TextBox.DefaultHeight;
            gridControl.SlotsHorizontally = 2;
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
        }

        private void OnValueChanged()
        {
            if (IsSetBlocked || Values == null)
                return;

            var isSliding = XElement.IsSliding || YElement.IsSliding;
            var token = isSliding ? this : null;
            var value = new Double2(XElement.ValueBox.Value, YElement.ValueBox.Value);
            object v = Values[0];
            if (v is Vector2)
                v = (Vector2)value;
            else if (v is Float2)
                v = (Float2)value;
            else if (v is Double2)
                v = (Double2)value;
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
                var value = Double2.Zero;
                if (Values[0] is Vector2 asVector2)
                    value = asVector2;
                else if (Values[0] is Float2 asFloat2)
                    value = asFloat2;
                else if (Values[0] is Double2 asDouble2)
                    value = asDouble2;
                XElement.ValueBox.Value = value.X;
                YElement.ValueBox.Value = value.Y;
            }
        }
    }

    /// <summary>
    /// Default implementation of the inspector used to edit Int2 value type properties.
    /// </summary>
    [CustomEditor(typeof(Int2)), DefaultEditor]
    public class Int2Editor : CustomEditor
    {
        /// <summary>
        /// The X component editor.
        /// </summary>
        protected IntegerValueElement XElement;

        /// <summary>
        /// The Y component editor.
        /// </summary>
        protected IntegerValueElement YElement;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            var grid = layout.UniformGrid();
            var gridControl = grid.CustomControl;
            gridControl.ClipChildren = false;
            gridControl.Height = TextBox.DefaultHeight;
            gridControl.SlotsHorizontally = 2;
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
        }

        private void OnValueChanged()
        {
            if (IsSetBlocked || Values == null)
                return;

            var isSliding = XElement.IsSliding || YElement.IsSliding;
            var token = isSliding ? this : null;
            var value = new Int2(XElement.IntValue.Value, YElement.IntValue.Value);
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
                var value = (Int2)Values[0];
                XElement.IntValue.Value = value.X;
                YElement.IntValue.Value = value.Y;
            }
        }
    }
}
