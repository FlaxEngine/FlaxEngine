// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections;
using System.Linq;
using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.CustomEditors.GUI;
using FlaxEditor.GUI;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.Input;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Json;
using FlaxEngine.Utilities;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Default implementation of the inspector used to edit key-value dictionaries.
    /// </summary>
    public class DictionaryEditor : CustomEditor
    {
        /// <summary>
        /// The custom implementation of the dictionary items labels that can be used to remove items or edit keys.
        /// </summary>
        /// <seealso cref="FlaxEditor.CustomEditors.GUI.PropertyNameLabel" />
        private class DictionaryItemLabel : PropertyNameLabel
        {
            private DictionaryEditor _editor;
            private object _key;

            /// <summary>
            /// Initializes a new instance of the <see cref="DictionaryItemLabel"/> class.
            /// </summary>
            /// <param name="editor">The editor.</param>
            /// <param name="key">The key.</param>
            public DictionaryItemLabel(DictionaryEditor editor, object key)
            : base(key?.ToString() ?? "<null>")
            {
                _editor = editor;
                _key = key;

                SetupContextMenu += OnSetupContextMenu;
            }

            private void OnSetupContextMenu(PropertyNameLabel label, ContextMenu menu, CustomEditor linkedEditor)
            {
                if (menu.Items.Any())
                    menu.AddSeparator();
                menu.AddButton("Remove", OnRemoveClicked).Enabled = !_editor._readOnly;
                menu.AddButton("Edit", OnEditClicked).Enabled = _editor._canEditKeys;
            }

            private void OnRemoveClicked(ContextMenuButton button)
            {
                _editor.Remove(_key);
            }

            private void OnEditClicked(ContextMenuButton button)
            {
                var keyType = _editor.Values.Type.GetGenericArguments()[0];
                if (keyType == typeof(string) || keyType.IsPrimitive)
                {
                    // Edit as text
                    var popup = RenamePopup.Show(Parent, Rectangle.Margin(Bounds, Margin), Text, false);
                    popup.Validate += (renamePopup, value) =>
                    {
                        object newKey;
                        if (keyType.IsPrimitive)
                            newKey = JsonSerializer.Deserialize(value, keyType);
                        else
                            newKey = value;
                        return !((IDictionary)_editor.Values[0]).Contains(newKey);
                    };
                    popup.Renamed += renamePopup =>
                    {
                        object newKey;
                        if (keyType.IsPrimitive)
                            newKey = JsonSerializer.Deserialize(renamePopup.Text, keyType);
                        else
                            newKey = renamePopup.Text;
                        _editor.ChangeKey(_key, newKey);
                        _key = newKey;
                        Text = _key.ToString();
                    };
                }
                else if (keyType.IsEnum)
                {
                    // Edit via enum picker
                    var popup = RenamePopup.Show(Parent, Rectangle.Margin(Bounds, Margin), Text, false);
                    var picker = new EnumComboBox(keyType)
                    {
                        AnchorPreset = AnchorPresets.StretchAll,
                        Offsets = Margin.Zero,
                        Parent = popup,
                        EnumTypeValue = _key,
                    };
                    picker.ValueChanged += () =>
                    {
                        popup.Hide();
                        object newKey = picker.EnumTypeValue;
                        if (!((IDictionary)_editor.Values[0]).Contains(newKey))
                        {
                            _editor.ChangeKey(_key, newKey);
                            _key = newKey;
                            Text = _key.ToString();
                        }
                    };
                }
                else
                {
                    // Generic editor
                    var popup = ContextMenuBase.ShowEmptyMenu(Parent, Rectangle.Margin(Bounds, Margin));
                    var presenter = new CustomEditorPresenter(null);
                    presenter.Panel.AnchorPreset = AnchorPresets.StretchAll;
                    presenter.Panel.IsScrollable = false;
                    presenter.Panel.Parent = popup;
                    presenter.Select(_key);
                    presenter.Modified += () =>
                    {
                        popup.Hide();
                        object newKey = presenter.Selection[0];
                        _editor.ChangeKey(_key, newKey);
                        _key = newKey;
                        Text = _key?.ToString();
                    };
                }
            }

            /// <inheritdoc />
            public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
            {
                if (button == MouseButton.Left && _editor._canEditKeys)
                {
                    OnEditClicked(null);
                    return true;
                }

                return base.OnMouseDoubleClick(location, button);
            }

            /// <inheritdoc />
            public override void OnDestroy()
            {
                _editor = null;
                _key = null;

                base.OnDestroy();
            }
        }

        private IntValueBox _sizeBox;
        private Color _background;
        private int _elementsCount;
        private bool _readOnly;
        private bool _notNullItems;
        private bool _canEditKeys;
        private bool _keyEdited;
        private CollectionAttribute.DisplayType _displayType;

        /// <summary>
        /// Gets the length of the collection.
        /// </summary>
        public int Count => (Values[0] as IDictionary)?.Count ?? 0;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            // No support for different collections for now
            if (HasDifferentValues || HasDifferentTypes)
                return;

            var type = Values.Type;
            var size = Count;
            var argTypes = type.GetGenericArguments();
            var keyType = argTypes[0];
            var valueType = argTypes[1];
            _canEditKeys = keyType == typeof(string) || keyType.IsPrimitive || keyType.IsEnum || keyType.IsValueType;
            _background = FlaxEngine.GUI.Style.Current.CollectionBackgroundColor;
            _readOnly = false;
            _notNullItems = false;
            _displayType = CollectionAttribute.DisplayType.Header;

            // Try get CollectionAttribute for collection editor meta
            var attributes = Values.GetAttributes();
            Type overrideEditorType = null;
            float spacing = 0.0f;
            var collection = (CollectionAttribute)attributes?.FirstOrDefault(x => x is CollectionAttribute);
            if (collection != null)
            {
                _canEditKeys &= collection.CanReorderItems;
                _readOnly = collection.ReadOnly;
                _notNullItems = collection.NotNullItems;
                if (collection.BackgroundColor.HasValue)
                    _background = collection.BackgroundColor.Value;
                overrideEditorType = TypeUtils.GetType(collection.OverrideEditorTypeName).Type;
                spacing = collection.Spacing;
                _displayType = collection.Display;
            }
            if (attributes != null && attributes.Any(x => x is ReadOnlyAttribute))
            {
                _readOnly = true;
                _canEditKeys = false;
            }

            // Size
            if (layout.ContainerControl is DropPanel dropPanel)
            {
                var height = dropPanel.HeaderHeight - dropPanel.HeaderTextMargin.Height;
                var y = -dropPanel.HeaderHeight + dropPanel.HeaderTextMargin.Top;
                _sizeBox = new IntValueBox(size)
                {
                    MinValue = 0,
                    MaxValue = _notNullItems ? size : ushort.MaxValue,
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

                if (_readOnly || !_canEditKeys)
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
                panel.Panel.BackgroundColor = _background;
                var keysEnumerable = ((IDictionary)Values[0]).Keys.OfType<object>();
                var keys = keysEnumerable as object[] ?? keysEnumerable.ToArray();
                var valuesType = new ScriptType(valueType);

                // Use separate layout cells for each collection items to improve layout updates for them in separation
                var useSharedLayout = valueType.IsPrimitive || valueType.IsEnum;

                for (int i = 0; i < size; i++)
                {
                    if (i > 0 && i < size && spacing > 0)
                    {
                        panel.Space(spacing);
                    }

                    var key = keys.ElementAt(i);
                    var overrideEditor = overrideEditorType != null ? (CustomEditor)Activator.CreateInstance(overrideEditorType) : null;
                    var property = panel.AddPropertyItem(new DictionaryItemLabel(this, key));
                    var itemLayout = useSharedLayout ? (LayoutElementsContainer)property : property.VerticalPanel();
                    itemLayout.Object(new DictionaryValueContainer(valuesType, key, Values), overrideEditor);
                    if (_readOnly && itemLayout.Children.Count > 0)
                        GenericEditor.OnReadOnlyProperty(itemLayout);
                }
            }
            _elementsCount = size;

            // Add/Remove buttons
            if (!_readOnly && _canEditKeys)
            {
                var area = layout.Space(20);
                var addButton = new Button(area.ContainerControl.Width - (16 + 16 + 2 + 2), 2, 16, 16)
                {
                    Text = "+",
                    TooltipText = "Add new item",
                    AnchorPreset = AnchorPresets.TopRight,
                    Parent = area.ContainerControl,
                    Enabled = !_notNullItems,
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
        /// Removes the item of the specified key. It supports undo.
        /// </summary>
        /// <param name="key">The key of the item to remove.</param>
        private void Remove(object key)
        {
            if (IsSetBlocked)
                return;

            // Allocate new collection
            var dictionary = Values[0] as IDictionary;
            var type = Values.Type;
            var newValues = (IDictionary)type.CreateInstance();

            // Copy all keys/values except the specified one
            if (dictionary != null)
            {
                foreach (var e in dictionary.Keys)
                {
                    if (Equals(e, key))
                        continue;
                    newValues[e] = dictionary[e];
                }
            }

            SetValue(newValues);
        }

        /// <summary>
        /// Changes the key of the item.
        /// </summary>
        /// <param name="oldKey">The old key value.</param>
        /// <param name="newKey">The new key value.</param>
        protected void ChangeKey(object oldKey, object newKey)
        {
            var dictionary = (IDictionary)Values[0];
            var newValues = (IDictionary)Values.Type.CreateInstance();
            foreach (var e in dictionary.Keys)
            {
                if (Equals(e, oldKey))
                    newValues[newKey] = dictionary[e];
                else
                    newValues[e] = dictionary[e];
            }
            SetValue(newValues);
            _keyEdited = true; // TODO: use custom UndoAction to rebuild UI after key modification
        }

        /// <summary>
        /// Resizes collection to the specified new size.
        /// </summary>
        /// <param name="newSize">The new size.</param>
        protected void Resize(int newSize)
        {
            var dictionary = Values[0] as IDictionary;
            var oldSize = dictionary?.Count ?? 0;

            if (oldSize == newSize)
                return;

            // Allocate new collection
            var type = Values.Type;
            var argTypes = type.GetGenericArguments();
            var keyType = argTypes[0];
            var valueType = argTypes[1];
            var newValues = (IDictionary)type.CreateInstance();

            // Copy all keys/values
            int itemsLeft = newSize;
            if (dictionary != null)
            {
                foreach (var e in dictionary.Keys)
                {
                    if (itemsLeft == 0)
                        break;
                    newValues[e] = dictionary[e];
                    itemsLeft--;
                }
            }

            // Insert new items (find unique keys)
            int newItemsLeft = newSize - oldSize;
            while (newItemsLeft-- > 0)
            {
                object newKey = null;
                if (keyType.IsPrimitive)
                {
                    long uniqueKey = 0;
                    bool isUnique;
                    do
                    {
                        isUnique = true;
                        foreach (var e in newValues.Keys)
                        {
                            var asLong = Convert.ToInt64(e);
                            if (asLong.Equals(uniqueKey))
                            {
                                uniqueKey++;
                                isUnique = false;
                                break;
                            }
                        }
                    } while (!isUnique);
                    newKey = Convert.ChangeType(uniqueKey, keyType);
                }
                else if (keyType.IsEnum)
                {
                    var enumValues = Enum.GetValues(keyType);
                    int uniqueKeyIndex = 0;
                    bool isUnique;
                    do
                    {
                        isUnique = true;
                        foreach (var e in newValues.Keys)
                        {
                            if (Equals(e, enumValues.GetValue(uniqueKeyIndex)))
                            {
                                uniqueKeyIndex++;
                                isUnique = false;
                                break;
                            }
                        }
                    } while (!isUnique && uniqueKeyIndex < enumValues.Length);
                    newKey = enumValues.GetValue(uniqueKeyIndex);
                }
                else if (keyType == typeof(string))
                {
                    string uniqueKey = "Key";
                    bool isUnique;
                    do
                    {
                        isUnique = true;
                        foreach (var e in newValues.Keys)
                        {
                            if (string.Equals((string)e, uniqueKey, StringComparison.InvariantCulture))
                            {
                                uniqueKey += "*";
                                isUnique = false;
                                break;
                            }
                        }
                    } while (!isUnique);
                    newKey = uniqueKey;
                }
                else
                {
                    newKey = TypeUtils.GetDefaultValue(new ScriptType(keyType));
                }
                newValues[newKey] = TypeUtils.GetDefaultValue(new ScriptType(valueType));
            }

            SetValue(newValues);
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            if (_keyEdited)
            {
                _keyEdited = false;
                RebuildLayout();
                RebuildParentCollection();
            }

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
    }
}
