// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.GUI;
using FlaxEditor.Scripting;
using FlaxEngine.Utilities;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Base implementation of the inspector used to edit properties of the given abstract or interface type that contain a setter to assign a derived implementation type.
    /// </summary>
    public abstract class ObjectSwitcherEditor : CustomEditor
    {
        /// <summary>
        /// Defines type that can be assigned to the modified property.
        /// </summary>
        public struct OptionType
        {
            /// <summary>
            /// The type name used to show in the type selector dropdown menu (for user interface).
            /// </summary>
            public string Name;

            /// <summary>
            /// The type.
            /// </summary>
            public Type Type;

            /// <summary>
            /// The creator function that spawns the object of the given type.
            /// </summary>
            public Func<Type, object> Creator;

            /// <summary>
            /// Initializes a new instance of the <see cref="OptionType"/> struct.
            /// </summary>
            /// <param name="type">The type.</param>
            public OptionType(Type type)
            {
                Name = type.Name;
                Type = type;
                Creator = Activator.CreateInstance;
            }

            /// <summary>
            /// Initializes a new instance of the <see cref="OptionType"/> struct.
            /// </summary>
            /// <param name="name">The name.</param>
            /// <param name="type">The type.</param>
            public OptionType(string name, Type type)
            {
                Name = name;
                Type = type;
                Creator = Activator.CreateInstance;
            }

            /// <summary>
            /// Initializes a new instance of the <see cref="OptionType"/> struct.
            /// </summary>
            /// <param name="name">The name.</param>
            /// <param name="type">The type.</param>
            /// <param name="creator">The instance creation function.</param>
            public OptionType(string name, Type type, Func<Type, object> creator)
            {
                Name = name;
                Type = type;
                Creator = creator;
            }
        }

        /// <summary>
        /// Gets the options collection for the property value assignment.
        /// </summary>
        protected abstract OptionType[] Options { get; }

        /// <summary>
        /// Gets the name of the type ComboBox property name for the object type picking.
        /// </summary>
        protected virtual string TypeComboBoxName => "Type";

        private OptionType[] _options;
        private ScriptType _type;

        private ScriptType Type => Values[0] == null ? Values.Type : TypeUtils.GetObjectType(Values[0]);

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            // Get the target options
            _options = Options;
            if (_options == null)
                throw new ArgumentNullException();

            int selectedIndex = -1;
            bool hasDifferentTypes = Values.HasDifferentTypes;
            var type = Type;
            _type = type;

            // Type
            var typeEditor = layout.ComboBox(TypeComboBoxName, "Type of the object value. Use it to change the object.");
            for (int i = 0; i < _options.Length; i++)
            {
                typeEditor.ComboBox.AddItem(_options[i].Name);
                selectedIndex = _options[i].Type == type.Type ? i : selectedIndex;
            }
            typeEditor.ComboBox.SupportMultiSelect = false;
            typeEditor.ComboBox.SelectedIndex = hasDifferentTypes ? -1 : selectedIndex;
            typeEditor.ComboBox.SelectedIndexChanged += OnSelectedIndexChanged;

            // Editing different types is not supported
            if (Values.HasDifferentTypes)
            {
                var property = layout.AddPropertyItem("Value");
                property.Label("Different Values");
                return;
            }

            // Nothing to edit
            if (Values.HasNull)
            {
                var property = layout.AddPropertyItem("Value");
                property.Label("<null>");
                return;
            }

            // Value
            var values = new CustomValueContainer(type, (instance, index) => instance);
            values.AddRange(Values);
            var editor = CustomEditorsUtil.CreateEditor(type);
            var style = editor.Style;
            if (style == DisplayStyle.InlineIntoParent)
            {
                layout.Object(values, editor);
            }
            else if (style == DisplayStyle.Group)
            {
                var group = layout.Group("Value", true);
                group.Panel.Open(false);
                group.Object(values, editor);

                // Remove empty group
                if (group.ContainerControl.ChildrenCount == 0)
                {
                    layout.Children.Remove(group);
                    group.Panel.Dispose();
                }
            }
            else
            {
                layout.AddPropertyItem("Value").Object(values, editor);
            }
        }

        private void OnSelectedIndexChanged(ComboBox comboBox)
        {
            object value = null;
            if (comboBox.SelectedIndex != -1)
            {
                var option = _options[comboBox.SelectedIndex];
                if (option.Type != null)
                    value = option.Creator(option.Type);
            }
            SetValue(value);
            RebuildLayoutOnRefresh();
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            // Check if type has been modified outside the editor (eg. from code)
            if (Type != _type)
            {
                if (ParentEditor != null)
                    ParentEditor.RebuildLayout();
                else
                    RebuildLayout();
            }
        }
    }
}
