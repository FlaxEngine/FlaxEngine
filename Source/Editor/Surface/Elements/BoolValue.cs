// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Surface.Elements
{
    /// <summary>
    /// Bool value element.
    /// </summary>
    /// <seealso cref="FlaxEditor.Surface.SurfaceNodeElementControl" />
    [HideInEditor]
    public sealed class BoolValue : SurfaceNodeElementControl
    {
        private bool _mouseDown;

        /// <summary>
        /// Gets or sets a value.
        /// </summary>
        public bool Value
        {
            get => (bool)ParentNode.Values[Archetype.ValueIndex];
            set => ParentNode.SetValue(Archetype.ValueIndex, value);
        }

        /// <summary>
        /// Toggle sate
        /// </summary>
        public void Toggle()
        {
            Value = !Value;
        }

        /// <inheritdoc />
        public BoolValue(SurfaceNode parentNode, NodeElementArchetype archetype)
        : base(parentNode, archetype, archetype.ActualPosition, new Vector2(16), true)
        {
        }

        /// <inheritdoc />
        public override void OnSurfaceCanEditChanged(bool canEdit)
        {
            base.OnSurfaceCanEditChanged(canEdit);

            Enabled = canEdit;
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            // Cache data
            var style = Style.Current;
            var box = new Rectangle(Vector2.Zero, Size);
            bool value = Value;

            // Background
            var backgroundColor = IsMouseOver ? style.TextBoxBackgroundSelected : style.TextBoxBackground;
            Render2D.FillRectangle(box, backgroundColor);

            // Border
            Color borderColor = style.BorderNormal;
            if (!Enabled)
                borderColor *= 0.5f;
            else if (_mouseDown)
                borderColor = style.BorderSelected;
            Render2D.DrawRectangle(box, borderColor);

            // Icon
            if (value)
                Render2D.DrawSprite(style.CheckBoxTick, box, Enabled ? style.BorderSelected * 1.2f : style.ForegroundDisabled);
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Vector2 location, MouseButton button)
        {
            // Check mouse down
            if (button == MouseButton.Left)
            {
                // Set flag
                _mouseDown = true;
            }

            return base.OnMouseDown(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Vector2 location, MouseButton button)
        {
            if (_mouseDown && button == MouseButton.Left)
            {
                _mouseDown = false;
                Toggle();
            }

            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            // Clear flag
            _mouseDown = false;

            base.OnMouseLeave();
        }

        /// <summary>
        /// Gets the boolean value from the specified parent node. Handles type casting and components gather.
        /// </summary>
        /// <param name="parentNode">The parent node.</param>
        /// <param name="arch">The node element archetype.</param>
        /// <returns>The result value.</returns>
        public static bool Get(SurfaceNode parentNode, NodeElementArchetype arch)
        {
            if (arch.ValueIndex < 0)
                return false;

            bool result;
            var value = parentNode.Values[arch.ValueIndex];

            // Note: this value box may edit on component of the vector like Vector3.Y, BoxID from Archetype tells which component pick

            if (value is bool valueBool)
            {
                result = valueBool;
            }
            else if (value is int valueInt)
            {
                result = valueInt != 0;
            }
            else if (value is float valueFloat)
            {
                result = !Mathf.IsZero(valueFloat);
            }
            else if (value is Vector2 valueVec2)
            {
                result = !Mathf.IsZero(arch.BoxID == 0 ? valueVec2.X : valueVec2.Y);
            }
            else if (value is Vector3 valueVec3)
            {
                result = !Mathf.IsZero(arch.BoxID == 0 ? valueVec3.X : arch.BoxID == 1 ? valueVec3.Y : valueVec3.Z);
            }
            else if (value is Vector4 valueVec4)
            {
                result = !Mathf.IsZero(arch.BoxID == 0 ? valueVec4.X : arch.BoxID == 1 ? valueVec4.Y : arch.BoxID == 2 ? valueVec4.Z : valueVec4.W);
            }
            else
            {
                result = false;
            }

            return result;
        }

        /// <summary>
        /// Sets the boolean value of the specified parent node. Handles type casting and components assignment.
        /// </summary>
        /// <param name="parentNode">The parent node.</param>
        /// <param name="arch">The node element archetype.</param>
        /// <param name="toSet">The value to set.</param>
        public static void Set(SurfaceNode parentNode, NodeElementArchetype arch, bool toSet)
        {
            if (arch.ValueIndex < 0)
                return;

            var value = parentNode.Values[arch.ValueIndex];
            float toSetF = toSet ? 1.0f : 0.0f;

            if (value is int)
            {
                value = toSet;
            }
            else if (value is float)
            {
                value = (bool)toSet;
            }
            else if (value is Vector2 valueVec2)
            {
                if (arch.BoxID == 0)
                    valueVec2.X = toSetF;
                else
                    valueVec2.Y = toSetF;
                value = valueVec2;
            }
            else if (value is Vector3 valueVec3)
            {
                if (arch.BoxID == 0)
                    valueVec3.X = toSetF;
                else if (arch.BoxID == 1)
                    valueVec3.Y = toSetF;
                else
                    valueVec3.Z = toSetF;
                value = valueVec3;
            }
            else if (value is Vector4 valueVec4)
            {
                if (arch.BoxID == 0)
                    valueVec4.X = toSetF;
                else if (arch.BoxID == 1)
                    valueVec4.Y = toSetF;
                else if (arch.BoxID == 2)
                    valueVec4.Z = toSetF;
                else
                    valueVec4.W = toSetF;
                value = valueVec4;
            }
            else
            {
                value = 0;
            }

            parentNode.SetValue(arch.ValueIndex, value);
        }
    }
}
