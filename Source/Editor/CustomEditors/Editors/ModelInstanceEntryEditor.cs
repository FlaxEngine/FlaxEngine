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
        private ModelInstanceEntry _entry;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            _updateName = true;
            var group = layout.Group("Entry");
            _group = group;
            
            _entry = (ModelInstanceEntry)Values[0];
            var entryIndex = ParentEditor.ChildrenEditors.IndexOf(this);
            var materiaLabel = new PropertyNameLabel("Material");
            materiaLabel.TooltipText = "The mesh surface material used for the rendering.";
            if (ParentEditor.ParentEditor.Values[0] is StaticModel staticModel)
            {
                // Ensure that entry with default material set is set back to null
                if (_entry.Material == staticModel.Model.MaterialSlots[entryIndex].Material)
                {
                    staticModel.SetMaterial(entryIndex, null);
                }
                _material = staticModel.GetMaterial(entryIndex);

                var matContainer = new CustomValueContainer(new ScriptType(typeof(MaterialBase)), _material, (instance, index) => _material, (instance, index, value) => _material = value as MaterialBase);
                var materialEditor = (_group.Property(materiaLabel, matContainer)) as AssetRefEditor;
                materialEditor.Values.SetDefaultValue(staticModel.Model.MaterialSlots[entryIndex].Material);
                materialEditor.RefreshDefaultValue();
                materialEditor.Picker.SelectedItemChanged += () =>
                {
                    var material = materialEditor.Picker.SelectedAsset as MaterialBase;
                    if (!material)
                    {
                        staticModel.SetMaterial(entryIndex, GPUDevice.Instance.DefaultMaterial);
                        materialEditor.Picker.SelectedAsset = GPUDevice.Instance.DefaultMaterial;
                    }
                    else if (material == staticModel.Model.MaterialSlots[entryIndex].Material)
                    {
                        staticModel.SetMaterial(entryIndex, null);
                    }
                    else
                    {
                        staticModel.SetMaterial(entryIndex, material);
                    }
                };
            }
            else if (ParentEditor.ParentEditor.Values[0] is AnimatedModel animatedModel)
            {
                // Ensure that entry with default material set is set back to null
                if (_entry.Material == animatedModel.SkinnedModel.MaterialSlots[entryIndex].Material)
                {
                    animatedModel.SetMaterial(entryIndex, null);
                }
                _material = animatedModel.GetMaterial(entryIndex);
                
                var matContainer = new CustomValueContainer(new ScriptType(typeof(MaterialBase)), _material, (instance, index) => _material, (instance, index, value) =>
                {
                    _material = value as MaterialBase;
                });
                var materialEditor = (_group.Property(materiaLabel, matContainer)) as AssetRefEditor;
                materialEditor.Values.SetDefaultValue(animatedModel.SkinnedModel.MaterialSlots[entryIndex].Material);
                materialEditor.RefreshDefaultValue();
                materialEditor.Picker.SelectedItemChanged += () =>
                {
                    var material = materialEditor.Picker.SelectedAsset as MaterialBase;
                    if (!material)
                    {
                        animatedModel.SetMaterial(entryIndex, GPUDevice.Instance.DefaultMaterial);
                        materialEditor.Picker.SelectedAsset = GPUDevice.Instance.DefaultMaterial;
                    }
                    else if (material == animatedModel.SkinnedModel.MaterialSlots[entryIndex].Material)
                    {
                        animatedModel.SetMaterial(entryIndex, null);
                    }
                    else
                    {
                        animatedModel.SetMaterial(entryIndex, material);
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
            {
                return;
            }
            base.SpawnProperty(itemLayout, itemValues, item);
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            Debug.Log("Hit");
            if (_updateName &&
                _group != null &&
                ParentEditor?.ParentEditor != null &&
                ParentEditor.ParentEditor.Values.Count > 0)
            {
                var entryIndex = ParentEditor.ChildrenEditors.IndexOf(this);
                if (ParentEditor.ParentEditor.Values[0] is StaticModel staticModel)
                {
                    var model = staticModel.Model;
                    if (model && model.IsLoaded)
                    {
                        var slots = model.MaterialSlots;
                        if (slots != null && slots.Length > entryIndex)
                        {
                            _group.Panel.HeaderText = "Entry " + slots[entryIndex].Name;
                            _updateName = false;
                        }
                    }
                }
                else if (ParentEditor.ParentEditor.Values[0] is AnimatedModel animatedModel)
                {
                    var model = animatedModel.SkinnedModel;
                    if (model && model.IsLoaded)
                    {
                        var slots = model.MaterialSlots;
                        if (slots != null && slots.Length > entryIndex)
                        {
                            _group.Panel.HeaderText = "Entry " + slots[entryIndex].Name;
                            _updateName = false;
                        }
                    }
                }
            }

            base.Refresh();
        }
    }
}
