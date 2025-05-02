// Copyright (c) Wojciech Figat. All rights reserved.

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

            // Disable slider if surface doesn't allow it
            if (ParentNode.Surface != null && !ParentNode.Surface.CanLivePreviewValueChanges)
                _slideSpeed = 0.0f;
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
                Render2D.DrawRectangle(new Rectangle(Float2.Zero, Size), Style.Current.BorderNormal);
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
        /// <param name="customValue">The custom value override (optional).</param>
        /// <returns>The result value.</returns>
        public static float Get(SurfaceNode parentNode, NodeElementArchetype arch, object customValue = null)
        {
            if (arch.ValueIndex < 0 && customValue == null)
                return 0;

            float result;
            var value = customValue ?? parentNode.Values[arch.ValueIndex];
            var component = arch.BoxID; // Reuse existing field

            // Note: this value box may edit on component of the vector like Vector3.Y, BoxID from Archetype tells which component pick

            if (value is int asInt)
            {
                result = (float)asInt;
            }
            else if (value is float asFloat)
            {
                result = asFloat;
            }
            else if (value is double asDouble)
            {
                result = (float)asDouble;
            }
            else if (value is Vector2 asVector2)
            {
                result = (float)(component == 0 ? asVector2.X : asVector2.Y);
            }
            else if (value is Vector3 asVector3)
            {
                result = (float)(component == 0 ? asVector3.X : component == 1 ? asVector3.Y : asVector3.Z);
            }
            else if (value is Vector4 asVector4)
            {
                result = (float)(component == 0 ? asVector4.X : component == 1 ? asVector4.Y : component == 2 ? asVector4.Z : asVector4.W);
            }
            else if (value is Float2 asFloat2)
            {
                result = (component == 0 ? asFloat2.X : asFloat2.Y);
            }
            else if (value is Float3 asFloat3)
            {
                result = (component == 0 ? asFloat3.X : component == 1 ? asFloat3.Y : asFloat3.Z);
            }
            else if (value is Float4 asFloat4)
            {
                result = (component == 0 ? asFloat4.X : component == 1 ? asFloat4.Y : component == 2 ? asFloat4.Z : asFloat4.W);
            }
            else if (value is Double2 asDouble2)
            {
                result = (float)(component == 0 ? asDouble2.X : asDouble2.Y);
            }
            else if (value is Double3 asDouble3)
            {
                result = (float)(component == 0 ? asDouble3.X : component == 1 ? asDouble3.Y : asDouble3.Z);
            }
            else if (value is Double4 asDouble4)
            {
                result = (float)(component == 0 ? asDouble4.X : component == 1 ? asDouble4.Y : component == 2 ? asDouble4.Z : asDouble4.W);
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
            var component = arch.BoxID; // Reuse existing field

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
            else if (component == -1)
            {
                // Per-component editing is used only by nodes from Constant group, otherwise use float
                value = toSet;
            }
            else if (value is Vector2 asVector2)
            {
                if (component == 0)
                    asVector2.X = toSet;
                else
                    asVector2.Y = toSet;
                value = asVector2;
            }
            else if (value is Vector3 asVector3)
            {
                if (component == 0)
                    asVector3.X = toSet;
                else if (component == 1)
                    asVector3.Y = toSet;
                else
                    asVector3.Z = toSet;
                value = asVector3;
            }
            else if (value is Vector4 asVector4)
            {
                if (component == 0)
                    asVector4.X = toSet;
                else if (component == 1)
                    asVector4.Y = toSet;
                else if (component == 2)
                    asVector4.Z = toSet;
                else
                    asVector4.W = toSet;
                value = asVector4;
            }
            else if (value is Float2 asFloat2)
            {
                if (component == 0)
                    asFloat2.X = toSet;
                else
                    asFloat2.Y = toSet;
                value = asFloat2;
            }
            else if (value is Float3 asFloat3)
            {
                if (component == 0)
                    asFloat3.X = toSet;
                else if (component == 1)
                    asFloat3.Y = toSet;
                else
                    asFloat3.Z = toSet;
                value = asFloat3;
            }
            else if (value is Float4 asFloat4)
            {
                if (component == 0)
                    asFloat4.X = toSet;
                else if (component == 1)
                    asFloat4.Y = toSet;
                else if (component == 2)
                    asFloat4.Z = toSet;
                else
                    asFloat4.W = toSet;
                value = asFloat4;
            }
            else if (value is Double2 asDouble2)
            {
                if (component == 0)
                    asDouble2.X = toSet;
                else
                    asDouble2.Y = toSet;
                value = asDouble2;
            }
            else if (value is Double3 asDouble3)
            {
                if (component == 0)
                    asDouble3.X = toSet;
                else if (component == 1)
                    asDouble3.Y = toSet;
                else
                    asDouble3.Z = toSet;
                value = asDouble3;
            }
            else if (value is Double4 asDouble4)
            {
                if (component == 0)
                    asDouble4.X = toSet;
                else if (component == 1)
                    asDouble4.Y = toSet;
                else if (component == 2)
                    asDouble4.Z = toSet;
                else
                    asDouble4.W = toSet;
                value = asDouble4;
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
            else if (value is Float2)
            {
                value = new Float2(toSet);
            }
            else if (value is Float3)
            {
                value = new Float3(toSet);
            }
            else if (value is Float4)
            {
                value = new Float4(toSet);
            }
            else if (value is Double2)
            {
                value = new Double2(toSet);
            }
            else if (value is Double3)
            {
                value = new Double3(toSet);
            }
            else if (value is Double4)
            {
                value = new Double4(toSet);
            }
            else
            {
                value = 0;
            }

            parentNode.SetValue(arch.ValueIndex, value);
        }
    }
}
