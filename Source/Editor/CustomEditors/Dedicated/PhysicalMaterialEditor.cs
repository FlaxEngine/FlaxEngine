// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using FlaxEditor.CustomEditors.Editors;
using FlaxEditor.CustomEditors.GUI;
using FlaxEngine;

namespace FlaxEditor.CustomEditors.Dedicated
{
    /// <summary>
    /// Custom editor for <see cref="PhysicalMaterial"/>.
    /// </summary>
    /// <seealso cref="GenericEditor" />
    [CustomEditor(typeof(PhysicalMaterial)), DefaultEditor]
    public class PhysicalMaterialEditor : GenericEditor
    {
        private readonly List<CheckablePropertyNameLabel> _labels = new List<CheckablePropertyNameLabel>(64);
        private const int FrictionCombineModeOrder = 1;
        private const int RestitutionCombineModeOrder = 4;

        /// <inheritdoc />
        protected override void SpawnProperty(LayoutElementsContainer itemLayout, ValueContainer itemValues, ItemInfo item)
        {
            var order = item.Order.Order;

            if (order != FrictionCombineModeOrder && order != RestitutionCombineModeOrder)
            {
                base.SpawnProperty(itemLayout, itemValues, item);
                return;
            }

            // Add labels with a check box
            var label = new CheckablePropertyNameLabel(item.DisplayName);
            label.CheckBox.Tag = order;
            label.CheckChanged += CheckBoxOnCheckChanged;
            _labels.Add(label);
            itemLayout.Property(label, itemValues, item.OverrideEditor, item.TooltipText);
        }

        private void CheckBoxOnCheckChanged(CheckablePropertyNameLabel label)
        {
            if (IsSetBlocked)
                return;

            var order = (int)label.CheckBox.Tag;
            var value = (PhysicalMaterial)Values[0];
            if (order == FrictionCombineModeOrder)
                value.OverrideFrictionCombineMode = label.CheckBox.Checked;
            else if (order == RestitutionCombineModeOrder)
                value.OverrideRestitutionCombineMode = label.CheckBox.Checked;
            SetValue(value);
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            // Update all labels
            var value = (PhysicalMaterial)Values[0];
            for (int i = 0; i < _labels.Count; i++)
            {
                var order = (int)_labels[i].CheckBox.Tag;
                bool check = false;
                if (order == FrictionCombineModeOrder)
                    check = value.OverrideFrictionCombineMode;
                else if (order == RestitutionCombineModeOrder)
                    check = value.OverrideRestitutionCombineMode;
                _labels[i].CheckBox.Checked = check;
            }
        }

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            _labels.Clear();

            base.Initialize(layout);
        }
    }
}
