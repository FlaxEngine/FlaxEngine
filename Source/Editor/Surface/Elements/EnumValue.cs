// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.GUI;
using FlaxEngine.Utilities;

namespace FlaxEditor.Surface.Elements
{
    /// <summary>
    /// Combo box for enum element.
    /// </summary>
    /// <seealso cref="EnumComboBox" />
    /// <seealso cref="ISurfaceNodeElement" />
    /// [HideInEditor]
    public class EnumValue : EnumComboBox, ISurfaceNodeElement
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
        public EnumValue(SurfaceNode parentNode, NodeElementArchetype archetype)
        : base(TypeUtils.GetType(archetype.Text).Type)
        {
            X = archetype.ActualPositionX;
            Y = archetype.ActualPositionY;
            Width = archetype.Size.X;
            ParentNode = parentNode;
            Archetype = archetype;
            Value = Convert.ToInt32(ParentNode.Values[Archetype.ValueIndex]);
        }

        /// <inheritdoc />
        protected override void OnValueChanged()
        {
            if (Convert.ToInt32(ParentNode.Values[Archetype.ValueIndex]) != (int)Value)
            {
                // Edit value
                ParentNode.SetValue(Archetype.ValueIndex, Value);
            }

            base.OnValueChanged();
        }
    }
}
