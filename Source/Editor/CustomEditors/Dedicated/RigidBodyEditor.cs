// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.Reflection.Emit;
using FlaxEditor.CustomEditors.GUI;
using FlaxEngine;
using FlaxEngine.GUI;

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
        private Label _infoLabel;

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
            if (_infoLabel != null)
            {
                string text = string.Empty;
                if (Editor.IsPlayMode)
                {
                    var linearVelocity = value.LinearVelocity;
                    var centerOfMass = value.CenterOfMass;
                    text = $"Speed: {linearVelocity.Length}\n" +
                           $"Linear Velocity: {linearVelocity}\n" +
                           $"Angular Velocity: {value.AngularVelocity}\n" +
                           $"Center of Mass (local): {centerOfMass}\n" +
                           $"Center Of Mass (world): {value.Transform.LocalToWorld(centerOfMass)}\n" +
                           $"Is Sleeping: {value.IsSleeping}";
                }
                _infoLabel.Text = text;
            }
        }

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            _labels.Clear();

            base.Initialize(layout);

            // Add info box
            if (IsSingleObject && Values[0] is RigidBody && Editor.IsPlayMode)
            {
                var group = layout.Group("Info");
                _infoLabel = group.Label(string.Empty).Label;
                _infoLabel.AutoHeight = true;
                _infoLabel.Margin = new Margin(3);
            }
        }
    }
}
