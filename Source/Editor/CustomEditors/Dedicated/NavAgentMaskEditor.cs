// Copyright (c) Wojciech Figat. All rights reserved.

using System.Linq;
using FlaxEditor.Content.Settings;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Dedicated
{
    /// <summary>
    /// Custom editor for <see cref="NavAgentMask"/>.
    /// </summary>
    /// <seealso cref="ActorEditor" />
    [CustomEditor(typeof(NavAgentMask)), DefaultEditor]
    internal class NavAgentMaskEditor : CustomEditor
    {
        private CheckBox[] _checkBoxes;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            var settings = GameSettings.Load<NavigationSettings>();
            if (settings.NavMeshes == null || settings.NavMeshes.Length == 0)
            {
                layout.Label("Missing navmesh settings");
                return;
            }

            _checkBoxes = new CheckBox[settings.NavMeshes.Length];
            for (int i = 0; i < settings.NavMeshes.Length; i++)
            {
                ref var navmesh = ref settings.NavMeshes[i];
                var property = layout.AddPropertyItem(navmesh.Name, navmesh.Agent.ToString());
                property.Labels.Last().TextColorHighlighted = navmesh.Color;
                var checkbox = property.Checkbox().CheckBox;
                UpdateCheckbox(checkbox, i);
                checkbox.Tag = i;
                checkbox.StateChanged += OnCheckboxStateChanged;
                _checkBoxes[i] = checkbox;
            }
        }

        /// <inheritdoc />
        protected override void Deinitialize()
        {
            _checkBoxes = null;

            base.Deinitialize();
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            if (_checkBoxes != null)
            {
                for (int i = 0; i < _checkBoxes.Length; i++)
                {
                    UpdateCheckbox(_checkBoxes[i], i);
                }
            }

            base.Refresh();
        }

        private void OnCheckboxStateChanged(CheckBox checkBox)
        {
            var i = (int)checkBox.Tag;
            var value = (NavAgentMask)Values[0];
            var mask = 1u << i;
            value.Mask &= ~mask;
            value.Mask |= checkBox.Checked ? mask : 0;
            SetValue(value);
        }

        private void UpdateCheckbox(CheckBox checkbox, int i)
        {
            for (var j = 0; j < Values.Count; j++)
            {
                var value = (((NavAgentMask)Values[j]).Mask & (1 << i)) != 0;
                if (j == 0)
                {
                    checkbox.Checked = value;
                }
                else if (checkbox.State != CheckBoxState.Intermediate)
                {
                    if (checkbox.Checked != value)
                        checkbox.State = CheckBoxState.Intermediate;
                }
            }
        }
    }
}
