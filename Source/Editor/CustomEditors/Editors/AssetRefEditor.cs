// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Linq;
using FlaxEditor.Content;
using FlaxEditor.GUI;
using FlaxEditor.Scripting;
using FlaxEngine;
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

            float height = 48;
            var attributes = Values.GetAttributes();
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
                    assetType = ScriptType.Null;
                    Picker.Validator.FileExtension = assetReference.TypeName;
                }
                else
                {
                    var customType = TypeUtils.GetType(assetReference.TypeName);
                    if (customType != ScriptType.Null)
                        assetType = customType;
                    else if (!Content.Settings.GameSettings.OptionalPlatformSettings.Contains(assetReference.TypeName))
                        Debug.LogWarning(string.Format("Unknown asset type '{0}' to use for asset picker filter.", assetReference.TypeName));
                    else
                        assetType = ScriptType.Void;
                }
            }

            Picker.Validator.AssetType = assetType;
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

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            if (!HasDifferentValues)
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
}
