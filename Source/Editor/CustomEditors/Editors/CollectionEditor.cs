// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System;
using System.Collections;
using System.Linq;
using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.CustomEditors.GUI;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Utilities;

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

        /// <summary>
        /// Determines if value of collection can be null.
        /// </summary>
        protected bool NotNullItems;

        private IntegerValueElement _size;
        private Color _background;
        private int _elementsCount;
        private bool _readOnly;
        private bool _canReorderItems;

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
                return type.IsGenericType ? new ScriptType(type.GetGenericArguments()[0]) : type.GetElementType();
            }
        }

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            // No support for different collections for now
            if (HasDifferentValues || HasDifferentTypes)
                return;

            var size = Count;
            _readOnly = false;
            _canReorderItems = true;
            _background = FlaxEngine.GUI.Style.Current.CollectionBackgroundColor;
            NotNullItems = false;

            // Try get CollectionAttribute for collection editor meta
            var attributes = Values.GetAttributes();
            Type overrideEditorType = null;
            float spacing = 10.0f;
            var collection = (CollectionAttribute)attributes?.FirstOrDefault(x => x is CollectionAttribute);
            if (collection != null)
            {
                _readOnly = collection.ReadOnly;
                _canReorderItems = collection.CanReorderItems;
                NotNullItems = collection.NotNullItems;
                if (collection.BackgroundColor.HasValue)
                    _background = collection.BackgroundColor.Value;
                overrideEditorType = TypeUtils.GetType(collection.OverrideEditorTypeName).Type;
                spacing = collection.Spacing;
            }

            // Size
            if (_readOnly || (NotNullItems && size == 0))
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
                var panel = layout.VerticalPanel();
                panel.Panel.BackgroundColor = _background;
                var elementType = ElementType;

                // Use separate layout cells for each collection items to improve layout updates for them in separation
                var useSharedLayout = elementType.IsPrimitive || elementType.IsEnum;

                if (_canReorderItems)
                {
                    for (int i = 0; i < size; i++)
                    {
                        if (i != 0 && spacing > 0f)
                        {
                            if (panel.Children.Count > 0 && panel.Children[panel.Children.Count - 1] is PropertiesListElement propertiesListElement)
                            {
                                if (propertiesListElement.Labels.Count > 0)
                                {
                                    var label = propertiesListElement.Labels[propertiesListElement.Labels.Count - 1];
                                    var margin = label.Margin;
                                    margin.Bottom += spacing;
                                    label.Margin = margin;
                                }
                                propertiesListElement.Space(spacing);
                            }
                            else
                            {
                                panel.Space(spacing);
                            }
                        }

                        var overrideEditor = overrideEditorType != null ? (CustomEditor)Activator.CreateInstance(overrideEditorType) : null;
                        var property = panel.AddPropertyItem(new CollectionItemLabel(this, i));
                        var itemLayout = useSharedLayout ? (LayoutElementsContainer)property : property.VerticalPanel();
                        itemLayout.Object(new ListValueContainer(elementType, i, Values, attributes), overrideEditor);
                    }
                }
                else
                {
                    for (int i = 0; i < size; i++)
                    {
                        if (i != 0 && spacing > 0f)
                        {
                            if (panel.Children.Count > 0 && panel.Children[panel.Children.Count - 1] is PropertiesListElement propertiesListElement)
                                propertiesListElement.Space(spacing);
                            else
                                panel.Space(spacing);
                        }

                        var overrideEditor = overrideEditorType != null ? (CustomEditor)Activator.CreateInstance(overrideEditorType) : null;
                        var property = panel.AddPropertyItem("Element " + i);
                        var itemLayout = useSharedLayout ? (LayoutElementsContainer)property : property.VerticalPanel();
                        itemLayout.Object(new ListValueContainer(elementType, i, Values, attributes), overrideEditor);
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
                    Parent = area.ContainerControl,
                    Enabled = !NotNullItems || size > 0,
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
                    Enabled = size > 0,
                };
                removeButton.Clicked += () =>
                {
                    if (IsSetBlocked)
                        return;

                    Resize(Count - 1);
                };
            }
        }

        /// <summary>
        /// Rebuilds the parent layout if its collection.
        /// </summary>
        public void RebuildParentCollection()
        {
            if (ParentEditor is DictionaryEditor dictionaryEditor)
            {
                dictionaryEditor.RebuildParentCollection();
                dictionaryEditor.RebuildLayout();
                return;
            }
            if (ParentEditor is CollectionEditor collectionEditor)
            {
                collectionEditor.RebuildParentCollection();
                collectionEditor.RebuildLayout();
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
                RebuildParentCollection();
            }
        }

        /// <inheritdoc />
        protected override bool OnDirty(CustomEditor editor, object value, object token = null)
        {
            if (NotNullItems)
            {
                if (value == null && editor.ParentEditor == this)
                    return false;
                if (editor == this && value is IList list)
                {
                    for (int i = 0; i < list.Count; i++)
                    {
                        if (list[i] == null)
                            return false;
                    }
                }
            }
            return base.OnDirty(editor, value, token);
        }
    }
}
