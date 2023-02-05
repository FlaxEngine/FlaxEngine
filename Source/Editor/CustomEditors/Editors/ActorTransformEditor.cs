// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using FlaxEditor.CustomEditors.GUI;
using FlaxEngine;
using FlaxEngine.GUI;

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
        /// Custom editor for actor position property.
        /// </summary>
        /// <seealso cref="FlaxEditor.CustomEditors.Editors.Vector3Editor" />
        public class PositionEditor : Vector3Editor
        {
            /// <inheritdoc />
            public override void Initialize(LayoutElementsContainer layout)
            {
                base.Initialize(layout);

                // Override colors
                var back = FlaxEngine.GUI.Style.Current.TextBoxBackground;
                var grayOutFactor = 0.6f;
                XElement.ValueBox.BorderColor = Color.Lerp(AxisColorX, back, grayOutFactor);
                XElement.ValueBox.BorderSelectedColor = AxisColorX;
                YElement.ValueBox.BorderColor = Color.Lerp(AxisColorY, back, grayOutFactor);
                YElement.ValueBox.BorderSelectedColor = AxisColorY;
                ZElement.ValueBox.BorderColor = Color.Lerp(AxisColorZ, back, grayOutFactor);
                ZElement.ValueBox.BorderSelectedColor = AxisColorZ;
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
                XElement.ValueBox.BorderColor = Color.Lerp(AxisColorX, back, grayOutFactor);
                XElement.ValueBox.BorderSelectedColor = AxisColorX;
                YElement.ValueBox.BorderColor = Color.Lerp(AxisColorY, back, grayOutFactor);
                YElement.ValueBox.BorderSelectedColor = AxisColorY;
                ZElement.ValueBox.BorderColor = Color.Lerp(AxisColorZ, back, grayOutFactor);
                ZElement.ValueBox.BorderSelectedColor = AxisColorZ;
            }
        }

        /// <summary>
        /// Custom editor for actor scale property.
        /// </summary>
        /// <seealso cref="FlaxEditor.CustomEditors.Editors.Float3Editor" />
        public class ScaleEditor : Float3Editor
        {
            /// <inheritdoc />
            public override void Initialize(LayoutElementsContainer layout)
            {
                base.Initialize(layout);

                // Override colors
                var back = FlaxEngine.GUI.Style.Current.TextBoxBackground;
                var grayOutFactor = 0.6f;
                XElement.ValueBox.BorderColor = Color.Lerp(AxisColorX, back, grayOutFactor);
                XElement.ValueBox.BorderSelectedColor = AxisColorX;
                
                if (XElement.ValueBox.Parent.Parent is PropertiesList list)
                {
                    foreach (var child in list.Children)
                    {
                        if (!(child is PropertyNameLabel))
                            continue;

                        var nameLabel = child as PropertyNameLabel;
                        if (!string.Equals(nameLabel.Text, "Scale"))
                            continue;
                        
                        var lockButton = new Button
                        {
                            Parent = nameLabel,
                            Width = 18,
                            Height = 18,
                            BackgroundBrush = new SpriteBrush(Editor.Instance.Icons.Link32),
                            BackgroundColor = Color.White,
                            BorderColor = Color.Transparent,
                            BorderColorSelected = Color.Transparent,
                            BorderColorHighlighted = Color.Transparent,
                            AnchorPreset = AnchorPresets.TopLeft,
                            TooltipText = "Locks/Unlocks setting the scale values.",
                        };
                        lockButton.LocalX += 40;
                        lockButton.LocalY += 1;
                        lockButton.Clicked += () =>
                        {
                            ChangeValuesTogether = !ChangeValuesTogether;
                            // TODO: change image
                            Debug.Log(ChangeValuesTogether);
                        };

                        break;
                    }
                }
                
                YElement.ValueBox.BorderColor = Color.Lerp(AxisColorY, back, grayOutFactor);
                YElement.ValueBox.BorderSelectedColor = AxisColorY;
                ZElement.ValueBox.BorderColor = Color.Lerp(AxisColorZ, back, grayOutFactor);
                ZElement.ValueBox.BorderSelectedColor = AxisColorZ;
                
                Debug.Log(YElement.ValueBox.Parent);
                Debug.Log(YElement.ValueBox.Parent.Parent);
                Debug.Log(YElement.ValueBox.Parent.Parent.Parent);
                Debug.Log(YElement.ValueBox.Parent.Parent.Parent.Parent);
                Debug.Log(YElement.ValueBox.Parent.Parent.Parent.Parent.Parent);
            }
        }
    }
}
