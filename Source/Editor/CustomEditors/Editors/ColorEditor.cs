// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.GUI.Input;
using FlaxEngine;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Default implementation of the inspector used to edit Color value type properties.
    /// </summary>
    [CustomEditor(typeof(Color)), DefaultEditor]
    public sealed class ColorEditor : CustomEditor
    {
        private CustomElement<ColorValueBox> element;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            element = layout.Custom<ColorValueBox>();
            element.CustomControl.ValueChanged += OnValueChanged;
        }

        private void OnValueChanged()
        {
            var token = element.CustomControl.IsSliding ? this : null;
            var value = Values[0];
            var color = element.CustomControl.Value;
            if (value is Color)
                SetValue(color, token);
            else if (value is Color32)
            {
                Color32 color32;
                color.ToBgra(out color32.R, out color32.G, out color32.B, out color32.A);
                SetValue(color32, token);
            }
            else if (value is Float4)
                SetValue(new Float4(color.R, color.G, color.B, color.A), token);
            else if (value is Double4)
                SetValue(new Double4(color.R, color.G, color.B, color.A), token);
            else if (value is Vector4)
                SetValue(new Vector4(color.R, color.G, color.B, color.A), token);
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            if (HasDifferentValues)
            {
                // TODO: support different values on ColorValueBox
            }
            else
            {
                var value = Values[0];
                if (value is Color asColor)
                    element.CustomControl.Value = asColor;
                else if (value is Color32 asColor32)
                    element.CustomControl.Value = new Color(asColor32.R, asColor32.G, asColor32.B, asColor32.A);
                else if (value is Float4 asFloat4)
                    element.CustomControl.Value = new Color(asFloat4.X, asFloat4.Y, asFloat4.Z, asFloat4.W);
                else if (value is Double4 asDouble4)
                    element.CustomControl.Value = new Color((float)asDouble4.X, (float)asDouble4.Y, (float)asDouble4.Z, (float)asDouble4.W);
                else if (value is Vector4 asVector4)
                    element.CustomControl.Value = new Color(asVector4.X, asVector4.Y, asVector4.Z, asVector4.W);
            }
        }
    }
}
