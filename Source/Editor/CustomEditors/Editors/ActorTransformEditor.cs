// Copyright (c) Wojciech Figat. All rights reserved.

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

                if (XElement.ValueBox.Parent is UniformGridPanel ug)
                {
                    ug.SlotPadding = new Margin(3.0f, 0.0f, 0.0f, 0.0f);
                    CheckLayout(ug);
                }

                // Override colors
                XElement.ValueBox.BorderSelectedColor = AxisColorX;
                YElement.ValueBox.BorderSelectedColor = AxisColorY;
                ZElement.ValueBox.BorderSelectedColor = AxisColorZ;
                XElement.ValueBox.HighlightColor = AxisColorX;
                XElement.ValueBox.Category = Utils.ValueCategory.Distance;
                YElement.ValueBox.HighlightColor = AxisColorY;
                YElement.ValueBox.Category = Utils.ValueCategory.Distance;
                ZElement.ValueBox.HighlightColor = AxisColorZ;
                ZElement.ValueBox.Category = Utils.ValueCategory.Distance;
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
                
                if (XElement.ValueBox.Parent is UniformGridPanel ug)
                {
                    ug.SlotPadding = new Margin(3.0f, 0.0f, 0.0f, 0.0f);
                    CheckLayout(ug);
                }

                // Override colors
                XElement.ValueBox.BorderSelectedColor = AxisColorX;
                YElement.ValueBox.BorderSelectedColor = AxisColorY;
                ZElement.ValueBox.BorderSelectedColor = AxisColorZ;
                XElement.ValueBox.HighlightColor = AxisColorX;
                XElement.ValueBox.Category = Utils.ValueCategory.Angle;
                YElement.ValueBox.HighlightColor = AxisColorY;
                YElement.ValueBox.Category = Utils.ValueCategory.Angle;
                ZElement.ValueBox.HighlightColor = AxisColorZ;
                ZElement.ValueBox.Category = Utils.ValueCategory.Angle;
            }
        }

        /// <summary>
        /// Custom editor for actor scale property.
        /// </summary>
        /// <seealso cref="FlaxEditor.CustomEditors.Editors.Float3Editor" />
        public class ScaleEditor : Float3Editor
        {
            private Button _linkButton;

            /// <inheritdoc />
            public override void Initialize(LayoutElementsContainer layout)
            {
                base.Initialize(layout);

                LinkValues = Editor.Instance.Windows.PropertiesWin.ScaleLinked;

                // Add button with the link icon

                _linkButton = new Button
                {
                    Parent = LinkedLabel,
                    Width = 18,
                    Height = 18,
                    AnchorPreset = AnchorPresets.MiddleLeft,
                };
                _linkButton.Clicked += ToggleLink;
                ToggleEnabled();
                SetLinkStyle();
                if (LinkedLabel != null)
                {
                    var textSize = FlaxEngine.GUI.Style.Current.FontMedium.MeasureText(LinkedLabel.Text.Value);
                    _linkButton.LocalX += textSize.X + 10;
                    LinkedLabel.SetupContextMenu += (label, menu, editor) =>
                    {
                        menu.AddSeparator();
                        if (LinkValues)
                            menu.AddButton("Unlink", ToggleLink).LinkTooltip("Unlinks scale components from uniform scaling");
                        else
                            menu.AddButton("Link", ToggleLink).LinkTooltip("Links scale components for uniform scaling");
                    };
                }

                if (XElement.ValueBox.Parent is UniformGridPanel ug)
                {
                    ug.SlotPadding = new Margin(3.0f, 0.0f, 0.0f, 0.0f);
                    CheckLayout(ug);
                }

                // Override colors
                var back = FlaxEngine.GUI.Style.Current.TextBoxBackground;
                XElement.ValueBox.BorderSelectedColor = AxisColorX;
                YElement.ValueBox.BorderSelectedColor = AxisColorY;
                ZElement.ValueBox.BorderSelectedColor = AxisColorZ;
                XElement.ValueBox.HighlightColor = AxisColorX;
                YElement.ValueBox.HighlightColor = AxisColorY;
                ZElement.ValueBox.HighlightColor = AxisColorZ;
            }

            /// <summary>
            /// Toggles the linking functionality.
            /// </summary>
            public void ToggleLink()
            {
                LinkValues = !LinkValues;
                Editor.Instance.Windows.PropertiesWin.ScaleLinked = LinkValues;
                ToggleEnabled();
                SetLinkStyle();
            }

            /// <summary>
            /// Toggles enables on value boxes.
            /// </summary>
            public void ToggleEnabled()
            {
                if (LinkValues)
                {
                    if (Mathf.NearEqual(((Float3)Values[0]).X, 0))
                    {
                        XElement.ValueBox.Enabled = false;
                    }
                    if (Mathf.NearEqual(((Float3)Values[0]).Y, 0))
                    {
                        YElement.ValueBox.Enabled = false;
                    }
                    if (Mathf.NearEqual(((Float3)Values[0]).Z, 0))
                    {
                        ZElement.ValueBox.Enabled = false;
                    }
                }
                else
                {
                    XElement.ValueBox.Enabled = true;
                    YElement.ValueBox.Enabled = true;
                    ZElement.ValueBox.Enabled = true;
                }
            }

            private void SetLinkStyle()
            {
                var style = FlaxEngine.GUI.Style.Current;
                var backgroundColor = LinkValues ? style.Foreground : style.ForegroundDisabled;
                _linkButton.SetColors(backgroundColor);
                _linkButton.BorderColor = _linkButton.BorderColorSelected = _linkButton.BorderColorHighlighted = Color.Transparent;
                _linkButton.TooltipText = LinkValues ? "Unlinks scale components from uniform scaling" : "Links scale components for uniform scaling";
                _linkButton.BackgroundBrush = new SpriteBrush(LinkValues ? Editor.Instance.Icons.Link32 : Editor.Instance.Icons.BrokenLink32);
            }
        }

        private static void CheckLayout(UniformGridPanel ug)
        {
            // Enlarge to fix border visibility
            ug.Height += 2;
            ug.SlotSpacing += new Float2(2);
            ug.SlotPadding += new Margin(0, 0, 1, 1);
        }
    }
}
