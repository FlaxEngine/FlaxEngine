// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using FlaxEditor.CustomEditors.GUI;
using FlaxEngine;

namespace FlaxEditor.CustomEditors.Dedicated
{
    /// <summary>
    /// Custom editor for <see cref="RigidBody"/>.
    /// </summary>
    /// <seealso cref="ActorEditor" />
    [CustomEditor(typeof(RigidBody)), DefaultEditor]
    public class RigidBodyEditor : ActorEditor
    {
        private readonly List<CheckablePropertyNameLabel> _labels = new List<CheckablePropertyNameLabel>(64);
        private const int MassOrder = 110;

        /// <inheritdoc />
        protected override void SpawnProperty(LayoutElementsContainer itemLayout, ValueContainer itemValues, ItemInfo item)
        {
            var order = item.Order?.Order ?? 0;

            if (order != MassOrder)
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
            label.UpdateStyle();
        }

        private void CheckBoxOnCheckChanged(CheckablePropertyNameLabel label)
        {
            if (IsSetBlocked)
                return;

            var order = (int)label.CheckBox.Tag;
            var value = (RigidBody)Values[0];
            if (order == MassOrder)
                value.OverrideMass = label.CheckBox.Checked;
            SetValue(value);
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            // Update all labels
            var value = (RigidBody)Values[0];
            for (int i = 0; i < _labels.Count; i++)
            {
                var order = (int)_labels[i].CheckBox.Tag;
                bool check = false;
                if (order == MassOrder)
                    check = value.OverrideMass;
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
