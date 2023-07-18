// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

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
        private ScriptType _valueType;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            if (HasDifferentTypes)
                return;
            Picker = layout.Custom<AssetPicker>().CustomControl;

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
                    Picker.FileExtension = assetReference.TypeName;
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

            Picker.AssetType = assetType;
            Picker.Height = height;
            Picker.SelectedItemChanged += OnSelectedItemChanged;
        }

        private void OnSelectedItemChanged()
        {
            if (typeof(AssetItem).IsAssignableFrom(_valueType.Type))
                SetValue(Picker.SelectedItem);
            else if (_valueType.Type == typeof(Guid))
                SetValue(Picker.SelectedID);
            else if (_valueType.Type == typeof(SceneReference))
                SetValue(new SceneReference(Picker.SelectedID));
            else if (_valueType.Type == typeof(string))
                SetValue(Picker.SelectedPath);
            else
                SetValue(Picker.SelectedAsset);
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            if (!HasDifferentValues)
            {
                if (Values[0] is AssetItem assetItem)
                    Picker.SelectedItem = assetItem;
                else if (Values[0] is Guid guid)
                    Picker.SelectedID = guid;
                else if (Values[0] is SceneReference sceneAsset)
                    Picker.SelectedItem = Editor.Instance.ContentDatabase.FindAsset(sceneAsset.ID);
                else if (Values[0] is string path)
                    Picker.SelectedPath = path;
                else
                    Picker.SelectedAsset = Values[0] as Asset;
            }
        }
    }
}
