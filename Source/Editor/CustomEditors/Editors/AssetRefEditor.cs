// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Linq;
using FlaxEditor.Content;
using FlaxEditor.GUI;
using FlaxEditor.GUI.Drag;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Utilities;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Default implementation of the inspector used to edit reference to the <see cref="AssetItem"/>.
    /// </summary>
    [CustomEditor(typeof(AssetItem)), DefaultEditor]
    public sealed class AssetItemRefEditor : AssetRefEditor
    {
    }

    /// <summary>
    /// Default implementation of the inspector used to edit reference to the <see cref="SceneReference"/>.
    /// </summary>
    [CustomEditor(typeof(SceneReference)), DefaultEditor]
    public sealed class SceneRefEditor : AssetRefEditor
    {
    }

    /// <summary>
    /// Default implementation of the inspector used to edit reference to the <see cref="FlaxEngine.Asset"/>.
    /// </summary>
    /// <remarks>Supports editing reference to the asset using various containers: <see cref="Asset"/> or <see cref="AssetItem"/> or <see cref="Guid"/>.</remarks>
    [CustomEditor(typeof(Asset)), DefaultEditor]
    public class AssetRefEditor : CustomEditor
    {
        /// <summary>
        /// The asset picker used to get a reference to an asset.
        /// </summary>
        public AssetPicker Picker;

        private bool _isRefreshing;
        private ScriptType _valueType;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            if (HasDifferentTypes)
                return;
            Picker = layout.Custom<AssetPicker>().CustomControl;
            var value = Values[0];
            _valueType = Values.Type.Type != typeof(object) || value == null ? Values.Type : TypeUtils.GetObjectType(value);
            var assetType = _valueType;
            if (assetType == typeof(string))
                assetType = new ScriptType(typeof(Asset));
            else if (_valueType.Type != null && _valueType.Type.Name == typeof(JsonAssetReference<>).Name)
                assetType = new ScriptType(_valueType.Type.GenericTypeArguments[0]);
            Picker.Validator.AssetType = assetType;
            ApplyAssetReferenceAttribute(Values, out var height, Picker.Validator);
            Picker.Height = height;
            Picker.SelectedItemChanged += OnSelectedItemChanged;
        }

        private void OnSelectedItemChanged()
        {
            if (_isRefreshing)
                return;
            if (typeof(AssetItem).IsAssignableFrom(_valueType.Type))
                SetValue(Picker.Validator.SelectedItem);
            else if (_valueType.Type == typeof(Guid))
                SetValue(Picker.Validator.SelectedID);
            else if (_valueType.Type == typeof(SceneReference))
                SetValue(new SceneReference(Picker.Validator.SelectedID));
            else if (_valueType.Type == typeof(string))
                SetValue(Picker.Validator.SelectedPath);
            else if (_valueType.Type.Name == typeof(JsonAssetReference<>).Name)
            {
                var value = Values[0];
                value.GetType().GetField("Asset").SetValue(value, Picker.Validator.SelectedAsset as JsonAsset);
                SetValue(value);
            }
            else
                SetValue(Picker.Validator.SelectedAsset);
        }

        internal static void ApplyAssetReferenceAttribute(ValueContainer values, out float height, AssetPickerValidator validator)
        {
            height = 48;
            var attributes = values.GetAttributes();
            var assetReference = (AssetReferenceAttribute)attributes?.FirstOrDefault(x => x is AssetReferenceAttribute);
            if (assetReference != null)
            {
                if (assetReference.UseSmallPicker)
                    height = 32;
                if (string.IsNullOrEmpty(assetReference.TypeName))
                {
                }
                else if (assetReference.TypeName.Length > 1 && assetReference.TypeName[0] == '.')
                {
                    // Generic file picker
                    validator.AssetType = ScriptType.Null;
                    validator.FileExtension = assetReference.TypeName;
                }
                else
                {
                    var customType = TypeUtils.GetType(assetReference.TypeName);
                    if (customType != ScriptType.Null)
                        validator.AssetType = customType;
                    else if (!Content.Settings.GameSettings.OptionalPlatformSettings.Contains(assetReference.TypeName))
                        Debug.LogWarning(string.Format("Unknown asset type '{0}' to use for asset picker filter.", assetReference.TypeName));
                    else
                        validator.AssetType = ScriptType.Void;
                }
            }
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            var differentValues = HasDifferentValues;
            Picker.DifferentValues = differentValues;
            if (!differentValues)
            {
                _isRefreshing = true;
                var value = Values[0];
                if (value is AssetItem assetItem)
                    Picker.Validator.SelectedItem = assetItem;
                else if (value is Guid guid)
                    Picker.Validator.SelectedID = guid;
                else if (value is SceneReference sceneAsset)
                    Picker.Validator.SelectedItem = Editor.Instance.ContentDatabase.FindAsset(sceneAsset.ID);
                else if (value is string path)
                    Picker.Validator.SelectedPath = path;
                else if (value != null && value.GetType().Name == typeof(JsonAssetReference<>).Name)
                    Picker.Validator.SelectedAsset = value.GetType().GetField("Asset").GetValue(value) as JsonAsset;
                else
                    Picker.Validator.SelectedAsset = value as Asset;
                _isRefreshing = false;
            }
        }
    }

    /// <summary>
    /// Default implementation of the inspector used to edit reference to the files via path (absolute or relative to the project).
    /// </summary>
    /// <remarks>Supports editing reference to the asset via path using various containers: <see cref="Asset"/> or <see cref="AssetItem"/> or <see cref="System.String"/>.</remarks>
    public class FilePathEditor : CustomEditor
    {
        private sealed class TextBoxWithPicker : TextBox
        {
            private const float DropdownIconMargin = 3.0f;
            private const float DropdownIconSize = 12.0f;
            private Rectangle DropdownRect => new Rectangle(Width - DropdownIconSize - DropdownIconMargin, DropdownIconMargin, DropdownIconSize, DropdownIconSize);

            public Action ShowPicker;
            public Action<ContentItem> OnAssetDropped;
            
            private DragItems _dragItems;
            private DragHandlers _dragHandlers;
            private bool _hasValidDragOver;
            private Func<ContentItem, bool> _validate;

            public void SetValidationMethod(Func<ContentItem, bool> validate)
            {
                _validate = validate;
            }

            public override void Draw()
            {
                base.Draw();

                var style = FlaxEngine.GUI.Style.Current;
                var dropdownRect = DropdownRect;
                Render2D.DrawSprite(style.ArrowDown, dropdownRect, Enabled ? (DropdownRect.Contains(PointFromWindow(RootWindow.MousePosition)) ? style.BorderSelected : style.Foreground) : style.ForegroundDisabled);
                
                // Check if drag is over
                if (IsDragOver && _hasValidDragOver)
                {
                    var bounds = new Rectangle(Float2.Zero, Size);
                    Render2D.FillRectangle(bounds, style.Selection);
                    Render2D.DrawRectangle(bounds, style.SelectionBorder);
                }
            }

            public override bool OnMouseDown(Float2 location, MouseButton button)
            {
                if (DropdownRect.Contains(ref location))
                {
                    Focus();
                    ShowPicker();
                    return true;
                }

                return base.OnMouseDown(location, button);
            }

            public override void OnMouseMove(Float2 location)
            {
                base.OnMouseMove(location);

                if (DropdownRect.Contains(ref location))
                    Cursor = CursorType.Default;
                else
                    Cursor = CursorType.IBeam;
            }

            protected override Rectangle TextRectangle
            {
                get
                {
                    var result = base.TextRectangle;
                    result.Size.X -= DropdownIconSize + DropdownIconMargin * 2;
                    return result;
                }
            }

            protected override Rectangle TextClipRectangle
            {
                get
                {
                    var result = base.TextClipRectangle;
                    result.Size.X -= DropdownIconSize + DropdownIconMargin * 2;
                    return result;
                }
            }

            private DragDropEffect DragEffect => _hasValidDragOver ? DragDropEffect.Move : DragDropEffect.None;

            /// <inheritdoc />
            public override DragDropEffect OnDragEnter(ref Float2 location, DragData data)
            {
                base.OnDragEnter(ref location, data);

                // Ensure to have valid drag helpers (uses lazy init)
                if (_dragItems == null)
                    _dragItems = new DragItems(ValidateDragAsset);
                if (_dragHandlers == null)
                {
                    _dragHandlers = new DragHandlers
                    {
                        _dragItems,
                    };
                }

                _hasValidDragOver = _dragHandlers.OnDragEnter(data) != DragDropEffect.None;
                

                return DragEffect;
            }

            private bool ValidateDragAsset(ContentItem contentItem)
            {
                // Load or get asset
                return _validate?.Invoke(contentItem) ?? false;
            }

            /// <inheritdoc />
            public override DragDropEffect OnDragMove(ref Float2 location, DragData data)
            {
                base.OnDragMove(ref location, data);

                return DragEffect;
            }

            /// <inheritdoc />
            public override void OnDragLeave()
            {
                _hasValidDragOver = false;
                _dragHandlers.OnDragLeave();

                base.OnDragLeave();
            }

            /// <inheritdoc />
            public override DragDropEffect OnDragDrop(ref Float2 location, DragData data)
            {
                var result = DragEffect;

                base.OnDragDrop(ref location, data);
                
                if (_dragItems.HasValidDrag)
                {
                    OnAssetDropped(_dragItems.Objects[0]);
                }

                return result;
            }
        }

        private TextBoxWithPicker _textBox;
        private AssetPickerValidator _validator;
        private bool _isRefreshing;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            if (HasDifferentTypes)
                return;

            _validator = new AssetPickerValidator(ScriptType.Null);
            _textBox = layout.Custom<TextBoxWithPicker>().CustomControl;
            _textBox.ShowPicker = OnShowPicker;
            _textBox.OnAssetDropped = OnItemDropped;
            _textBox.EditEnd += OnEditEnd;
            _textBox.SetValidationMethod(_validator.IsValid);
            AssetRefEditor.ApplyAssetReferenceAttribute(Values, out _, _validator);
        }

        private void OnItemDropped(ContentItem item)
        {
            SetPickerPath(item);
        }

        private void OnShowPicker()
        {
            if (_validator.AssetType != ScriptType.Null)
                AssetSearchPopup.Show(_textBox, _textBox.BottomLeft, _validator.IsValid, SetPickerPath);
            else
                ContentSearchPopup.Show(_textBox, _textBox.BottomLeft, _validator.IsValid, SetPickerPath);
        }

        private void SetPickerPath(ContentItem item)
        {
            var path = Utilities.Utils.ToPathProject(item.Path);
            SetPath(path);

            _isRefreshing = true;
            _textBox.Defocus();
            _textBox.Text = path;
            _isRefreshing = false;

            _textBox.RootWindow.Focus();
            _textBox.Focus();
        }

        private void OnEditEnd()
        {
            SetPath(_textBox.Text);
        }

        private string GetPath()
        {
            var value = Values[0];
            if (value is AssetItem assetItem)
                return Utilities.Utils.ToPathProject(assetItem.Path);
            if (value is Asset asset)
                return Utilities.Utils.ToPathProject(asset.Path);
            if (value is string str)
                return str;
            return null;
        }

        private void SetPath(string path)
        {
            if (_isRefreshing)
                return;
            var value = Values[0];
            if (value is AssetItem)
                SetValue(Editor.Instance.ContentDatabase.Find(Utilities.Utils.ToPathAbsolute(path)));
            else if (value is Asset)
                SetValue(FlaxEngine.Content.LoadAsync(path));
            else if (value is string)
                SetValue(path);
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            _isRefreshing = true;
            _textBox.Text = HasDifferentValues ? "Multiple Values" : GetPath();
            _isRefreshing = false;
        }

        /// <inheritdoc />
        protected override void Deinitialize()
        {
            if (_validator != null)
            {
                _validator.OnDestroy();
                _validator = null;
            }

            base.Deinitialize();
        }
    }
}
