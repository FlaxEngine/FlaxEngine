// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.Content.Settings;
using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.GUI;
using FlaxEditor.GUI.ContextMenu;
using FlaxEngine;
using System.Collections.Generic;
using System.Linq;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Custom editor for picking actor layer. Instead of choosing bit mask or layer index it shows a combo box with simple layer picking by name.
    /// </summary>
    public sealed class ActorLayerEditor : CustomEditor
    {
        private const string AddOrEditOption = "Add or edit Layers...";
        private ComboBoxElement _element;
        private bool _updatingItems;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            _element = layout.ComboBox();
            UpdateLayerItems((int)Values[0]);
            _element.ComboBox.PopupShowing += OnPopupShowing;
            _element.ComboBox.PopupShown += OnPopupShown;
            _element.ComboBox.SelectedIndexChanged += OnSelectedIndexChanged;
        }

        private void OnPopupShowing(ComboBox comboBox)
        {
            UpdateLayerItems(HasDifferentValues ? -1 : (int)Values[0]);
        }

        private void OnPopupShown(ComboBox comboBox)
        {
            var addOrEditOption = (ContextMenuButton)comboBox.Popup.Items.FirstOrDefault(x => x is ContextMenuButton b && b.Text == AddOrEditOption);
            if (addOrEditOption != null)
                addOrEditOption.Icon = Editor.Instance.Icons.Settings12;
        }

        private void UpdateLayerItems(int selectedIndex)
        {
            _updatingItems = true;
            var layers = LayersAndTagsSettings.GetCurrentLayers();
            _element.ComboBox.SetItems(layers);
            _element.ComboBox.AddItem(AddOrEditOption);
            _element.ComboBox.SelectedIndex = selectedIndex >= 0 && selectedIndex < layers.Length ? selectedIndex : -1;
            _updatingItems = false;
        }

        private void SelectCurrentLayer()
        {
            UpdateLayerItems(HasDifferentValues ? -1 : (int)Values[0]);
        }

        private void OnSelectedIndexChanged(ComboBox comboBox)
        {
            if (_updatingItems)
                return;

            int value = comboBox.SelectedIndex;
            if (value + 1 == comboBox.Items.Count)
            {
                Editor.Instance.ContentEditing.OpenSettings<LayersAndTagsSettings>();
                SelectCurrentLayer();
                return;
            }
            if (value == -1)
                value = 0;

            // If selected is single actor that has children, ask if apply layer to the sub objects as well
            if (Values.IsSingleObject && (int)Values[0] != value && ParentEditor.Values[0] is Actor actor && actor.HasChildren && !Editor.IsPlayMode)
            {
                var valueText = comboBox.SelectedItem;

                // Ask user
                var result = MessageBox.Show(string.Format("Do you want to change layer to \"{0}\" for all child actors as well?", valueText), "Change actor layer", MessageBoxButtons.YesNoCancel);
                if (result == DialogResult.Cancel)
                    return;
                if (result == DialogResult.Yes)
                {
                    // Note: this possibly breaks the design a little bit
                    // But it's the easiest way to set value for selected actor and its children with one undo action
                    List<Actor> actors = new List<Actor>(32);
                    Utilities.Utils.GetActorsTree(actors, actor);
                    if (Presenter.Undo != null)
                    {
                        using (new UndoMultiBlock(Presenter.Undo, actors.ToArray(), "Change layer"))
                        {
                            for (int i = 0; i < actors.Count; i++)
                            {
                                actors[i].Layer = value;
                            }
                        }
                    }
                    else
                    {
                        for (int i = 0; i < actors.Count; i++)
                        {
                            actors[i].Layer = value;
                        }
                    }

                    return;
                }
            }

            SetValue(value);
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            if (HasDifferentValues)
            {
                // TODO: support different values on many actor selected
            }
            else
            {
                UpdateLayerItems((int)Values[0]);
            }
        }
    }
}
