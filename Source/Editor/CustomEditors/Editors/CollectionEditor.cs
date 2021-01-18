// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections;
using System.Linq;
using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.CustomEditors.GUI;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Default implementation of the inspector used to edit arrays/list.
    /// </summary>
    [HideInEditor]
    public abstract class CollectionEditor : CustomEditor
    {
        /// <summary>
        /// The custom implementation of the collection items labels that can be used to reorder items.
        /// </summary>
        /// <seealso cref="FlaxEditor.CustomEditors.GUI.PropertyNameLabel" />
        private class CollectionItemLabel : PropertyNameLabel
        {
            /// <summary>
            /// The editor.
            /// </summary>
            public CollectionEditor Editor;

            /// <summary>
            /// The index of the item (zero-based).
            /// </summary>
            public readonly int Index;

            /// <summary>
            /// Initializes a new instance of the <see cref="CollectionItemLabel"/> class.
            /// </summary>
            /// <param name="editor">The editor.</param>
            /// <param name="index">The index.</param>
            public CollectionItemLabel(CollectionEditor editor, int index)
            : base("Element " + index)
            {
                Editor = editor;
                Index = index;

                SetupContextMenu += OnSetupContextMenu;
            }

            private void OnSetupContextMenu(PropertyNameLabel label, ContextMenu menu, CustomEditor linkedEditor)
            {
                menu.AddSeparator();

                var moveUpButton = menu.AddButton("Move up", OnMoveUpClicked);
                moveUpButton.Enabled = Index > 0;

                var moveDownButton = menu.AddButton("Move down", OnMoveDownClicked);
                moveDownButton.Enabled = Index + 1 < Editor.Count;

                menu.AddButton("Remove", OnRemoveClicked);
            }

            private void OnMoveUpClicked(ContextMenuButton button)
            {
                Editor.Move(Index, Index - 1);
            }

            private void OnMoveDownClicked(ContextMenuButton button)
            {
                Editor.Move(Index, Index + 1);
            }

            private void OnRemoveClicked(ContextMenuButton button)
            {
                Editor.Remove(Index);
            }
        }

        private IntegerValueElement _size;
        private int _elementsCount;
        private bool _readOnly;
        private bool _canReorderItems;
        private bool _notNullItems;

        /// <summary>
        /// Gets the length of the collection.
        /// </summary>
        public abstract int Count { get; }

        /// <summary>
        /// Gets the type of the collection elements.
        /// </summary>
        public ScriptType ElementType
        {
            get
            {
                var type = Values.Type;
                return new ScriptType(type.IsGenericType ? type.GetGenericArguments()[0] : type.GetElementType());
            }
        }

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            _readOnly = false;
            _canReorderItems = true;
            _notNullItems = false;

            // No support for different collections for now
            if (HasDifferentValues || HasDifferentTypes)
                return;

            var size = Count;

            // Try get CollectionAttribute for collection editor meta
            var attributes = Values.GetAttributes();
            Type overrideEditorType = null;
            float spacing = 10.0f;
            var collection = (CollectionAttribute)attributes?.FirstOrDefault(x => x is CollectionAttribute);
            if (collection != null)
            {
                // TODO: handle NotNullItems by filtering child editors SetValue

                _readOnly = collection.ReadOnly;
                _canReorderItems = collection.CanReorderItems;
                _notNullItems = collection.NotNullItems;
                overrideEditorType = TypeUtils.GetType(collection.OverrideEditorTypeName).Type;
                spacing = collection.Spacing;
            }

            // Size
            if (_readOnly)
            {
                layout.Label("Size", size.ToString());
            }
            else
            {
                _size = layout.IntegerValue("Size");
                _size.IntValue.MinValue = 0;
                _size.IntValue.MaxValue = ushort.MaxValue;
                _size.IntValue.Value = size;
                _size.IntValue.ValueChanged += OnSizeChanged;
            }

            // Elements
            if (size > 0)
            {
                var elementType = ElementType;
                if (_canReorderItems)
                {
                    for (int i = 0; i < size; i++)
                    {
                        if (i != 0 && spacing > 0f)
                        {
                            if (layout.Children.Count > 0 && layout.Children[layout.Children.Count - 1] is PropertiesListElement propertiesListElement)
                                propertiesListElement.Space(spacing);
                            else
                                layout.Space(spacing);
                        }

                        var overrideEditor = overrideEditorType != null ? (CustomEditor)Activator.CreateInstance(overrideEditorType) : null;
                        layout.Object(new CollectionItemLabel(this, i), new ListValueContainer(elementType, i, Values), overrideEditor);
                    }
                }
                else
                {
                    for (int i = 0; i < size; i++)
                    {
                        if (i != 0 && spacing > 0f)
                        {
                            if (layout.Children.Count > 0 && layout.Children[layout.Children.Count - 1] is PropertiesListElement propertiesListElement)
                                propertiesListElement.Space(spacing);
                            else
                                layout.Space(spacing);
                        }

                        var overrideEditor = overrideEditorType != null ? (CustomEditor)Activator.CreateInstance(overrideEditorType) : null;
                        layout.Object("Element " + i, new ListValueContainer(elementType, i, Values), overrideEditor);
                    }
                }
            }
            _elementsCount = size;

            // Add/Remove buttons
            if (!_readOnly)
            {
                var area = layout.Space(20);
                var addButton = new Button(area.ContainerControl.Width - (16 + 16 + 2 + 2), 2, 16, 16)
                {
                    Text = "+",
                    TooltipText = "Add new item",
                    AnchorPreset = AnchorPresets.TopRight,
                    Parent = area.ContainerControl
                };
                addButton.Clicked += () =>
                {
                    if (IsSetBlocked)
                        return;

                    Resize(Count + 1);
                };
                var removeButton = new Button(addButton.Right + 2, addButton.Y, 16, 16)
                {
                    Text = "-",
                    TooltipText = "Remove last item",
                    AnchorPreset = AnchorPresets.TopRight,
                    Parent = area.ContainerControl,
                    Enabled = size > 0
                };
                removeButton.Clicked += () =>
                {
                    if (IsSetBlocked)
                        return;

                    Resize(Count - 1);
                };
            }
        }

        private void OnSizeChanged()
        {
            if (IsSetBlocked)
                return;

            Resize(_size.IntValue.Value);
        }

        /// <summary>
        /// Moves the specified item at the given index and swaps it with the other item. It supports undo.
        /// </summary>
        /// <param name="srcIndex">Index of the source item.</param>
        /// <param name="dstIndex">Index of the destination item to swap with.</param>
        private void Move(int srcIndex, int dstIndex)
        {
            if (IsSetBlocked)
                return;

            var cloned = CloneValues();

            var tmp = cloned[dstIndex];
            cloned[dstIndex] = cloned[srcIndex];
            cloned[srcIndex] = tmp;

            SetValue(cloned);
        }

        /// <summary>
        /// Removes the item at the specified index. It supports undo.
        /// </summary>
        /// <param name="index">The index of the item to remove.</param>
        private void Remove(int index)
        {
            if (IsSetBlocked)
                return;

            var newValues = Allocate(Count - 1);
            var oldValues = (IList)Values[0];

            for (int i = 0; i < index; i++)
            {
                newValues[i] = oldValues[i];
            }

            for (int i = index; i < newValues.Count; i++)
            {
                newValues[i] = oldValues[i + 1];
            }

            SetValue(newValues);
        }

        /// <summary>
        /// Allocates the collection of the specified size.
        /// </summary>
        /// <param name="size">The size.</param>
        /// <returns>The collection.</returns>
        protected abstract IList Allocate(int size);

        /// <summary>
        /// Resizes collection to the specified new size.
        /// </summary>
        /// <param name="newSize">The new size.</param>
        protected abstract void Resize(int newSize);

        /// <summary>
        /// Clones the collection values.
        /// </summary>
        protected abstract IList CloneValues();

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            // No support for different collections for now
            if (HasDifferentValues || HasDifferentTypes)
                return;

            // Check if collection has been resized (by UI or from external source)
            if (Count != _elementsCount)
            {
                RebuildLayout();
            }
        }
    }
}
