// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using FlaxEditor.Content.Settings;
using FlaxEditor.GUI;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.Tree;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Dedicated
{
    /// <summary>
    /// Custom editor for Light Layer Culling Masks
    /// </summary>
    public sealed class LightLayersMaskEditor : CustomEditor
    {
        private ClickableLabel _label;
        private ContextMenuBase _picker;
        private uint[] _pendingMasks;

        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            _label = layout.ClickableLabel(GetSummary()).CustomControl;
            _label.RightClick += ShowPicker;

            var button = new Button
            {
                Size = new Float2(16.0f),
                Text = "...",
                TooltipText = "Edit light layers...",
                Parent = _label,
            };
            button.SetAnchorPreset(AnchorPresets.MiddleRight, false, true);
            button.X -= 1;
            button.Clicked += ShowPicker;
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();
            _label.Text = GetSummary();
            _pendingMasks = null;
        }

        /// <inheritdoc />
        protected override void Deinitialize()
        {
            _picker?.Dispose();
            _picker = null;
            _label = null;
            _pendingMasks = null;
            base.Deinitialize();
        }
        
        private string GetSummary()
        {
            if (HasDifferentValues)
                return "Mixed";

            uint mask = (uint)Values[0];
            if (mask == 0)
                return "None";
            if (mask == uint.MaxValue)
                return "All layers";

            var names = LayersAndTagsSettings.GetCurrentLightLayers();
            var selected = new List<string>();
            for (int slot = 0; slot < names.Length; slot++)
            {
                if ((mask & (1u << slot)) != 0)
                    selected.Add(names[slot]);
            }

            return selected.Count <= 2
                   ? string.Join(", ", selected)
                   : $"{selected.Count} layers";
        }
        
        private void SetLayerBits(uint bits, bool enabled)
        {
            var updated = new uint[Values.Count];
            for (int i = 0; i < updated.Length; i++)
            {
                uint current = _pendingMasks != null ? _pendingMasks[i] : (uint)Values[i];
                updated[i] = enabled ? current | bits : current & ~bits;
            }

            _pendingMasks = updated;
            if (updated.Length == 1)
                SetValue(updated[0]);
            else
                SetValue(updated);
        }
        
        private void ShowPicker()
        {
            _picker?.Dispose();
            var menu = Utilities.Utils.CreateSearchPopup(
                out _, out var tree, headerHeight: 26.0f, autoSearch: true);
            _picker = menu;

            var names = LayersAndTagsSettings.GetCurrentLightLayers();
            var checkBoxes = new List<CheckBox>(names.Length);
            bool updatingCheckBoxes = false;

            for (int slot = 0; slot < names.Length; slot++)
            {
                uint bit = 1u << slot;
                var node = new TreeNodeWithAddons()
                {
                    Text = $"{slot}: {names[slot]}",
                    TextMargin = new Margin(22.0f, 2.0f, 2.0f, 2.0f),
                    Parent = tree,
                };
                var checkBox = new CheckBox(16.0f, 0, false)
                {
                    Height = 16.0f,
                    IsScrollable = false,
                    Parent = node,
                };
                node.Addons.Add(checkBox);
                checkBoxes.Add(checkBox);

                bool enabled = ((uint)Values[0] & bit) != 0;
                checkBox.Checked = enabled;
                for (int i = 1; i < Values.Count; i++)
                {
                    bool otherEnabled = ((uint)Values[i] & bit) != 0;
                    if (otherEnabled == enabled)
                        continue;

                    checkBox.State = CheckBoxState.Intermediate;
                    break;
                }

                checkBox.StateChanged += box =>
                {
                    if (!updatingCheckBoxes)
                        SetLayerBits(bit, box.Checked);
                };
            }
            
            void SetAllLayers(bool enabled)
            {
                updatingCheckBoxes = true;
                try
                {
                    foreach (var checkBox in checkBoxes)
                        checkBox.Checked = enabled;
                }
                finally
                {
                    updatingCheckBoxes = false;
                }

                SetLayerBits(uint.MaxValue, enabled);
            }

            var selectAll = new Button(2, 2, 90, 22)
            {
                Text = "Select All",
                Parent = menu,
            };
            selectAll.Clicked += () => SetAllLayers(true);

            var deselectAll = new Button(94, 2, 100, 22)
            {
                Text = "Deselect All",
                Parent = menu,
            };
            deselectAll.Clicked += () => SetAllLayers(false);

            menu.Show(_label, new Float2(0, _label.Height));
        }        
    }
}
