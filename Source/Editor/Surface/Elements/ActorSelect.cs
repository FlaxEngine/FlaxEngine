// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.CustomEditors.Editors;
using FlaxEngine;
using FlaxEngine.Utilities;

namespace FlaxEditor.Surface.Elements
{
    /// <summary>
    /// Actors picking control.
    /// </summary>
    /// <seealso cref="FlaxObjectRefPickerControl" />
    /// <seealso cref="ISurfaceNodeElement" />
    [HideInEditor]
    public class ActorSelect : FlaxObjectRefPickerControl, ISurfaceNodeElement
    {
        /// <inheritdoc />
        public SurfaceNode ParentNode { get; }

        /// <inheritdoc />
        public NodeElementArchetype Archetype { get; }

        /// <summary>
        /// Initializes a new instance of the <see cref="ActorSelect"/> class.
        /// </summary>
        /// <param name="parentNode">The parent node.</param>
        /// <param name="archetype">The archetype.</param>
        public ActorSelect(SurfaceNode parentNode, NodeElementArchetype archetype)
        {
            ParentNode = parentNode;
            Archetype = archetype;
            Bounds = new Rectangle(Archetype.ActualPosition, archetype.Size);
            Type = TypeUtils.GetType(archetype.Text);

            ParentNode.ValuesChanged += OnNodeValuesChanged;
            OnNodeValuesChanged();
        }

        private void OnNodeValuesChanged()
        {
            ValueID = (Guid)ParentNode.Values[Archetype.ValueIndex];
        }

        /// <inheritdoc />
        protected override void OnValueChanged()
        {
            var selectedId = ValueID;
            if (ParentNode != null && (Guid)ParentNode.Values[Archetype.ValueIndex] != selectedId)
            {
                ParentNode.SetValue(Archetype.ValueIndex, selectedId);
            }

            base.OnValueChanged();
        }
    }
}
