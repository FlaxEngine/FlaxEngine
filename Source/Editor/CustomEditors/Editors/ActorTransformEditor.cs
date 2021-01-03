// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Actor transform editor.
    /// </summary>
    [HideInEditor]
    public static class ActorTransformEditor
    {
        /// <summary>
        /// The X axis color.
        /// </summary>
        public static Color AxisColorX = new Color(1.0f, 0.0f, 0.02745f, 1.0f);

        /// <summary>
        /// The Y axis color.
        /// </summary>
        public static Color AxisColorY = new Color(0.239215f, 1.0f, 0.047058f, 1.0f);

        /// <summary>
        /// The Z axis color.
        /// </summary>
        public static Color AxisColorZ = new Color(0.0f, 0.0235294f, 1.0f, 1.0f);

        /// <summary>
        /// Custom editor for actor position/scale property.
        /// </summary>
        /// <seealso cref="FlaxEditor.CustomEditors.Editors.Vector3Editor" />
        public class PositionScaleEditor : Vector3Editor
        {
            /// <inheritdoc />
            public override void Initialize(LayoutElementsContainer layout)
            {
                base.Initialize(layout);

                // Override colors
                var back = FlaxEngine.GUI.Style.Current.TextBoxBackground;
                var grayOutFactor = 0.6f;
                XElement.FloatValue.BorderColor = Color.Lerp(AxisColorX, back, grayOutFactor);
                XElement.FloatValue.BorderSelectedColor = AxisColorX;
                YElement.FloatValue.BorderColor = Color.Lerp(AxisColorY, back, grayOutFactor);
                YElement.FloatValue.BorderSelectedColor = AxisColorY;
                ZElement.FloatValue.BorderColor = Color.Lerp(AxisColorZ, back, grayOutFactor);
                ZElement.FloatValue.BorderSelectedColor = AxisColorZ;
            }
        }

        /// <summary>
        /// Custom editor for actor orientation property.
        /// </summary>
        /// <seealso cref="FlaxEditor.CustomEditors.Editors.QuaternionEditor" />
        public class OrientationEditor : QuaternionEditor
        {
            /// <inheritdoc />
            public override void Initialize(LayoutElementsContainer layout)
            {
                base.Initialize(layout);

                // Override colors
                var back = FlaxEngine.GUI.Style.Current.TextBoxBackground;
                var grayOutFactor = 0.6f;
                XElement.FloatValue.BorderColor = Color.Lerp(AxisColorX, back, grayOutFactor);
                XElement.FloatValue.BorderSelectedColor = AxisColorX;
                YElement.FloatValue.BorderColor = Color.Lerp(AxisColorY, back, grayOutFactor);
                YElement.FloatValue.BorderSelectedColor = AxisColorY;
                ZElement.FloatValue.BorderColor = Color.Lerp(AxisColorZ, back, grayOutFactor);
                ZElement.FloatValue.BorderSelectedColor = AxisColorZ;
            }
        }
    }
}
