// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.CustomEditors.GUI;
using FlaxEditor.Scripting;
using FlaxEngine;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Default implementation of the inspector used to edit <see cref="ModelInstanceEntry"/> value type properties.
    /// </summary>
    [CustomEditor(typeof(ModelInstanceEntry)), DefaultEditor]
    public sealed class ModelInstanceEntryEditor : GenericEditor
    {
        private GroupElement _group;
        private bool _updateName;
        private MaterialBase _material;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            _updateName = true;
            var group = layout.Group("Entry");
            _group = group;

            if (ParentEditor == null)
                return;
            var entry = (ModelInstanceEntry)Values[0];
            var entryIndex = ParentEditor.ChildrenEditors.IndexOf(this);
            var materialLabel = new PropertyNameLabel("Material");
            materialLabel.TooltipText = "The mesh surface material used for the rendering.";
            if (ParentEditor.ParentEditor?.Values[0] is ModelInstanceActor modelInstance)
            {
                // Ensure that entry with default material set is set back to null
                var defaultValue = GPUDevice.Instance.DefaultMaterial;
                {
                    var slots = modelInstance.MaterialSlots;
                    if (entry.Material == slots[entryIndex].Material)
                    {
                        modelInstance.SetMaterial(entryIndex, null);
                    }
                    _material = modelInstance.GetMaterial(entryIndex);
                    if (slots[entryIndex].Material)
                    {
                        defaultValue = slots[entryIndex].Material;
                    }
                }

                var matContainer = new CustomValueContainer(new ScriptType(typeof(MaterialBase)), _material, (instance, index) => _material, (instance, index, value) => _material = value as MaterialBase);
                var materialEditor = (AssetRefEditor)_group.Property(materialLabel, matContainer);
                materialEditor.Values.SetDefaultValue(defaultValue);
                materialEditor.RefreshDefaultValue();
                materialEditor.Picker.SelectedItemChanged += () =>
                {
                    var slots = modelInstance.MaterialSlots;
                    var material = materialEditor.Picker.SelectedAsset as MaterialBase;
                    var defaultMaterial = GPUDevice.Instance.DefaultMaterial;
                    if (!material)
                    {
                        modelInstance.SetMaterial(entryIndex, defaultMaterial);
                        materialEditor.Picker.SelectedAsset = defaultMaterial;
                    }
                    else if (material == slots[entryIndex].Material)
                    {
                        modelInstance.SetMaterial(entryIndex, null);
                    }
                    else if (material == defaultMaterial && !slots[entryIndex].Material)
                    {
                        modelInstance.SetMaterial(entryIndex, null);
                    }
                    else
                    {
                        modelInstance.SetMaterial(entryIndex, material);
                    }
                };
            }

            base.Initialize(group);
        }

        /// <inheritdoc />
        protected override void SpawnProperty(LayoutElementsContainer itemLayout, ValueContainer itemValues, ItemInfo item)
        {
            // Skip material member as it is overridden
            if (item.Info.Name == "Material")
                return;
            base.SpawnProperty(itemLayout, itemValues, item);
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            if (_updateName &&
                _group != null &&
                ParentEditor?.ParentEditor != null &&
                ParentEditor.ParentEditor.Values.Count > 0)
            {
                var entryIndex = ParentEditor.ChildrenEditors.IndexOf(this);
                if (ParentEditor.ParentEditor.Values[0] is ModelInstanceActor modelInstance)
                {
                    var slots = modelInstance.MaterialSlots;
                    if (slots != null && slots.Length > entryIndex)
                    {
                        _group.Panel.HeaderText = "Entry " + slots[entryIndex].Name;
                        _updateName = false;
                    }
                }
            }

            base.Refresh();
        }
    }
}
