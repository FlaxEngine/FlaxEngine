// Copyright (c) Wojciech Figat. All rights reserved.

using System.Linq;
using FlaxEditor.CustomEditors.Elements;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Default implementation of the inspector used to edit Vector4 value type properties.
    /// </summary>
    [CustomEditor(typeof(Vector4)), DefaultEditor]
    public class Vector4Editor :
#if USE_LARGE_WORLDS
    Double4Editor
#else
    Float4Editor
#endif
    {
    }

    /// <summary>
    /// Default implementation of the inspector used to edit Vector4 value type properties.
    /// </summary>
    [CustomEditor(typeof(Float4)), DefaultEditor]
    public class Float4Editor : CustomEditor
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

        /// <summary>
        /// The W component editor.
        /// </summary>
        protected FloatValueElement WElement;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            var grid = layout.UniformGrid();
            var gridControl = grid.CustomControl;
            gridControl.ClipChildren = false;
            gridControl.Height = TextBox.DefaultHeight;
            gridControl.SlotsHorizontally = 4;
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

            ZElement = grid.FloatValue();
            ZElement.SetLimits(limit);
            ZElement.ValueBox.ValueChanged += OnValueChanged;
            ZElement.ValueBox.SlidingEnd += ClearToken;

            WElement = grid.FloatValue();
            WElement.SetLimits(limit);
            WElement.ValueBox.ValueChanged += OnValueChanged;
            WElement.ValueBox.SlidingEnd += ClearToken;
        }

        private void OnValueChanged()
        {
            if (IsSetBlocked || Values == null)
                return;

            var isSliding = XElement.IsSliding || YElement.IsSliding || ZElement.IsSliding || WElement.IsSliding;
            var token = isSliding ? this : null;
            var value = new Float4(XElement.ValueBox.Value, YElement.ValueBox.Value, ZElement.ValueBox.Value, WElement.ValueBox.Value);
            object v = Values[0];
            if (v is Vector4)
                v = (Vector4)value;
            else if (v is Float4)
                v = (Float4)value;
            else if (v is Double4)
                v = (Double4)value;
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
                var value = Float4.Zero;
                if (Values[0] is Vector4 asVector4)
                    value = asVector4;
                else if (Values[0] is Float4 asFloat4)
                    value = asFloat4;
                else if (Values[0] is Double4 asDouble4)
                    value = asDouble4;
                XElement.ValueBox.Value = value.X;
                YElement.ValueBox.Value = value.Y;
                ZElement.ValueBox.Value = value.Z;
                WElement.ValueBox.Value = value.W;
            }
        }
    }

    /// <summary>
    /// Default implementation of the inspector used to edit Double4 value type properties.
    /// </summary>
    [CustomEditor(typeof(Double4)), DefaultEditor]
    public class Double4Editor : CustomEditor
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

        /// <summary>
        /// The W component editor.
        /// </summary>
        protected DoubleValueElement WElement;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            var grid = layout.UniformGrid();
            var gridControl = grid.CustomControl;
            gridControl.ClipChildren = false;
            gridControl.Height = TextBox.DefaultHeight;
            gridControl.SlotsHorizontally = 4;
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

            WElement = grid.DoubleValue();
            WElement.SetLimits(limit);
            WElement.ValueBox.ValueChanged += OnValueChanged;
            WElement.ValueBox.SlidingEnd += ClearToken;
        }

        private void OnValueChanged()
        {
            if (IsSetBlocked || Values == null)
                return;

            var isSliding = XElement.IsSliding || YElement.IsSliding || ZElement.IsSliding || WElement.IsSliding;
            var token = isSliding ? this : null;
            var value = new Double4(XElement.ValueBox.Value, YElement.ValueBox.Value, ZElement.ValueBox.Value, WElement.ValueBox.Value);
            object v = Values[0];
            if (v is Vector4)
                v = (Vector4)value;
            else if (v is Float4)
                v = (Float4)value;
            else if (v is Double4)
                v = (Double4)value;
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
                var value = Double4.Zero;
                if (Values[0] is Vector4 asVector4)
                    value = asVector4;
                else if (Values[0] is Float4 asFloat4)
                    value = asFloat4;
                else if (Values[0] is Double4 asDouble4)
                    value = asDouble4;
                XElement.ValueBox.Value = value.X;
                YElement.ValueBox.Value = value.Y;
                ZElement.ValueBox.Value = value.Z;
                WElement.ValueBox.Value = value.W;
            }
        }
    }

    /// <summary>
    /// Default implementation of the inspector used to edit Int4 value type properties.
    /// </summary>
    [CustomEditor(typeof(Int4)), DefaultEditor]
    public class Int4Editor : CustomEditor
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

        /// <summary>
        /// The W component editor.
        /// </summary>
        protected IntegerValueElement WElement;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            var grid = layout.UniformGrid();
            var gridControl = grid.CustomControl;
            gridControl.ClipChildren = false;
            gridControl.Height = TextBox.DefaultHeight;
            gridControl.SlotsHorizontally = 4;
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

            WElement = grid.IntegerValue();
            WElement.SetLimits(limit);
            WElement.IntValue.ValueChanged += OnValueChanged;
            WElement.IntValue.SlidingEnd += ClearToken;
        }

        private void OnValueChanged()
        {
            if (IsSetBlocked || Values == null)
                return;

            var isSliding = XElement.IsSliding || YElement.IsSliding || ZElement.IsSliding || WElement.IsSliding;
            var token = isSliding ? this : null;
            var value = new Int4(XElement.IntValue.Value, YElement.IntValue.Value, ZElement.IntValue.Value, WElement.IntValue.Value);
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
                var value = (Int4)Values[0];
                XElement.IntValue.Value = value.X;
                YElement.IntValue.Value = value.Y;
                ZElement.IntValue.Value = value.Z;
                WElement.IntValue.Value = value.W;
            }
        }
    }
}
