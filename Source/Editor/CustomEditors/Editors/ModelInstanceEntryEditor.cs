// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.CustomEditors.GUI;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Default implementation of the inspector used to edit <see cref="ModelInstanceEntry"/> value type properties.
    /// </summary>
    [CustomEditor(typeof(ModelInstanceEntry)), DefaultEditor]
    public sealed class ModelInstanceEntryEditor : GenericEditor
    {
        private DropPanel _mainPanel;
        private bool _updateName;
        private int _entryIndex;
        private bool _isRefreshing;
        private MaterialBase _material;
        private ModelInstanceActor _modelInstance;
        private AssetRefEditor _materialEditor;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            _updateName = true;
            if (layout.ContainerControl.Parent is DropPanel panel)
            {
                _mainPanel = panel;
                _mainPanel.HeaderText = "Entry";
            }

            if (ParentEditor == null || HasDifferentTypes)
                return;
            var entry = (ModelInstanceEntry)Values[0];
            var entryIndex = ParentEditor.ChildrenEditors.IndexOf(this);
            var materialLabel = new PropertyNameLabel("Material");
            materialLabel.TooltipText = "The mesh surface material used for the rendering.";
            var parentEditorValues = ParentEditor.ParentEditor?.Values;
            if (parentEditorValues?[0] is ModelInstanceActor modelInstance)
            {
                // TODO: store _modelInstance and _material in array for each selected model instance actor
                _entryIndex = entryIndex;
                _modelInstance = modelInstance;
                var slots = modelInstance.MaterialSlots;
                if (slots == null || entryIndex >= slots.Length)
                    return;
                if (entry.Material == slots[entryIndex].Material)
                {
                    // Ensure that entry with default material set is set back to null
                    modelInstance.SetMaterial(entryIndex, null);
                }
                _material = modelInstance.GetMaterial(entryIndex);
                var defaultValue = GPUDevice.Instance.DefaultMaterial;
                if (slots[entryIndex].Material)
                {
                    // Use default value set on asset (eg. Model Asset)
                    defaultValue = slots[entryIndex].Material;
                }

                // Create material picker
                var materialValue = new CustomValueContainer(new ScriptType(typeof(MaterialBase)), _material, (instance, index) => _material, (instance, index, value) => _material = value as MaterialBase);
                for (var i = 1; i < parentEditorValues.Count; i++)
                    materialValue.Add(_material);
                var materialEditor = (AssetRefEditor)layout.Property(materialLabel, materialValue);
                materialEditor.Values.SetDefaultValue(defaultValue);
                materialEditor.RefreshDefaultValue();
                materialEditor.Picker.SelectedItemChanged += OnSelectedMaterialChanged;
                _materialEditor = materialEditor;
            }

            base.Initialize(layout);
        }

        private void OnSelectedMaterialChanged()
        {
            if (_isRefreshing || _modelInstance == null)
                return;
            _isRefreshing = true;
            var slots = _modelInstance.MaterialSlots;
            var material = _materialEditor.Picker.Validator.SelectedAsset as MaterialBase;
            var defaultMaterial = GPUDevice.Instance.DefaultMaterial;
            var value = (ModelInstanceEntry)Values[0];
            var prevMaterial = value.Material;
            if (!material)
            {
                // Fallback to default material
                _materialEditor.Picker.Validator.SelectedAsset = defaultMaterial;
                value.Material = defaultMaterial;
            }
            else if (material == slots[_entryIndex].Material)
            {
                // Asset default material
                value.Material = null;
            }
            else if (material == defaultMaterial && !slots[_entryIndex].Material)
            {
                // Default material while asset has no set as well
                value.Material = null;
            }
            else
            {
                // Custom material
                value.Material = material;
            }
            if (prevMaterial != value.Material)
                SetValue(value);
            _isRefreshing = false;
        }

        /// <inheritdoc />
        protected override void SpawnProperty(LayoutElementsContainer itemLayout, ValueContainer itemValues, ItemInfo item)
        {
            // Skip material member as it is overridden
            if (item.Info.Name == "Material" && _materialEditor != null)
                return;
            base.SpawnProperty(itemLayout, itemValues, item);
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            // Update panel title to match material slot name
            if (_updateName &&
                _mainPanel != null &&
                ParentEditor?.ParentEditor != null &&
                ParentEditor.ParentEditor.Values.Count > 0)
            {
                var entryIndex = ParentEditor.ChildrenEditors.IndexOf(this);
                if (ParentEditor.ParentEditor.Values[0] is ModelInstanceActor modelInstance)
                {
                    var slots = modelInstance.MaterialSlots;
                    if (slots != null && slots.Length > entryIndex)
                    {
                        _updateName = false;
                        _mainPanel.HeaderText = "Entry " + slots[entryIndex].Name;
                    }
                }
            }

            // Refresh currently selected material
            _material = _modelInstance.GetMaterial(_entryIndex);

            base.Refresh();
        }

        /// <inheritdoc />
        protected override void Deinitialize()
        {
            _material = null;
            _modelInstance = null;
            _materialEditor = null;

            base.Deinitialize();
        }
    }
}
