// Copyright (c) Wojciech Figat. All rights reserved.

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
        : base(parentNode, archetype, archetype.ActualPosition, new Float2(16), true)
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
            var box = new Rectangle(Float2.Zero, Size);
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
        public override bool OnMouseDown(Float2 location, MouseButton button)
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
        public override bool OnMouseUp(Float2 location, MouseButton button)
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
        /// <param name="customValue">The custom value override (optional).</param>
        /// <returns>The result value.</returns>
        public static bool Get(SurfaceNode parentNode, NodeElementArchetype arch, object customValue = null)
        {
            if (arch.ValueIndex < 0 && customValue == null)
                return false;

            bool result;
            var value = customValue ?? parentNode.Values[arch.ValueIndex];

            // Note: this value box may edit on component of the vector like Vector3.Y, BoxID from Archetype tells which component pick

            if (value is bool asBool)
            {
                result = asBool;
            }
            else if (value is int asInt)
            {
                result = asInt != 0;
            }
            else if (value is float asFloat)
            {
                result = !Mathf.IsZero(asFloat);
            }
            else if (value is Vector2 asVector2)
            {
                result = !Mathf.IsZero(arch.BoxID == 0 ? asVector2.X : asVector2.Y);
            }
            else if (value is Vector3 asVector3)
            {
                result = !Mathf.IsZero(arch.BoxID == 0 ? asVector3.X : arch.BoxID == 1 ? asVector3.Y : asVector3.Z);
            }
            else if (value is Vector4 asVector4)
            {
                result = !Mathf.IsZero(arch.BoxID == 0 ? asVector4.X : arch.BoxID == 1 ? asVector4.Y : arch.BoxID == 2 ? asVector4.Z : asVector4.W);
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
            else if (value is Vector2 asVector2)
            {
                if (arch.BoxID == 0)
                    asVector2.X = toSetF;
                else
                    asVector2.Y = toSetF;
                value = asVector2;
            }
            else if (value is Vector3 asVector3)
            {
                if (arch.BoxID == 0)
                    asVector3.X = toSetF;
                else if (arch.BoxID == 1)
                    asVector3.Y = toSetF;
                else
                    asVector3.Z = toSetF;
                value = asVector3;
            }
            else if (value is Vector4 asVector4)
            {
                if (arch.BoxID == 0)
                    asVector4.X = toSetF;
                else if (arch.BoxID == 1)
                    asVector4.Y = toSetF;
                else if (arch.BoxID == 2)
                    asVector4.Z = toSetF;
                else
                    asVector4.W = toSetF;
                value = asVector4;
            }
            else
            {
                value = 0;
            }

            parentNode.SetValue(arch.ValueIndex, value);
        }
    }
}
