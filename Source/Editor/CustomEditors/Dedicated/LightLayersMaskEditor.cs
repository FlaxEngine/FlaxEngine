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
        }

        /// <inheritdoc />
        protected override void Deinitialize()
        {
            _picker?.Dispose();
            _picker = null;
            _label = null;
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
        
        private void ShowPicker()
        {
            _picker?.Dispose();
            var menu = Utilities.Utils.CreateSearchPopup(
                out _, out var tree, headerHeight: 26.0f, autoSearch: true);
            _picker = menu;

            var names = LayersAndTagsSettings.GetCurrentLightLayers();
            uint changedBits = 0;
            uint enabledBits = 0;

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
                    changedBits |= bit;
                    if (box.Checked)
                        enabledBits |= bit;
                    else
                        enabledBits &= ~bit;
                };
            }

            var apply = new Button(menu.Width - 66, 2, 64, 22)
            {
                Text = "Apply",
                Parent = menu,
            };
            apply.Clicked += () =>
            {
                if (changedBits != 0)
                {
                    if (Values.Count == 1)
                    {
                        uint current = (uint)Values[0];
                        SetValue((current & ~changedBits) | enabledBits);
                    }
                    else
                    {
                        var updated = new uint[Values.Count];
                        for (int i = 0; i < updated.Length; i++)
                        {
                            uint current = (uint)Values[i];
                            updated[i] = (current & ~changedBits) | enabledBits;
                        }

                        SetValue(updated);
                    }
                }

                menu.Hide();
            };

            menu.Show(_label, new Float2(0, _label.Height));
        }        
    }
}
