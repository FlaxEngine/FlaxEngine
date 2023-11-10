// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System;
using System.Collections;
using System.Linq;
using FlaxEditor.Content;
using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.CustomEditors.GUI;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.Drag;
using FlaxEditor.SceneGraph;
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
            if (HasDifferentTypes)
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

            var dragArea = layout.CustomContainer<DragAreaControl>();
            dragArea.CustomControl.Editor = this;
            dragArea.CustomControl.ElementType = ElementType;

            // Check for the AssetReferenceAttribute. In JSON assets, it can be used to filter
            // which scripts can be dragged over and dropped on this collection editor.
            var assetReference = (AssetReferenceAttribute)attributes?.FirstOrDefault(x => x is AssetReferenceAttribute);
            if (assetReference != null)
            {
                if (string.IsNullOrEmpty(assetReference.TypeName))
                {
                }
                else if (assetReference.TypeName.Length > 1 && assetReference.TypeName[0] == '.')
                {
                    dragArea.CustomControl.ElementType = ScriptType.Null;
                    dragArea.CustomControl.FileExtension = assetReference.TypeName;
                }
                else
                {
                    var customType = TypeUtils.GetType(assetReference.TypeName);
                    if (customType != ScriptType.Null)
                        dragArea.CustomControl.ElementType = customType;
                    else if (!Content.Settings.GameSettings.OptionalPlatformSettings.Contains(assetReference.TypeName))
                        Debug.LogWarning(string.Format("Unknown asset type '{0}' to use for drag and drop filter.", assetReference.TypeName));
                    else
                        dragArea.CustomControl.ElementType = ScriptType.Void;
                }
            }

            // Size
            if (_readOnly || (NotNullItems && size == 0))
            {
                dragArea.Label("Size", size.ToString());
            }
            else
            {
                _size = dragArea.IntegerValue("Size");
                _size.IntValue.MinValue = 0;
                _size.IntValue.MaxValue = ushort.MaxValue;
                _size.IntValue.Value = size;
                _size.IntValue.EditEnd += OnSizeChanged;
            }

            // Elements
            if (size > 0)
            {
                var panel = dragArea.VerticalPanel();
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
                var panel = dragArea.HorizontalPanel();
                panel.Panel.Size = new Float2(0, 20);
                panel.Panel.Margin = new Margin(2);

                var removeButton = panel.Button("-", "Remove last item");
                removeButton.Button.Size = new Float2(16, 16);
                removeButton.Button.Enabled = size > 0;
                removeButton.Button.AnchorPreset = AnchorPresets.TopRight;
                removeButton.Button.Clicked += () =>
                {
                    if (IsSetBlocked)
                        return;

                    Resize(Count - 1);
                };

                var addButton = panel.Button("+", "Add new item");
                addButton.Button.Size = new Float2(16, 16);
                addButton.Button.Enabled = !NotNullItems || size > 0;
                addButton.Button.AnchorPreset = AnchorPresets.TopRight;
                addButton.Button.Clicked += () =>
                {
                    if (IsSetBlocked)
                        return;

                    Resize(Count + 1);
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

        private class DragAreaControl : VerticalPanel
        {
            private DragItems _dragItems;
            private DragActors _dragActors;
            private DragHandlers _dragHandlers;
            private AssetPickerValidator _pickerValidator;

            public ScriptType ElementType
            {
                get => _pickerValidator?.AssetType ?? ScriptType.Null;
                set => _pickerValidator = new AssetPickerValidator(value);
            }

            public CollectionEditor Editor { get; set; }

            public string FileExtension
            {
                set => _pickerValidator.FileExtension = value;
            }

            /// <inheritdoc />
            public override void Draw()
            {
                if (_dragHandlers is { HasValidDrag: true })
                {
                    var area = new Rectangle(Float2.Zero, Size);
                    Render2D.FillRectangle(area, Color.Orange * 0.5f);
                    Render2D.DrawRectangle(area, Color.Black);
                }

                base.Draw();
            }

            public override void OnDestroy()
            {
                _pickerValidator.OnDestroy();
            }

            private bool ValidateActors(ActorNode node)
            {
                return node.Actor.GetScript(ElementType.Type) || ElementType.Type.IsAssignableTo(typeof(Actor));
            }

            /// <inheritdoc />
            public override DragDropEffect OnDragEnter(ref Float2 location, DragData data)
            {
                var result = base.OnDragEnter(ref location, data);
                if (result != DragDropEffect.None)
                    return result;

                if (_dragHandlers == null)
                {
                    _dragItems = new DragItems(_pickerValidator.IsValid);
                    _dragActors = new DragActors(ValidateActors);
                    _dragHandlers = new DragHandlers
                    {
                        _dragActors,
                        _dragItems
                    };
                }
                return _dragHandlers.OnDragEnter(data);
            }

            /// <inheritdoc />
            public override DragDropEffect OnDragMove(ref Float2 location, DragData data)
            {
                var result = base.OnDragMove(ref location, data);
                if (result != DragDropEffect.None)
                    return result;

                return _dragHandlers.Effect;
            }

            /// <inheritdoc />
            public override void OnDragLeave()
            {
                _dragHandlers.OnDragLeave();

                base.OnDragLeave();
            }

            /// <inheritdoc />
            public override DragDropEffect OnDragDrop(ref Float2 location, DragData data)
            {
                var result = base.OnDragDrop(ref location, data);
                if (result != DragDropEffect.None)
                {
                    _dragHandlers.OnDragDrop(null);
                    return result;
                }

                if (_dragHandlers.HasValidDrag)
                {
                    if (_dragItems.HasValidDrag)
                    {
                        var list = Editor.CloneValues();
                        if (list == null)
                        {
                            if (Editor.Values.Type.IsArray)
                            {
                                list = TypeUtils.CreateArrayInstance(Editor.Values.Type.GetElementType(), 0);
                            }
                            else
                            {
                                list = Editor.Values.Type.CreateInstance() as IList;
                            }
                        }
                        if (list.IsFixedSize)
                        {
                            var oldSize = list.Count;
                            var newSize = list.Count + _dragItems.Objects.Count;
                            var type = Editor.Values.Type.GetElementType();
                            var array = TypeUtils.CreateArrayInstance(type, newSize);
                            list.CopyTo(array, 0);

                            for (var i = oldSize; i < newSize; i++)
                            {
                                var validator = new AssetPickerValidator
                                {
                                    FileExtension = _pickerValidator.FileExtension,
                                    AssetType = _pickerValidator.AssetType,
                                    SelectedItem = _dragItems.Objects[i - oldSize],
                                };

                                if (typeof(AssetItem).IsAssignableFrom(ElementType.Type))
                                    array.SetValue(validator.SelectedItem, i);
                                else if (ElementType.Type == typeof(Guid))
                                    array.SetValue(validator.SelectedID, i);
                                else if (ElementType.Type == typeof(SceneReference))
                                    array.SetValue(new SceneReference(validator.SelectedID), i);
                                else if (ElementType.Type == typeof(string))
                                    array.SetValue(validator.SelectedPath, i);
                                else
                                    array.SetValue(validator.SelectedAsset, i);

                                validator.OnDestroy();
                            }
                            Editor.SetValue(array);
                        }
                        else
                        {
                            foreach (var item in _dragItems.Objects)
                            {
                                var validator = new AssetPickerValidator
                                {
                                    FileExtension = _pickerValidator.FileExtension,
                                    AssetType = _pickerValidator.AssetType,
                                    SelectedItem = item,
                                };

                                if (typeof(AssetItem).IsAssignableFrom(ElementType.Type))
                                    list.Add(validator.SelectedItem);
                                else if (ElementType.Type == typeof(Guid))
                                    list.Add(validator.SelectedID);
                                else if (ElementType.Type == typeof(SceneReference))
                                    list.Add(new SceneReference(validator.SelectedID));
                                else if (ElementType.Type == typeof(string))
                                    list.Add(validator.SelectedPath);
                                else
                                    list.Add(validator.SelectedAsset);

                                validator.OnDestroy();
                            }
                            Editor.SetValue(list);
                        }
                    }
                    else if (_dragActors.HasValidDrag)
                    {
                        var list = Editor.CloneValues();
                        if (list == null)
                        {
                            if (Editor.Values.Type.IsArray)
                            {
                                list = TypeUtils.CreateArrayInstance(Editor.Values.Type.GetElementType(), 0);
                            }
                            else
                            {
                                list = Editor.Values.Type.CreateInstance() as IList;
                            }
                        }

                        if (list.IsFixedSize)
                        {
                            var oldSize = list.Count;
                            var newSize = list.Count + _dragActors.Objects.Count;
                            var type = Editor.Values.Type.GetElementType();
                            var array = TypeUtils.CreateArrayInstance(type, newSize);
                            list.CopyTo(array, 0);

                            for (var i = oldSize; i < newSize; i++)
                            {
                                var actor = _dragActors.Objects[i - oldSize].Actor;
                                if (ElementType.Type.IsAssignableTo(typeof(Actor)))
                                {
                                    array.SetValue(actor, i);
                                }
                                else
                                {
                                    array.SetValue(actor.GetScript(ElementType.Type), i);
                                }
                            }
                            Editor.SetValue(array);
                        }
                        else
                        {
                            foreach (var actorNode in _dragActors.Objects)
                            {
                                if (ElementType.Type.IsAssignableTo(typeof(Actor)))
                                {
                                    list.Add(actorNode.Actor);
                                }
                                else
                                {
                                    list.Add(actorNode.Actor.GetScript(ElementType.Type));
                                }
                            }
                            Editor.SetValue(list);
                        }
                    }

                    _dragHandlers.OnDragDrop(null);
                }

                return result;
            }
        }
    }
}
