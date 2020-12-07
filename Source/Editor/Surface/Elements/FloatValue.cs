// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using FlaxEditor.GUI.Input;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Surface.Elements
{
    /// <summary>
    /// Floating point value editing element.
    /// </summary>
    /// <seealso cref="FloatValueBox" />
    /// <seealso cref="ISurfaceNodeElement" />
    [HideInEditor]
    public sealed class FloatValue : FloatValueBox, ISurfaceNodeElement
    {
        /// <inheritdoc />
        public SurfaceNode ParentNode { get; }

        /// <inheritdoc />
        public NodeElementArchetype Archetype { get; }

        /// <summary>
        /// Gets the surface.
        /// </summary>
        public VisjectSurface Surface => ParentNode.Surface;

        /// <inheritdoc />
        public FloatValue(SurfaceNode parentNode, NodeElementArchetype archetype)
        : base(Get(parentNode, archetype), archetype.Position.X, archetype.Position.Y, 50, archetype.ValueMin, archetype.ValueMax, 0.01f)
        {
            ParentNode = parentNode;
            Archetype = archetype;

            ParentNode.ValuesChanged += OnNodeValuesChanged;
        }

        private void OnNodeValuesChanged()
        {
            Value = Get(ParentNode, Archetype);
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            // Draw border
            if (!IsFocused)
                Render2D.DrawRectangle(new Rectangle(Vector2.Zero, Size), Style.Current.BorderNormal);
        }

        /// <inheritdoc />
        protected override void OnValueChanged()
        {
            base.OnValueChanged();
            Set(ParentNode, Archetype, Value);
        }

        /// <summary>
        /// Gets the floating point value from the specified parent node. Handles type casting and components gather.
        /// </summary>
        /// <param name="parentNode">The parent node.</param>
        /// <param name="arch">The node element archetype.</param>
        /// <returns>The result value.</returns>
        public static float Get(SurfaceNode parentNode, NodeElementArchetype arch)
        {
            if (arch.ValueIndex < 0)
                return 0;

            float result;
            var value = parentNode.Values[arch.ValueIndex];

            // Note: this value box may edit on component of the vector like Vector3.Y, BoxID from Archetype tells which component pick

            if (value is int valueInt)
            {
                result = (float)valueInt;
            }
            else if (value is float valueFloat)
            {
                result = valueFloat;
            }
            else if (value is double valueDouble)
            {
                result = (float)valueDouble;
            }
            else if (value is Vector2 valueVec2)
            {
                result = (arch.BoxID == 0 ? valueVec2.X : valueVec2.Y);
            }
            else if (value is Vector3 valueVec3)
            {
                result = (arch.BoxID == 0 ? valueVec3.X : arch.BoxID == 1 ? valueVec3.Y : valueVec3.Z);
            }
            else if (value is Vector4 valueVec4)
            {
                result = (arch.BoxID == 0 ? valueVec4.X : arch.BoxID == 1 ? valueVec4.Y : arch.BoxID == 2 ? valueVec4.Z : valueVec4.W);
            }
            else
            {
                result = 0;
            }

            return result;
        }

        /// <summary>
        /// Sets the floating point value of the specified parent node. Handles type casting and components assignment.
        /// </summary>
        /// <param name="parentNode">The parent node.</param>
        /// <param name="arch">The node element archetype.</param>
        /// <param name="toSet">The value to set.</param>
        public static void Set(SurfaceNode parentNode, NodeElementArchetype arch, float toSet)
        {
            if (arch.ValueIndex < 0)
                return;

            var value = parentNode.Values[arch.ValueIndex];

            if (value is int)
            {
                value = (int)toSet;
            }
            else if (value is float)
            {
                value = toSet;
            }
            else if (value is double)
            {
                value = (double)toSet;
            }
            else if (value is Vector2 valueVec2)
            {
                if (arch.BoxID == 0)
                    valueVec2.X = toSet;
                else
                    valueVec2.Y = toSet;
                value = valueVec2;
            }
            else if (value is Vector3 valueVec3)
            {
                if (arch.BoxID == 0)
                    valueVec3.X = toSet;
                else if (arch.BoxID == 1)
                    valueVec3.Y = toSet;
                else
                    valueVec3.Z = toSet;
                value = valueVec3;
            }
            else if (value is Vector4 valueVec4)
            {
                if (arch.BoxID == 0)
                    valueVec4.X = toSet;
                else if (arch.BoxID == 1)
                    valueVec4.Y = toSet;
                else if (arch.BoxID == 2)
                    valueVec4.Z = toSet;
                else
                    valueVec4.W = toSet;
                value = valueVec4;
            }
            else
            {
                value = 0;
            }

            parentNode.SetValue(arch.ValueIndex, value);
        }

        /// <summary>
        /// Sets all the values to the given value (eg. all components of the vector).
        /// </summary>
        /// <param name="parentNode">The parent node.</param>
        /// <param name="arch">The node element archetype.</param>
        /// <param name="toSet">The value to assign.</param>
        public static void SetAllValues(SurfaceNode parentNode, NodeElementArchetype arch, float toSet)
        {
            if (arch.ValueIndex < 0)
                return;

            var value = parentNode.Values[arch.ValueIndex];

            if (value is int)
            {
                value = (int)toSet;
            }
            else if (value is float)
            {
                value = toSet;
            }
            else if (value is double)
            {
                value = (double)toSet;
            }
            else if (value is Vector2)
            {
                value = new Vector2(toSet);
            }
            else if (value is Vector3)
            {
                value = new Vector3(toSet);
            }
            else if (value is Vector4)
            {
                value = new Vector4(toSet);
            }
            else
            {
                value = 0;
            }

            parentNode.SetValue(arch.ValueIndex, value);
        }
    }
}
