// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.IO;
using System.Linq;
using FlaxEditor.CustomEditors.Editors;
using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.GUI;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Dedicated
{
    /// <summary>
    /// Custom editor for <see cref="ModelInstanceActor.MeshReference"/>.
    /// </summary>
    /// <seealso cref="ActorEditor" />
    [CustomEditor(typeof(ModelInstanceActor.MeshReference)), DefaultEditor]
    public class MeshReferenceEditor : CustomEditor
    {
        private class MeshRefPickerControl : Control
        {
            private ModelInstanceActor.MeshReference _value = new ModelInstanceActor.MeshReference { LODIndex = -1, MeshIndex = -1 };
            private string _valueName;
            private Float2 _mousePos;

            public string[][] MeshNames;
            public event Action ValueChanged;

            public ModelInstanceActor.MeshReference Value
            {
                get => _value;
                set
                {
                    if (_value.LODIndex == value.LODIndex && _value.MeshIndex == value.MeshIndex)
                        return;
                    _value = value;
                    if (value.LODIndex == -1 || value.MeshIndex == -1)
                        _valueName = null;
                    else if (MeshNames.Length == 1)
                        _valueName = MeshNames[value.LODIndex][value.MeshIndex];
                    else
                        _valueName = $"LOD{value.LODIndex} - {MeshNames[value.LODIndex][value.MeshIndex]}";
                    ValueChanged?.Invoke();
                }
            }

            public MeshRefPickerControl()
            : base(0, 0, 50, 16)
            {
            }

            private void ShowDropDownMenu()
            {
                // Show context menu with tree structure of LODs and meshes
                Focus();
                var cm = new ItemsListContextMenu(200);
                var meshNames = MeshNames;
                var actor = _value.Actor;
                for (int lodIndex = 0; lodIndex < meshNames.Length; lodIndex++)
                {
                    var item = new ItemsListContextMenu.Item
                    {
                        Name = "LOD" + lodIndex,
                        Tag = new ModelInstanceActor.MeshReference { Actor = actor, LODIndex = lodIndex, MeshIndex = 0 },
                        TintColor = new Color(0.8f, 0.8f, 1.0f, 0.8f),
                    };
                    cm.AddItem(item);

                    for (int meshIndex = 0; meshIndex < meshNames[lodIndex].Length; meshIndex++)
                    {
                        item = new ItemsListContextMenu.Item
                        {
                            Name = "     " + meshNames[lodIndex][meshIndex],
                            Tag = new ModelInstanceActor.MeshReference { Actor = actor, LODIndex = lodIndex, MeshIndex = meshIndex },
                        };
                        if (_value.LODIndex == lodIndex && _value.MeshIndex == meshIndex)
                            item.BackgroundColor = FlaxEngine.GUI.Style.Current.BackgroundSelected;
                        cm.AddItem(item);
                    }
                }
                cm.ItemClicked += item => Value = (ModelInstanceActor.MeshReference)item.Tag;
                cm.Show(Parent, BottomLeft);
            }

            /// <inheritdoc />
            public override void Draw()
            {
                base.Draw();

                // Cache data
                var style = FlaxEngine.GUI.Style.Current;
                bool isSelected = _valueName != null;
                bool isEnabled = EnabledInHierarchy;
                var frameRect = new Rectangle(0, 0, Width, 16);
                if (isSelected)
                    frameRect.Width -= 16;
                frameRect.Width -= 16;
                var nameRect = new Rectangle(2, 1, frameRect.Width - 4, 14);
                var button1Rect = new Rectangle(nameRect.Right + 2, 1, 14, 14);
                var button2Rect = new Rectangle(button1Rect.Right + 2, 1, 14, 14);

                // Draw frame
                Render2D.DrawRectangle(frameRect, isEnabled && (IsMouseOver || IsNavFocused) ? style.BorderHighlighted : style.BorderNormal);

                // Check if has item selected
                if (isSelected)
                {
                    // Draw name
                    Render2D.PushClip(nameRect);
                    Render2D.DrawText(style.FontMedium, _valueName, nameRect, isEnabled ? style.Foreground : style.ForegroundDisabled, TextAlignment.Near, TextAlignment.Center);
                    Render2D.PopClip();

                    // Draw deselect button
                    Render2D.DrawSprite(style.Cross, button1Rect, isEnabled && button1Rect.Contains(_mousePos) ? style.Foreground : style.ForegroundGrey);
                }
                else
                {
                    // Draw info
                    Render2D.DrawText(style.FontMedium, "-", nameRect, isEnabled ? Color.OrangeRed : Color.DarkOrange, TextAlignment.Near, TextAlignment.Center);
                }

                // Draw picker button
                var pickerRect = isSelected ? button2Rect : button1Rect;
                Render2D.DrawSprite(style.ArrowDown, pickerRect, isEnabled && pickerRect.Contains(_mousePos) ? style.Foreground : style.ForegroundGrey);
            }

            /// <inheritdoc />
            public override void OnMouseEnter(Float2 location)
            {
                _mousePos = location;

                base.OnMouseEnter(location);
            }

            /// <inheritdoc />
            public override void OnMouseLeave()
            {
                _mousePos = Float2.Minimum;

                base.OnMouseLeave();
            }

            /// <inheritdoc />
            public override void OnMouseMove(Float2 location)
            {
                _mousePos = location;

                base.OnMouseMove(location);
            }

            /// <inheritdoc />
            public override bool OnMouseUp(Float2 location, MouseButton button)
            {
                // Cache data
                bool isSelected = _valueName != null;
                var frameRect = new Rectangle(0, 0, Width, 16);
                if (isSelected)
                    frameRect.Width -= 16;
                frameRect.Width -= 16;
                var nameRect = new Rectangle(2, 1, frameRect.Width - 4, 14);
                var button1Rect = new Rectangle(nameRect.Right + 2, 1, 14, 14);
                var button2Rect = new Rectangle(button1Rect.Right + 2, 1, 14, 14);

                // Deselect
                if (isSelected && button1Rect.Contains(ref location))
                    Value = new ModelInstanceActor.MeshReference { Actor = null, LODIndex = -1, MeshIndex = -1 };

                // Picker dropdown menu
                if ((isSelected ? button2Rect : button1Rect).Contains(ref location))
                    ShowDropDownMenu();

                return base.OnMouseUp(location, button);
            }

            /// <inheritdoc />
            public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
            {
                Focus();

                // Open model editor window
                if (_value.Actor is StaticModel staticModel)
                    Editor.Instance.ContentEditing.Open(staticModel.Model);
                else if (_value.Actor is AnimatedModel animatedModel)
                    Editor.Instance.ContentEditing.Open(animatedModel.SkinnedModel);

                return base.OnMouseDoubleClick(location, button);
            }

            /// <inheritdoc />
            public override void OnSubmit()
            {
                base.OnSubmit();

                ShowDropDownMenu();
            }

            /// <inheritdoc />
            public override void OnDestroy()
            {
                MeshNames = null;
                _valueName = null;

                base.OnDestroy();
            }
        }

        private ModelInstanceActor _actor;
        private CustomElement<FlaxObjectRefPickerControl> _actorPicker;
        private CustomElement<MeshRefPickerControl> _meshPicker;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            // Get the context actor to pick the mesh from it
            if (GetActor(out var actor))
            {
                // TODO: support editing multiple values
                layout.Label("Different values");
                return;
            }
            _actor = actor;

            if (ParentEditor.Values.Any(x => x is Cloth))
            {
                // Cloth always picks the parent model mesh
                if (actor == null)
                {
                    layout.Label("Cloth needs to be added as a child to model actor.");
                }
            }
            else
            {
                // Actor reference picker
                _actorPicker = layout.Custom<FlaxObjectRefPickerControl>();
                _actorPicker.CustomControl.Type = new ScriptType(typeof(ModelInstanceActor));
                _actorPicker.CustomControl.ValueChanged += () => SetValue(new ModelInstanceActor.MeshReference { Actor = (ModelInstanceActor)_actorPicker.CustomControl.Value });
            }

            if (actor != null)
            {
                // Get mesh names hierarchy
                string[][] meshNames;
                if (actor is StaticModel staticModel)
                {
                    var model = staticModel.Model;
                    if (model == null || model.WaitForLoaded())
                    {
                        layout.Label("No model.");
                        return;
                    }
                    var materials = model.MaterialSlots;
                    var lods = model.LODs;
                    meshNames = new string[lods.Length][];
                    for (int lodIndex = 0; lodIndex < lods.Length; lodIndex++)
                    {
                        var lodMeshes = lods[lodIndex].Meshes;
                        meshNames[lodIndex] = new string[lodMeshes.Length];
                        for (int meshIndex = 0; meshIndex < lodMeshes.Length; meshIndex++)
                        {
                            var mesh = lodMeshes[meshIndex];
                            var materialName = materials[mesh.MaterialSlotIndex].Name;
                            if (string.IsNullOrEmpty(materialName) && materials[mesh.MaterialSlotIndex].Material)
                                materialName = Path.GetFileNameWithoutExtension(materials[mesh.MaterialSlotIndex].Material.Path);
                            if (string.IsNullOrEmpty(materialName))
                                meshNames[lodIndex][meshIndex] = $"Mesh {meshIndex}";
                            else
                                meshNames[lodIndex][meshIndex] = $"Mesh {meshIndex} ({materialName})";
                        }
                    }
                }
                else if (actor is AnimatedModel animatedModel)
                {
                    var skinnedModel = animatedModel.SkinnedModel;
                    if (skinnedModel == null || skinnedModel.WaitForLoaded())
                    {
                        layout.Label("No model.");
                        return;
                    }
                    var materials = skinnedModel.MaterialSlots;
                    var lods = skinnedModel.LODs;
                    meshNames = new string[lods.Length][];
                    for (int lodIndex = 0; lodIndex < lods.Length; lodIndex++)
                    {
                        var lodMeshes = lods[lodIndex].Meshes;
                        meshNames[lodIndex] = new string[lodMeshes.Length];
                        for (int meshIndex = 0; meshIndex < lodMeshes.Length; meshIndex++)
                        {
                            var mesh = lodMeshes[meshIndex];
                            var materialName = materials[mesh.MaterialSlotIndex].Name;
                            if (string.IsNullOrEmpty(materialName) && materials[mesh.MaterialSlotIndex].Material)
                                materialName = Path.GetFileNameWithoutExtension(materials[mesh.MaterialSlotIndex].Material.Path);
                            if (string.IsNullOrEmpty(materialName))
                                meshNames[lodIndex][meshIndex] = $"Mesh {meshIndex}";
                            else
                                meshNames[lodIndex][meshIndex] = $"Mesh {meshIndex} ({materialName})";
                        }
                    }
                }
                else
                    return; // Not supported model type

                // Mesh reference picker
                _meshPicker = layout.Custom<MeshRefPickerControl>();
                _meshPicker.CustomControl.MeshNames = meshNames;
                _meshPicker.CustomControl.Value = (ModelInstanceActor.MeshReference)Values[0];
                _meshPicker.CustomControl.ValueChanged += () => SetValue(_meshPicker.CustomControl.Value);
            }
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            if (_actorPicker != null)
            {
                GetActor(out var actor);
                _actorPicker.CustomControl.Value = actor;
                if (actor != _actor)
                {
                    RebuildLayout();
                    return;
                }
            }
            if (_meshPicker != null)
            {
                _meshPicker.CustomControl.Value = (ModelInstanceActor.MeshReference)Values[0];
            }
        }

        private bool GetActor(out ModelInstanceActor actor)
        {
            actor = null;
            foreach (ModelInstanceActor.MeshReference value in Values)
            {
                if (actor == null)
                    actor = value.Actor;
                else if (actor != value.Actor)
                    return true;
            }
            return false;
        }
    }
}
