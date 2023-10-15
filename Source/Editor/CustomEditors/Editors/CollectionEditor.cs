// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System;
using System.Collections;
using System.Linq;
using FlaxEditor.CustomEditors.GUI;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.Input;
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
            
            private Image _moveUpImage;
            private Image _moveDownImage;

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

                var icons = FlaxEditor.Editor.Instance.Icons;
                var style = FlaxEngine.GUI.Style.Current;
                var imageSize = 18;
                var indexAmount = Index == 0 ? 2 : 0;
                _moveDownImage = new Image
                {
                    Brush = new SpriteBrush(icons.Down32),
                    TooltipText = "Move down",
                    IsScrollable = false,
                    AnchorPreset = AnchorPresets.MiddleRight,
                    Bounds = new Rectangle(-imageSize, -Height * 0.5f, imageSize, imageSize),
                    Color = style.ForegroundGrey,
                    Margin = new Margin(1),
                    Parent = this,
                };
                _moveDownImage.Clicked += MoveDownImageOnClicked;
                _moveDownImage.Enabled = Index + 1 < Editor.Count;
                _moveUpImage = new Image
                {
                    Brush = new SpriteBrush(icons.Up32),
                    TooltipText = "Move up",
                    IsScrollable = false,
                    AnchorPreset = AnchorPresets.MiddleRight,
                    Bounds = new Rectangle(-(imageSize * 2 + 2), -Height * 0.5f, imageSize, imageSize),
                    Color = style.ForegroundGrey,
                    Margin = new Margin(1),
                    Parent = this,
                };
                _moveUpImage.Clicked += MoveUpImageOnClicked;
                _moveUpImage.Enabled = Index > 0;

                SetupContextMenu += OnSetupContextMenu;
            }
            
            private void MoveUpImageOnClicked(Image image, MouseButton button)
            {
                OnMoveUpClicked();
            }
            
            private void MoveDownImageOnClicked(Image image, MouseButton button)
            {
                OnMoveDownClicked();
            }

            private void OnSetupContextMenu(PropertyNameLabel label, ContextMenu menu, CustomEditor linkedEditor)
            {
                menu.ItemsContainer.RemoveChildren();
                
                menu.AddButton("Copy", linkedEditor.Copy);
                var paste = menu.AddButton("Paste", linkedEditor.Paste);
                paste.Enabled = linkedEditor.CanPaste;
                
                menu.AddSeparator();
                var moveUpButton = menu.AddButton("Move up", OnMoveUpClicked);
                moveUpButton.Enabled = Index > 0;

                var moveDownButton = menu.AddButton("Move down", OnMoveDownClicked);
                moveDownButton.Enabled = Index + 1 < Editor.Count;

                menu.AddButton("Remove", OnRemoveClicked);
            }

            private void OnMoveUpClicked()
            {
                Editor.Move(Index, Index - 1);
            }

            private void OnMoveDownClicked()
            {
                Editor.Move(Index, Index + 1);
            }

            private void OnRemoveClicked()
            {
                Editor.Remove(Index);
            }
        }

        private class CollectionDropPanel : DropPanel
        {
            /// <summary>
            /// The collection editor.
            /// </summary>
            public CollectionEditor Editor;

            /// <summary>
            /// The index of the item (zero-based).
            /// </summary>
            public int Index { get; private set; }

            /// <summary>
            /// The linked editor.
            /// </summary>
            public CustomEditor LinkedEditor;

            private bool _canReorder = true;
            private Image _moveUpImage;
            private Image _moveDownImage;

            public void Setup(CollectionEditor editor, int index, bool canReorder = true)
            {
                HeaderHeight = 18;
                _canReorder = canReorder;
                EnableDropDownIcon = true;
                var icons = FlaxEditor.Editor.Instance.Icons;
                ArrowImageClosed = new SpriteBrush(icons.ArrowRight12);
                ArrowImageOpened = new SpriteBrush(icons.ArrowDown12);
                HeaderText = $"Element {index}";
                IsClosed = false;
                Editor = editor;
                Index = index;
                Offsets = new Margin(7, 7, 0, 0);
                
                MouseButtonRightClicked += OnMouseButtonRightClicked;
                if (_canReorder)
                {
                    var imageSize = HeaderHeight;
                    var style = FlaxEngine.GUI.Style.Current;
                    _moveDownImage = new Image
                    {
                        Brush = new SpriteBrush(icons.Down32),
                        TooltipText = "Move down",
                        IsScrollable = false,
                        AnchorPreset = AnchorPresets.TopRight,
                        Bounds = new Rectangle(-(imageSize + ItemsMargin.Right + 2), -HeaderHeight, imageSize, imageSize),
                        Color = style.ForegroundGrey,
                        Margin = new Margin(1),
                        Parent = this,
                    };
                    _moveDownImage.Clicked += MoveDownImageOnClicked;
                    _moveDownImage.Enabled = Index + 1 < Editor.Count;
                
                    _moveUpImage = new Image
                    {
                        Brush = new SpriteBrush(icons.Up32),
                        TooltipText = "Move up",
                        IsScrollable = false,
                        AnchorPreset = AnchorPresets.TopRight,
                        Bounds = new Rectangle(-(imageSize * 2 + ItemsMargin.Right + 2), -HeaderHeight, imageSize, imageSize),
                        Color = style.ForegroundGrey,
                        Margin = new Margin(1),
                        Parent = this,
                    };
                    _moveUpImage.Clicked += MoveUpImageOnClicked;
                    _moveUpImage.Enabled = Index > 0;
                }
            }

            private void MoveUpImageOnClicked(Image image, MouseButton button)
            {
                OnMoveUpClicked();
            }
            
            private void MoveDownImageOnClicked(Image image, MouseButton button)
            {
                OnMoveDownClicked();
            }

            private void OnMouseButtonRightClicked(DropPanel panel, Float2 location)
            {
                if (LinkedEditor == null)
                    return;
                var linkedEditor = LinkedEditor;
                var menu = new ContextMenu();

                menu.AddButton("Copy", linkedEditor.Copy);
                var paste = menu.AddButton("Paste", linkedEditor.Paste);
                paste.Enabled = linkedEditor.CanPaste;

                if (_canReorder)
                {
                    menu.AddSeparator();

                    var moveUpButton = menu.AddButton("Move up", OnMoveUpClicked);
                    moveUpButton.Enabled = Index > 0;

                    var moveDownButton = menu.AddButton("Move down", OnMoveDownClicked);
                    moveDownButton.Enabled = Index + 1 < Editor.Count;
                }

                menu.AddButton("Remove", OnRemoveClicked);

                menu.Show(panel, location);
            }

            private void OnMoveUpClicked()
            {
                Editor.Move(Index, Index - 1);
            }

            private void OnMoveDownClicked()
            {
                Editor.Move(Index, Index + 1);
            }

            private void OnRemoveClicked()
            {
                Editor.Remove(Index);
            }
        }

        /// <summary>
        /// Determines if value of collection can be null.
        /// </summary>
        protected bool NotNullItems;
        
        private IntValueBox _sizeBox;
        private Color _background;
        private int _elementsCount;
        private bool _readOnly;
        private bool _canReorderItems;
        private CollectionAttribute.DisplayType _displayType;

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
            _displayType = CollectionAttribute.DisplayType.Header;
            NotNullItems = false;

            // Try get CollectionAttribute for collection editor meta
            var attributes = Values.GetAttributes();
            Type overrideEditorType = null;
            float spacing = 1.0f;
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
                _displayType = collection.Display;
            }

            // Size
            if (layout.ContainerControl is DropPanel dropPanel)
            {
                var height = dropPanel.HeaderHeight - dropPanel.HeaderTextMargin.Height;
                var y = -dropPanel.HeaderHeight + dropPanel.HeaderTextMargin.Top;
                _sizeBox = new IntValueBox(size)
                {
                    MinValue = 0,
                    MaxValue = ushort.MaxValue,
                    AnchorPreset = AnchorPresets.TopRight,
                    Bounds = new Rectangle(-40 - dropPanel.ItemsMargin.Right, y, 40, height),
                    Parent = dropPanel,
                };

                var label = new Label
                {
                    Text = "Size",
                    AnchorPreset = AnchorPresets.TopRight,
                    Bounds = new Rectangle(-_sizeBox.Width - 40 - dropPanel.ItemsMargin.Right - 2, y, 40, height),
                    Parent = dropPanel
                };

                if (_readOnly || (NotNullItems && size == 0))
                {
                    _sizeBox.IsReadOnly = true;
                    _sizeBox.Enabled = false;
                }
                else
                {
                    _sizeBox.EditEnd += OnSizeChanged;
                }
            }

            // Elements
            if (size > 0)
            {
                var panel = layout.VerticalPanel();
                panel.Panel.Offsets = new Margin(7, 7, 0, 0);
                panel.Panel.BackgroundColor = _background;
                var elementType = ElementType;
                bool single = elementType.IsPrimitive ||
                              elementType.Equals(new ScriptType(typeof(string))) ||
                              elementType.IsEnum ||
                              (elementType.GetFields().Length == 1 && elementType.GetProperties().Length == 0) ||
                              (elementType.GetProperties().Length == 1 && elementType.GetFields().Length == 0) ||
                              elementType.Equals(new ScriptType(typeof(JsonAsset))) ||
                              elementType.Equals(new ScriptType(typeof(SettingsBase)));

                for (int i = 0; i < size; i++)
                {
                    // Apply spacing
                    if (i > 0 && i < size && spacing > 0)
                        panel.Space(spacing);
                    
                    var overrideEditor = overrideEditorType != null ? (CustomEditor)Activator.CreateInstance(overrideEditorType) : null;
                    if (_displayType == CollectionAttribute.DisplayType.Inline || (collection == null && single) || (_displayType == CollectionAttribute.DisplayType.Default && single))
                    {
                        PropertyNameLabel itemLabel;
                        if (_canReorderItems)
                            itemLabel = new CollectionItemLabel(this, i);
                        else
                            itemLabel = new PropertyNameLabel("Element " + i);
                        var property = panel.AddPropertyItem(itemLabel);
                        var itemLayout = property.VerticalPanel();
                        itemLabel.LinkedEditor = itemLayout.Object(new ListValueContainer(elementType, i, Values, attributes), overrideEditor);
                    }
                    else if (_displayType == CollectionAttribute.DisplayType.Header || (_displayType == CollectionAttribute.DisplayType.Default && !single))
                    {
                        var cdp = panel.CustomContainer<CollectionDropPanel>();
                        cdp.CustomControl.Setup(this, i, _canReorderItems);
                        var itemLayout = cdp.VerticalPanel();
                        cdp.CustomControl.LinkedEditor = itemLayout.Object(new ListValueContainer(elementType, i, Values, attributes), overrideEditor);
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

            Resize(_sizeBox.Value);
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
