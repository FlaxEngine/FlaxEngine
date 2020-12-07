// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using FlaxEditor.CustomEditors.Elements;
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

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            _updateName = true;
            var group = layout.Group("Entry");
            _group = group;

            base.Initialize(group);
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
                if (ParentEditor.ParentEditor.Values[0] is StaticModel staticModel)
                {
                    var model = staticModel.Model;
                    if (model && model.IsLoaded)
                    {
                        _group.Panel.HeaderText = "Entry " + model.MaterialSlots[entryIndex].Name;
                        _updateName = false;
                    }
                }
                else if (ParentEditor.ParentEditor.Values[0] is AnimatedModel animatedModel)
                {
                    var model = animatedModel.SkinnedModel;
                    if (model && model.IsLoaded)
                    {
                        _group.Panel.HeaderText = "Entry " + model.MaterialSlots[entryIndex].Name;
                        _updateName = false;
                    }
                }
            }

            base.Refresh();
        }
    }
}
