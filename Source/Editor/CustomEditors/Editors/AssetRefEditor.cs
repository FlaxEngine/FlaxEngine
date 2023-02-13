// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System;
using System.Linq;
using FlaxEditor.Content;
using FlaxEditor.GUI;
using FlaxEditor.Scripting;
using FlaxEngine;

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
        private AssetPicker _picker;
        private ScriptType _valueType;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            if (HasDifferentTypes)
                return;
            _picker = layout.Custom<AssetPicker>().CustomControl;

            _valueType = Values.Type.Type != typeof(object) || Values[0] == null ? Values.Type : TypeUtils.GetObjectType(Values[0]);
            var assetType = _valueType;
            if (assetType == typeof(string))
                assetType = new ScriptType(typeof(Asset));

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
                    _picker.FileExtension = assetReference.TypeName;
                }
                else
                {
                    var customType = TypeUtils.GetType(assetReference.TypeName);
                    if (customType != ScriptType.Null)
                        assetType = customType;
                    else if (!Content.Settings.GameSettings.OptionalPlatformSettings.Contains(assetReference.TypeName))
                        Debug.LogWarning(string.Format("Unknown asset type '{0}' to use for asset picker filter.", assetReference.TypeName));
                }
            }

            _picker.AssetType = assetType;
            _picker.Height = height;
            _picker.SelectedItemChanged += OnSelectedItemChanged;
        }

        private void OnSelectedItemChanged()
        {
            if (typeof(AssetItem).IsAssignableFrom(_valueType.Type))
                SetValue(_picker.SelectedItem);
            else if (_valueType.Type == typeof(Guid))
                SetValue(_picker.SelectedID);
            else if (_valueType.Type == typeof(SceneReference))
                SetValue(new SceneReference(_picker.SelectedID));
            else if (_valueType.Type == typeof(string))
                SetValue(_picker.SelectedPath);
            else
                SetValue(_picker.SelectedAsset);
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            if (!HasDifferentValues)
            {
                if (Values[0] is AssetItem assetItem)
                    _picker.SelectedItem = assetItem;
                else if (Values[0] is Guid guid)
                    _picker.SelectedID = guid;
                else if (Values[0] is SceneReference sceneAsset)
                    _picker.SelectedItem = Editor.Instance.ContentDatabase.FindAsset(sceneAsset.ID);
                else if (Values[0] is string path)
                    _picker.SelectedPath = path;
                else
                    _picker.SelectedAsset = Values[0] as Asset;
            }
        }
    }
}
