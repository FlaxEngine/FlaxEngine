// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Linq;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI
{
    /// <summary>
    /// Popup that shows the list of types to pick. Supports searching and basic type filtering.
    /// </summary>
    /// <seealso cref="FlaxEditor.GUI.ItemsListContextMenu" />
    public class TypeSearchPopup : ItemsListContextMenu
    {
        /// <summary>
        /// The type item.
        /// </summary>
        /// <seealso cref="FlaxEditor.GUI.ItemsListContextMenu.Item" />
        public class TypeItemView : Item
        {
            private ScriptType _type;

            /// <summary>
            /// Gets the type.
            /// </summary>
            public ScriptType Type => _type;

            /// <summary>
            /// Initializes a new instance of the <see cref="TypeItemView"/> class.
            /// </summary>
            public TypeItemView()
            {
                _type = ScriptType.Null;
                Name = "<null>";
                TooltipText = "Unset value.";
                Tag = _type;
            }

            /// <summary>
            /// Initializes a new instance of the <see cref="TypeItemView"/> class.
            /// </summary>
            /// <param name="type">The type.</param>
            public TypeItemView(ScriptType type)
            : this(type, type.GetAttributes(false))
            {
            }

            /// <summary>
            /// Initializes a new instance of the <see cref="TypeItemView"/> class.
            /// </summary>
            /// <param name="type">The type.</param>
            /// <param name="attributes">The type attributes.</param>
            public TypeItemView(ScriptType type, object[] attributes)
            {
                _type = type;

                Name = type.Name;
                TooltipText = Editor.Instance.CodeDocs.GetTooltip(type, attributes);
                Tag = type;
                var categoryAttribute = (CategoryAttribute)attributes.FirstOrDefault(x => x is CategoryAttribute);
                if (categoryAttribute != null)
                {
                    Category = categoryAttribute.Name;
                }
            }

            /// <inheritdoc />
            public override void OnDestroy()
            {
                _type = ScriptType.Null;

                base.OnDestroy();
            }
        }

        /// <summary>
        /// Validates if the given actor item can be used to pick it.
        /// </summary>
        /// <param name="type">The type.</param>
        /// <returns>True if is valid.</returns>
        public delegate bool IsValidDelegate(ScriptType type);

        private IsValidDelegate _isValid;
        private Action<ScriptType> _selected;

        private TypeSearchPopup(IsValidDelegate isValid, Action<ScriptType> selected)
        {
            _isValid = isValid;
            _selected = selected;

            ItemClicked += OnItemClicked;

            // TODO: use async thread to search types without UI stall
            var allTypes = Editor.Instance.CodeEditing.All.Get();
            AddItem(new TypeItemView());
            for (int i = 0; i < allTypes.Count; i++)
            {
                var type = allTypes[i];
                if (_isValid(type))
                {
                    var attributes = type.GetAttributes(true);
                    if (attributes.FirstOrDefault(x => x is HideInEditorAttribute || x is System.Runtime.CompilerServices.CompilerGeneratedAttribute) == null)
                    {
                        var mType = type.Type;
                        if (mType != null && mType.IsValueType && mType.ReflectedType != null && string.Equals(mType.ReflectedType.Name, "<PrivateImplementationDetails>", StringComparison.Ordinal))
                            continue;
                        AddItem(new TypeItemView(type, attributes));
                    }
                }
            }
            SortItems();
        }

        private void OnItemClicked(Item item)
        {
            _selected(((TypeItemView)item).Type);
        }

        /// <summary>
        /// Shows the popup.
        /// </summary>
        /// <param name="showTarget">The show target.</param>
        /// <param name="showTargetLocation">The show target location.</param>
        /// <param name="isValid">Event called to check if a given asset item is valid to be used.</param>
        /// <param name="selected">Event called on asset item pick.</param>
        /// <returns>The dialog.</returns>
        public static TypeSearchPopup Show(Control showTarget, Float2 showTargetLocation, IsValidDelegate isValid, Action<ScriptType> selected)
        {
            var popup = new TypeSearchPopup(isValid, selected);
            popup.Show(showTarget, showTargetLocation);
            return popup;
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _isValid = null;
            _selected = null;

            base.OnDestroy();
        }
    }
}
