// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.GUI;
using FlaxEngine;
using FlaxEngine.Utilities;

namespace FlaxEditor.Surface.Elements
{
    /// <summary>
    /// Assets picking control.
    /// </summary>
    /// <seealso cref="AssetPicker" />
    /// <seealso cref="ISurfaceNodeElement" />
    [HideInEditor]
    public class AssetSelect : AssetPicker, ISurfaceNodeElement
    {
        /// <inheritdoc />
        public SurfaceNode ParentNode { get; }

        /// <inheritdoc />
        public NodeElementArchetype Archetype { get; }

        /// <summary>
        /// Initializes a new instance of the <see cref="AssetSelect"/> class.
        /// </summary>
        /// <param name="parentNode">The parent node.</param>
        /// <param name="archetype">The archetype.</param>
        public AssetSelect(SurfaceNode parentNode, NodeElementArchetype archetype)
        : base(TypeUtils.GetType(archetype.Text), archetype.ActualPosition)
        {
            ParentNode = parentNode;
            Archetype = archetype;

            ParentNode.ValuesChanged += OnNodeValuesChanged;
            OnNodeValuesChanged();
        }

        private void OnNodeValuesChanged()
        {
            Validator.SelectedID = (Guid)ParentNode.Values[Archetype.ValueIndex];
        }

        /// <inheritdoc />
        protected override void OnSelectedItemChanged()
        {
            var selectedId = Validator.SelectedID;
            if (ParentNode != null && (Guid)ParentNode.Values[Archetype.ValueIndex] != selectedId)
            {
                ParentNode.SetValue(Archetype.ValueIndex, selectedId);
            }

            base.OnSelectedItemChanged();
        }
    }
}
