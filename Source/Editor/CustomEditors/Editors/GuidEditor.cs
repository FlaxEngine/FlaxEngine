// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Linq;
using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.GUI;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.Utilities;

namespace FlaxEditor.CustomEditors.Editors
{
    /// <summary>
    /// Default implementation of the inspector used to edit Guid properties.
    /// </summary>
    [CustomEditor(typeof(Guid)), DefaultEditor]
    public sealed class GuidEditor : CustomEditor
    {
        private TextBoxElement _element;
        private AssetPicker _picker;
        private bool _isReference;
        private bool _isRefreshing;

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.Inline;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            var attributes = Values.GetAttributes();
            var assetReference = (AssetReferenceAttribute)attributes?.FirstOrDefault(x => x is AssetReferenceAttribute);
            if (assetReference != null)
            {
                _picker = layout.Custom<AssetPicker>().CustomControl;
                ScriptType assetType = new ScriptType();

                float height = 48;
                if (assetReference.UseSmallPicker)
                    height = 36;

                if (string.IsNullOrEmpty(assetReference.TypeName))
                {
                    assetType = ScriptType.Void;
                }
                else if (assetReference.TypeName.Length > 1 && assetReference.TypeName[0] == '.')
                {
                    // Generic file picker
                    assetType = ScriptType.Null;
                    _picker.Validator.FileExtension = assetReference.TypeName;
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

                _picker.Validator.AssetType = assetType;
                _picker.Height = height;
                _picker.SelectedItemChanged += OnSelectedItemChanged;
                _isReference = true;
            }
            else
            {
                _element = layout.TextBox();
                _element.TextBox.EditEnd += OnEditEnd;
            }
        }

        private void OnSelectedItemChanged()
        {
            if (_isRefreshing)
                return;
            SetValue(_picker.Validator.SelectedID);
        }

        private void OnEditEnd()
        {
            if (Guid.TryParse(_element.Text, out Guid value))
            {
                SetValue(value);
            }
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();
            _isRefreshing = true;
            if (HasDifferentValues)
            {
                if (_isReference)
                {
                    // Not supported
                }
                else
                {
                    _element.TextBox.Text = string.Empty;
                    _element.TextBox.WatermarkText = "Different values";
                }
            }
            else
            {
                if (_isReference)
                {
                    _picker.Validator.SelectedID = (Guid)Values[0];
                }
                else
                {
                    _element.TextBox.Text = ((Guid)Values[0]).ToString("D");
                    _element.TextBox.WatermarkText = string.Empty;
                }
            }
            _isRefreshing = false;
        }
    }
}
