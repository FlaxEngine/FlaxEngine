// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

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
        public static Color AxisColorX = Color.ParseHex("CB2F44FF");

        /// <summary>
        /// The Y axis color.
        /// </summary>
        public static Color AxisColorY = Color.ParseHex("81CB1CFF");

        /// <summary>
        /// The Z axis color.
        /// </summary>
        public static Color AxisColorZ = Color.ParseHex("2872CBFF");
        
        /// <summary>
        /// The X axis color for background.
        /// </summary>
        public static Color AxisColorXBackground = Color.Lerp(AxisColorX, Style.Current.TextBoxBackground, DarkenFactor);
        
        /// <summary>
        /// The Y axis color for background.
        /// </summary>
        public static Color AxisColorYBackground = Color.Lerp(AxisColorY, Style.Current.TextBoxBackground, DarkenFactor);
        
        /// <summary>
        /// The Z axis color for background.
        /// </summary>
        public static Color AxisColorZBackground = Color.Lerp(AxisColorZ, Style.Current.TextBoxBackground, DarkenFactor);
        
        /// <summary>
        /// The darken factor for axis colors.
        /// </summary>
        public const float DarkenFactor = 0.3f;
        
        /// <summary>
        /// The darken factor for axis colors.
        /// </summary>
        public const float TextDarkenFactor = 0.9f;
        
        /// <summary>
        /// The X axis color for the text label.
        /// </summary>
        public static Color AxisTextColorX = Color.Lerp(AxisColorX, Color.White, TextDarkenFactor);
        
        /// <summary>
        /// The Y axis color for the text label.
        /// </summary>
        public static Color AxisTextColorY = Color.Lerp(AxisColorY, Color.White, TextDarkenFactor);
        
        /// <summary>
        /// The Z axis color for the text label.
        /// </summary>
        public static Color AxisTextColorZ = Color.Lerp(AxisColorZ, Color.White, TextDarkenFactor);

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
                XLabel.Control.BackgroundColor = AxisColorXBackground;
                XLabel.Label.TextColor = AxisTextColorX;
                YLabel.Control.BackgroundColor = AxisColorYBackground;
                YLabel.Label.TextColor = AxisTextColorY;
                ZLabel.Control.BackgroundColor = AxisColorZBackground;
                ZLabel.Label.TextColor = AxisTextColorZ;
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
                XLabel.Control.BackgroundColor = AxisColorXBackground;
                XLabel.Label.TextColor = AxisTextColorX;
                YLabel.Control.BackgroundColor = AxisColorYBackground;
                YLabel.Label.TextColor = AxisTextColorY;
                ZLabel.Control.BackgroundColor = AxisColorZBackground;
                ZLabel.Label.TextColor = AxisTextColorZ;
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
                    BackgroundBrush = new SpriteBrush(Editor.Instance.Icons.Link32),
                    Parent = LinkedLabel,
                    Width = 18,
                    Height = 18,
                    AnchorPreset = AnchorPresets.MiddleLeft,
                };
                _linkButton.Clicked += ToggleLink;
                ToggleEnabled();
                SetLinkStyle();
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

                // Override colors
                XLabel.Control.BackgroundColor = AxisColorXBackground;
                XLabel.Label.TextColor = AxisTextColorX;
                YLabel.Control.BackgroundColor = AxisColorYBackground;
                YLabel.Label.TextColor = AxisTextColorY;
                ZLabel.Control.BackgroundColor = AxisColorZBackground;
                ZLabel.Label.TextColor = AxisTextColorZ;
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
            }
        }
    }
}
