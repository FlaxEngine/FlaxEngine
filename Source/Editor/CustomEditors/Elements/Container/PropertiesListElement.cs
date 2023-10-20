// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.CustomEditors.GUI;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Elements
{
    /// <summary>
    /// <see cref="CustomEditor"/> properties list element.
    /// </summary>
    /// <seealso cref="LayoutElementsContainer"/>
    public class PropertiesListElement : LayoutElementsContainer
    {
        /// <summary>
        /// The list.
        /// </summary>
        public readonly PropertiesList Properties;

        /// <inheritdoc />
        public override ContainerControl ContainerControl => Properties;

        /// <summary>
        /// Initializes a new instance of the <see cref="PropertiesListElement"/> class.
        /// </summary>
        public PropertiesListElement()
        {
            Properties = new PropertiesList(this);
        }

        /// <inheritdoc />
        public override CustomEditor Property(string name, ValueContainer values, CustomEditor overrideEditor)
        {
            var editor = CustomEditorsUtil.CreateEditor(values, overrideEditor);
            var style = editor.Style;
            if (style == DisplayStyle.Group)
            {
                var group = Group(name, editor, true);
                group.Panel.Open();
                return group.Object(values:values, overrideEditor:editor);
            }
            return Object(values:values, overrideEditor:editor);
        }

        /// <inheritdoc />
        public override CustomEditor Property(string name, ValueContainer values)
        {
            var element = Property(name:name, values:values, overrideEditor: null);
            return element;
        }

        internal readonly List<PropertyNameLabel> Labels = new List<PropertyNameLabel>();

        internal void OnAddProperty(string name, string tooltip)
        {
            var label = new PropertyNameLabel(name)
            {
                Parent = Properties,
                TooltipText = tooltip,
                FirstChildControlIndex = Properties.Children.Count
            };
            Labels.Add(label);
        }

        internal void OnAddProperty(PropertyNameLabel label, string tooltip)
        {
            if (label == null)
                throw new ArgumentNullException();
            label.Parent = Properties;
            label.TooltipText = tooltip;
            label.FirstChildControlIndex = Properties.Children.Count;
            Labels.Add(label);
        }

        /// <inheritdoc />
        protected override void OnAddEditor(CustomEditor editor)
        {
            // Link to the last label
            if (Labels.Count > 0)
            {
                Labels[Labels.Count - 1].LinkEditor(editor);
            }

            base.OnAddEditor(editor);
        }

        /// <inheritdoc />
        public override void ClearLayout()
        {
            base.ClearLayout();
            Labels.Clear();
        }
    }
}
