// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using FlaxEditor.GUI.Input;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Surface.Elements
{
    /// <summary>
    /// Unsigned Integer value editing element.
    /// </summary>
    /// <seealso cref="UIntValueBox" />
    /// <seealso cref="ISurfaceNodeElement" />
    [HideInEditor]
    public sealed class UnsignedIntegerValue : UIntValueBox, ISurfaceNodeElement
    {
        /// <inheritdoc />
        public SurfaceNode ParentNode { get; }

        /// <inheritdoc />
        public NodeElementArchetype Archetype { get; }

        /// <inheritdoc />
        public UnsignedIntegerValue(SurfaceNode parentNode, NodeElementArchetype archetype)
        : base(Get(parentNode, archetype), archetype.Position.X, archetype.Position.Y, 50, (uint)archetype.ValueMin, (uint)archetype.ValueMax, 0.05f)
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
        /// Gets the unsigned integer value from the specified parent node. Handles type casting and components gather.
        /// </summary>
        /// <param name="parentNode">The parent node.</param>
        /// <param name="arch">The node element archetype.</param>
        /// <returns>The result value.</returns>
        public static uint Get(SurfaceNode parentNode, NodeElementArchetype arch)
        {
            if (arch.ValueIndex < 0)
                return 0;

            uint result;
            var value = parentNode.Values[arch.ValueIndex];

            // Note: this value box may edit on component of the vector like Vector3.Y, BoxID from Archetype tells which component pick

            if (value is int valueInt)
                result = (uint)valueInt;
            else if (value is uint valueUint)
                result = valueUint;
            else if (value is long valueLong)
                result = (uint)valueLong;
            else if (value is ulong valueUlong)
                result = (uint)valueUlong;
            else if (value is float valueFloat)
                result = (uint)valueFloat;
            else if (value is Vector2 valueVec2)
                result = (uint)(arch.BoxID == 0 ? valueVec2.X : valueVec2.Y);
            else if (value is Vector3 valueVec3)
                result = (uint)(arch.BoxID == 0 ? valueVec3.X : arch.BoxID == 1 ? valueVec3.Y : valueVec3.Z);
            else if (value is Vector4 valueVec4)
                result = (uint)(arch.BoxID == 0 ? valueVec4.X : arch.BoxID == 1 ? valueVec4.Y : arch.BoxID == 2 ? valueVec4.Z : valueVec4.W);
            else
                result = 0u;

            return result;
        }

        /// <summary>
        /// Sets the unsigned integer value of the specified parent node. Handles type casting and components assignment.
        /// </summary>
        /// <param name="parentNode">The parent node.</param>
        /// <param name="arch">The node element archetype.</param>
        /// <param name="toSet">The value to set.</param>
        public static void Set(SurfaceNode parentNode, NodeElementArchetype arch, uint toSet)
        {
            if (arch.ValueIndex < 0)
                return;

            var value = parentNode.Values[arch.ValueIndex];
            float toSetF = (float)toSet;

            if (value is int)
                value = (int)toSet;
            else if (value is uint)
                value = toSet;
            else if (value is long)
                value = (long)toSet;
            else if (value is ulong)
                value = (ulong)toSet;
            else if (value is float)
                value = toSetF;
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
                value = 0;

            parentNode.SetValue(arch.ValueIndex, value);
        }
    }
}
