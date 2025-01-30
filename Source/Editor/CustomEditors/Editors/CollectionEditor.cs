// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using FlaxEditor.CustomEditors.GUI;
using FlaxEditor.GUI.Input;
using FlaxEditor.Content;
using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.Drag;
using FlaxEditor.SceneGraph;
using FlaxEditor.Scripting;
using FlaxEditor.Windows;
using FlaxEditor.Windows.Assets;
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

            private Rectangle _arrangeButtonRect;
            private bool _arrangeButtonInUse;

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
                _arrangeButtonRect = new Rectangle(2, 4, 12, 12);

                // Extend margin of the label to support a dragging handle.
                Margin m = Margin;
                m.Left += 16;
                Margin = m;
            }

            private void OnSetupContextMenu(PropertyNameLabel label, ContextMenu menu, CustomEditor linkedEditor)
            {
                menu.ItemsContainer.RemoveChildren();

                menu.AddButton("Copy", linkedEditor.Copy);
                var b = menu.AddButton("Paste", linkedEditor.Paste);
                b.Enabled = linkedEditor.CanPaste && !Editor._readOnly;

                menu.AddSeparator();
                b = menu.AddButton("Move up", OnMoveUpClicked);
                b.Enabled = Index > 0 && !Editor._readOnly;

                b = menu.AddButton("Move down", OnMoveDownClicked);
                b.Enabled = Index + 1 < Editor.Count && !Editor._readOnly;

                b = menu.AddButton("Remove", OnRemoveClicked);
                b.Enabled = !Editor._readOnly;
            }

            /// <inheritdoc />
            public override void OnEndMouseCapture()
            {
                base.OnEndMouseCapture();

                _arrangeButtonInUse = false;
            }

            /// <inheritdoc />
            public override void Draw()
            {
                base.Draw();

                var style = FlaxEngine.GUI.Style.Current;
                var mousePosition = PointFromScreen(Input.MouseScreenPosition);
                var dragBarColor = _arrangeButtonRect.Contains(mousePosition) ? style.Foreground : style.ForegroundGrey;
                Render2D.DrawSprite(FlaxEditor.Editor.Instance.Icons.DragBar12, _arrangeButtonRect, _arrangeButtonInUse ? Color.Orange : dragBarColor);
                if (_arrangeButtonInUse && ArrangeAreaCheck(out _, out var arrangeTargetRect))
                {
                    Render2D.FillRectangle(arrangeTargetRect, style.Selection);
                }
            }

            /// <inheritdoc />
            protected override void OnSizeChanged()
            {
                base.OnSizeChanged();

                _arrangeButtonRect.Y = (Height - _arrangeButtonRect.Height) * 0.5f;
            }

            private bool ArrangeAreaCheck(out int index, out Rectangle rect)
            {
                var child = Editor.ChildrenEditors[0];
                var container = child.Layout.ContainerControl;
                var mousePosition = container.PointFromScreen(Input.MouseScreenPosition);
                var barSidesExtend = 20.0f;
                var barHeight = 5.0f;
                var barCheckAreaHeight = 40.0f;
                var pos = mousePosition.Y + barCheckAreaHeight * 0.5f;

                for (int i = 0; i < container.Children.Count / 2; i++)
                {
                    var containerChild = container.Children[i * 2]; // times 2 to skip the value editor
                    if (Mathf.IsInRange(pos, containerChild.Top, containerChild.Top + barCheckAreaHeight) || (i == 0 && pos < containerChild.Top))
                    {
                        index = i;
                        var p1 = containerChild.UpperLeft;
                        rect = new Rectangle(PointFromParent(p1) - new Float2(barSidesExtend * 0.5f, barHeight * 0.5f), Width + barSidesExtend, barHeight);
                        return true;
                    }
                }

                var p2 = container.Children[((container.Children.Count / 2) - 1) * 2].BottomLeft;
                if (pos > p2.Y)
                {
                    index = (container.Children.Count / 2) - 1;
                    rect = new Rectangle(PointFromParent(p2) - new Float2(barSidesExtend * 0.5f, barHeight * 0.5f), Width + barSidesExtend, barHeight);
                    return true;
                }

                index = -1;
                rect = Rectangle.Empty;
                return false;
            }

            /// <inheritdoc />
            public override bool OnMouseDown(Float2 location, MouseButton button)
            {
                if (button == MouseButton.Left && _arrangeButtonRect.Contains(ref location))
                {
                    _arrangeButtonInUse = true;
                    Focus();
                    StartMouseCapture();
                    return true;
                }

                return base.OnMouseDown(location, button);
            }

            /// <inheritdoc />
            public override bool OnMouseUp(Float2 location, MouseButton button)
            {
                if (button == MouseButton.Left && _arrangeButtonInUse)
                {
                    _arrangeButtonInUse = false;
                    EndMouseCapture();
                    if (ArrangeAreaCheck(out var index, out _))
                    {
                        Editor.Shift(Index, index);
                    }
                }

                return base.OnMouseUp(location, button);
            }

            /// <inheritdoc />
            public override void OnLostFocus()
            {
                if (_arrangeButtonInUse)
                {
                    _arrangeButtonInUse = false;
                    EndMouseCapture();
                }

                base.OnLostFocus();
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

            private Rectangle _arrangeButtonRect;
            private bool _arrangeButtonInUse;

            public void Setup(CollectionEditor editor, int index, bool canReorder = true)
            {
                HeaderHeight = 18;
                _canReorder = canReorder;
                EnableDropDownIcon = true;
                var icons = FlaxEditor.Editor.Instance.Icons;
                ArrowImageClosed = new SpriteBrush(icons.ArrowRight12);
                ArrowImageOpened = new SpriteBrush(icons.ArrowDown12);
                HeaderText = $"Element {index}";
                
                string saveName = string.Empty;
                if (editor.Presenter?.Owner is PropertiesWindow propertiesWindow)
                {
                    var selection = FlaxEditor.Editor.Instance.SceneEditing.Selection[0];
                    if (selection != null)
                    {
                        saveName += $"{selection.ID},";
                    }
                }
                else if (editor.Presenter?.Owner is PrefabWindow prefabWindow)
                {
                    var selection = prefabWindow.Selection[0];
                    if (selection != null)
                    {
                        saveName += $"{selection.ID},";
                    }
                }
                if (editor.ParentEditor?.Layout.ContainerControl is DropPanel pdp)
                {
                    saveName += $"{pdp.HeaderText},";
                }
                if (editor.Layout.ContainerControl is DropPanel mainGroup)
                {
                    saveName += $"{mainGroup.HeaderText}";
                    IsClosed = FlaxEditor.Editor.Instance.ProjectCache.IsGroupToggled($"{saveName}:{index}");
                }
                else
                {
                    IsClosed = false;
                }
                
                Editor = editor;
                Index = index;
                Offsets = new Margin(7, 7, 0, 0);

                MouseButtonRightClicked += OnMouseButtonRightClicked;
                if (_canReorder)
                {
                    HeaderTextMargin = new Margin(18, 0, 0, 0);
                    _arrangeButtonRect = new Rectangle(16, 3, 12, 12);
                }
                IsClosedChanged += OnIsClosedChanged;
            }

            private void OnIsClosedChanged(DropPanel panel)
            {
                string saveName = string.Empty;
                if (Editor.Presenter?.Owner is PropertiesWindow pw)
                {
                    var selection = FlaxEditor.Editor.Instance.SceneEditing.Selection[0];
                    if (selection != null)
                    {
                        saveName += $"{selection.ID},";
                    }
                }
                else if (Editor.Presenter?.Owner is PrefabWindow prefabWindow)
                {
                    var selection = prefabWindow.Selection[0];
                    if (selection != null)
                    {
                        saveName += $"{selection.ID},";
                    }
                }
                if (Editor.ParentEditor?.Layout.ContainerControl is DropPanel pdp)
                {
                    saveName += $"{pdp.HeaderText},";
                }
                if (Editor.Layout.ContainerControl is DropPanel mainGroup)
                {
                    saveName += $"{mainGroup.HeaderText}";
                    FlaxEditor.Editor.Instance.ProjectCache.SetGroupToggle($"{saveName}:{Index}", panel.IsClosed);
                }
            }

            private bool ArrangeAreaCheck(out int index, out Rectangle rect)
            {
                var container = Parent;
                var mousePosition = container.PointFromScreen(Input.MouseScreenPosition);
                var barSidesExtend = 20.0f;
                var barHeight = 5.0f;
                var barCheckAreaHeight = 40.0f;
                var pos = mousePosition.Y + barCheckAreaHeight * 0.5f;

                for (int i = 0; i < (container.Children.Count + 1) / 2; i++) // Add 1 to pretend there is a spacer at the end.
                {
                    var containerChild = container.Children[i * 2]; // times 2 to skip the value editor
                    if (Mathf.IsInRange(pos, containerChild.Top, containerChild.Top + barCheckAreaHeight) || (i == 0 && pos < containerChild.Top))
                    {
                        index = i;
                        var p1 = containerChild.UpperLeft;
                        rect = new Rectangle(PointFromParent(p1) - new Float2(barSidesExtend * 0.5f, barHeight * 0.5f), Width + barSidesExtend, barHeight);
                        return true;
                    }
                }

                var p2 = container.Children[container.Children.Count - 1].BottomLeft;
                if (pos > p2.Y)
                {
                    index = ((container.Children.Count + 1) / 2) - 1;
                    rect = new Rectangle(PointFromParent(p2) - new Float2(barSidesExtend * 0.5f, barHeight * 0.5f), Width + barSidesExtend, barHeight);
                    return true;
                }

                index = -1;
                rect = Rectangle.Empty;
                return false;
            }

            public override void Draw()
            {
                base.Draw();

                if (_canReorder)
                {
                    var style = FlaxEngine.GUI.Style.Current;
                    var mousePosition = PointFromScreen(Input.MouseScreenPosition);
                    var dragBarColor = _arrangeButtonRect.Contains(mousePosition) ? style.Foreground : style.ForegroundGrey;
                    Render2D.DrawSprite(FlaxEditor.Editor.Instance.Icons.DragBar12, _arrangeButtonRect, _arrangeButtonInUse ? Color.Orange : dragBarColor);
                    if (_arrangeButtonInUse && ArrangeAreaCheck(out _, out var arrangeTargetRect))
                    {
                        Render2D.FillRectangle(arrangeTargetRect, style.Selection);
                    }
                }
            }

            /// <inheritdoc />
            public override bool OnMouseDown(Float2 location, MouseButton button)
            {
                if (button == MouseButton.Left && _arrangeButtonRect.Contains(ref location))
                {
                    _arrangeButtonInUse = true;
                    Focus();
                    StartMouseCapture();
                    return true;
                }

                return base.OnMouseDown(location, button);
            }

            /// <inheritdoc />
            public override bool OnMouseUp(Float2 location, MouseButton button)
            {
                if (button == MouseButton.Left && _arrangeButtonInUse)
                {
                    _arrangeButtonInUse = false;
                    EndMouseCapture();
                    if (ArrangeAreaCheck(out var index, out _))
                    {
                        Editor.Shift(Index, index);
                    }
                }

                return base.OnMouseUp(location, button);
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
        private int _elementsCount, _minCount, _maxCount;
        private bool _readOnly;
        private bool _canResize;
        private bool _canReorderItems;
        private CollectionAttribute.DisplayType _displayType;
        private List<CollectionDropPanel> _cachedDropPanels = new List<CollectionDropPanel>();

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
        public override bool RevertValueWithChildren => false; // Always revert value for a whole collection

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            // No support for different collections for now
            if (HasDifferentTypes)
                return;

            var size = Count;
            _readOnly = false;
            _canResize = true;
            _canReorderItems = true;
            _minCount = 0;
            _maxCount = 0;
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
                _canResize = collection.CanResize;
                _readOnly = collection.ReadOnly;
                _minCount = collection.MinCount;
                _maxCount = collection.MaxCount;
                _canReorderItems = collection.CanReorderItems;
                NotNullItems = collection.NotNullItems;
                if (collection.BackgroundColor.HasValue)
                    _background = collection.BackgroundColor.Value;
                overrideEditorType = TypeUtils.GetType(collection.OverrideEditorTypeName).Type;
                spacing = collection.Spacing;
                _displayType = collection.Display;
            }
            if (attributes != null && attributes.Any(x => x is ReadOnlyAttribute))
            {
                _readOnly = true;
                _canResize = false;
                _canReorderItems = false;
            }
            if (_maxCount == 0)
                _maxCount = ushort.MaxValue;
            _canResize &= _minCount < _maxCount;

            var dragArea = layout.CustomContainer<DragAreaControl>();
            dragArea.CustomControl.Editor = this;
            dragArea.CustomControl.ElementType = ElementType;

            // Check for the AssetReferenceAttribute. In JSON assets, it can be used to filter which scripts can be dragged over and dropped on this collection editor
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
            if (layout.ContainerControl is DropPanel dropPanel)
            {
                var height = dropPanel.HeaderHeight - dropPanel.HeaderTextMargin.Height;
                var y = -dropPanel.HeaderHeight + dropPanel.HeaderTextMargin.Top;
                _sizeBox = new IntValueBox(size)
                {
                    MinValue = _minCount,
                    MaxValue = _maxCount,
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

                if (!_canResize || (NotNullItems && size == 0))
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
                var panel = dragArea.VerticalPanel();
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
                if (_cachedDropPanels == null)
                    _cachedDropPanels = new List<CollectionDropPanel>();
                else
                    _cachedDropPanels.Clear();
                for (int i = 0; i < size; i++)
                {
                    // Apply spacing
                    if (i > 0 && i < size && spacing > 0 && !single)
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
                        var itemLayout = (LayoutElementsContainer)property;
                        itemLabel.LinkedEditor = itemLayout.Object(new ListValueContainer(elementType, i, Values, attributes), overrideEditor);
                        if (_readOnly && itemLayout.Children.Count > 0)
                            GenericEditor.OnReadOnlyProperty(itemLayout);
                    }
                    else if (_displayType == CollectionAttribute.DisplayType.Header || (_displayType == CollectionAttribute.DisplayType.Default && !single))
                    {
                        var cdp = panel.CustomContainer<CollectionDropPanel>();
                        _cachedDropPanels.Add(cdp.CustomControl);
                        cdp.CustomControl.Setup(this, i, _canReorderItems);
                        var itemLayout = cdp.VerticalPanel();
                        cdp.CustomControl.LinkedEditor = itemLayout.Object(new ListValueContainer(elementType, i, Values, attributes), overrideEditor);
                        if (_readOnly && itemLayout.Children.Count > 0)
                            GenericEditor.OnReadOnlyProperty(itemLayout);
                    }
                }

                if (_displayType == CollectionAttribute.DisplayType.Header || (_displayType == CollectionAttribute.DisplayType.Default && !single))
                {
                    if (Layout is GroupElement g)
                        g.SetupContextMenu += OnSetupContextMenu;
                }
            }
            _elementsCount = size;

            // Add/Remove buttons
            if (_canResize && !_readOnly)
            {
                var panel = dragArea.HorizontalPanel();
                panel.Panel.Size = new Float2(0, 20);
                panel.Panel.Margin = new Margin(2);

                var removeButton = panel.Button("-", "Remove last item");
                removeButton.Button.Size = new Float2(16, 16);
                removeButton.Button.Enabled = size > _minCount;
                removeButton.Button.AnchorPreset = AnchorPresets.TopRight;
                removeButton.Button.Clicked += () =>
                {
                    if (IsSetBlocked)
                        return;
                    Resize(Count - 1);
                };

                var addButton = panel.Button("+", "Add new item");
                addButton.Button.Size = new Float2(16, 16);
                addButton.Button.Enabled = (!NotNullItems || size > 0) && size < _maxCount;
                addButton.Button.AnchorPreset = AnchorPresets.TopRight;
                addButton.Button.Clicked += () =>
                {
                    if (IsSetBlocked)
                        return;
                    Resize(Count + 1);
                };
            }
        }

        private void OnSetupContextMenu(ContextMenu menu, DropPanel panel)
        {
            if (menu.Items.Any(x => x is ContextMenuButton b && b.Text.Equals("Open All", StringComparison.Ordinal)))
                return;

            menu.AddSeparator();
            menu.AddButton("Open All", () =>
            {
                foreach (var cachedPanel in _cachedDropPanels)
                {
                    cachedPanel.IsClosed = false;
                }
            });
            menu.AddButton("Close All", () =>
            {
                foreach (var cachedPanel in _cachedDropPanels)
                {
                    cachedPanel.IsClosed = true;
                }
            });
        }

        /// <inheritdoc />
        protected override void Deinitialize()
        {
            _sizeBox = null;

            base.Deinitialize();
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
        /// Shifts the specified item at the given index and moves it through the list to the other item. It supports undo.
        /// </summary>
        /// <param name="srcIndex">Index of the source item.</param>
        /// <param name="dstIndex">Index of the destination to move to.</param>
        private void Shift(int srcIndex, int dstIndex)
        {
            if (IsSetBlocked)
                return;

            var cloned = CloneValues();
            if (dstIndex > srcIndex)
            {
                for (int i = srcIndex; i < dstIndex; i++)
                {
                    var tmp = cloned[i + 1];
                    cloned[i + 1] = cloned[i];
                    cloned[i] = tmp;
                }
            }
            else
            {
                for (int i = srcIndex; i > dstIndex; i--)
                {
                    var tmp = cloned[i - 1];
                    cloned[i - 1] = cloned[i];
                    cloned[i] = tmp;
                }
            }

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

            // Update reference/default value indicator
            if (_sizeBox != null)
            {
                var color = Color.Transparent;
                if (Values.HasReferenceValue && Values.ReferenceValue is IList referenceValue && referenceValue.Count != Count)
                    color = FlaxEngine.GUI.Style.Current.BackgroundSelected;
                else if (Values.HasDefaultValue && Values.DefaultValue is IList defaultValue && defaultValue.Count != Count)
                    color = Color.Yellow * 0.8f;
                _sizeBox.BorderColor = color;
            }

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
                    var style = FlaxEngine.GUI.Style.Current;
                    var area = new Rectangle(Float2.Zero, Size);
                    Render2D.FillRectangle(area, style.Selection);
                    Render2D.DrawRectangle(area, style.SelectionBorder);
                }

                base.Draw();
            }

            /// <inheritdoc />
            public override void OnDestroy()
            {
                _pickerValidator.OnDestroy();
                _pickerValidator = null;

                base.OnDestroy();
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
                if (result != DragDropEffect.None || _dragHandlers == null)
                    return result;

                return _dragHandlers.Effect;
            }

            /// <inheritdoc />
            public override void OnDragLeave()
            {
                _dragHandlers?.OnDragLeave();

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
