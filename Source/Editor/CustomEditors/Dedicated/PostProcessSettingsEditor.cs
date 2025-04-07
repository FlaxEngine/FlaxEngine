// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using FlaxEditor.CustomEditors.Editors;
using FlaxEditor.CustomEditors.GUI;
using FlaxEditor.Scripting;
using FlaxEngine;

namespace FlaxEditor.CustomEditors.Dedicated
{
    /// <summary>
    /// Base class for all post process settings structures editors.
    /// </summary>
    abstract class PostProcessSettingsEditor : GenericEditor
    {
        private List<CheckablePropertyNameLabel> _labels;

        protected abstract int OverrideFlags { get; set; }

        /// <inheritdoc />
        public override DisplayStyle Style => DisplayStyle.InlineIntoParent;

        /// <inheritdoc />
        protected override List<ItemInfo> GetItemsForType(ScriptType type)
        {
            // Show structure properties
            return GetItemsForType(type, true, true);
        }

        /// <inheritdoc />
        protected override void SpawnProperty(LayoutElementsContainer itemLayout, ValueContainer itemValues, ItemInfo item)
        {
            var setting = item.Info.GetAttribute<PostProcessSettingAttribute>();
            if (setting == null)
            {
                base.SpawnProperty(itemLayout, itemValues, item);
                return;
            }

            EvaluateVisibleIf(itemLayout, item, GetLabelIndex(itemLayout, item));

            // Add labels with a check box
            var label = new CheckablePropertyNameLabel(item.DisplayName);
            label.CheckBox.Tag = setting.Bit;
            label.CheckChanged += CheckBoxOnCheckChanged;
            _labels.Add(label);
            itemLayout.Property(label, itemValues, item.OverrideEditor, item.TooltipText);
        }

        private void CheckBoxOnCheckChanged(CheckablePropertyNameLabel label)
        {
            if (IsSetBlocked)
                return;

            var bit = (int)label.CheckBox.Tag;
            var overrideFlags = OverrideFlags;
            if (label.CheckBox.Checked)
                overrideFlags |= bit;
            else
                overrideFlags &= ~bit;
            OverrideFlags = overrideFlags;
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            // Update all labels checkboxes
            var overrideFlags = OverrideFlags;
            for (int i = 0; i < _labels.Count; i++)
            {
                var bit = (int)_labels[i].CheckBox.Tag;
                _labels[i].CheckBox.Checked = (overrideFlags & bit) != 0;
            }
        }

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            _labels = new List<CheckablePropertyNameLabel>(32);

            base.Initialize(layout);
        }

        /// <inheritdoc />
        protected override void Deinitialize()
        {
            _labels.Clear();
            _labels = null;

            base.Deinitialize();
        }
    }

    /// <summary>
    /// Editor for <see cref="AmbientOcclusionSettings"/> type.
    /// </summary>
    [CustomEditor(typeof(AmbientOcclusionSettings)), DefaultEditor]
    sealed class AmbientOcclusionSettingsEditor : PostProcessSettingsEditor
    {
        /// <inheritdoc />
        protected override int OverrideFlags
        {
            get => (int)((AmbientOcclusionSettings)Values[0]).OverrideFlags;
            set
            {
                var settings = (AmbientOcclusionSettings)Values[0];
                settings.OverrideFlags = (AmbientOcclusionSettingsOverride)value;
                SetValue(settings);
            }
        }
    }

    /// <summary>
    /// Editor for <see cref="GlobalIlluminationSettings"/> type.
    /// </summary>
    [CustomEditor(typeof(GlobalIlluminationSettings)), DefaultEditor]
    sealed class GlobalIlluminationSettingsEditor : PostProcessSettingsEditor
    {
        /// <inheritdoc />
        protected override int OverrideFlags
        {
            get => (int)((GlobalIlluminationSettings)Values[0]).OverrideFlags;
            set
            {
                var settings = (GlobalIlluminationSettings)Values[0];
                settings.OverrideFlags = (GlobalIlluminationSettingsOverride)value;
                SetValue(settings);
            }
        }
    }

    /// <summary>
    /// Editor for <see cref="BloomSettings"/> type.
    /// </summary>
    [CustomEditor(typeof(BloomSettings)), DefaultEditor]
    sealed class BloomSettingsEditor : PostProcessSettingsEditor
    {
        /// <inheritdoc />
        protected override int OverrideFlags
        {
            get => (int)((BloomSettings)Values[0]).OverrideFlags;
            set
            {
                var settings = (BloomSettings)Values[0];
                settings.OverrideFlags = (BloomSettingsOverride)value;
                SetValue(settings);
            }
        }
    }

    /// <summary>
    /// Editor for <see cref="ToneMappingSettings"/> type.
    /// </summary>
    [CustomEditor(typeof(ToneMappingSettings)), DefaultEditor]
    sealed class ToneMappingSettingsEditor : PostProcessSettingsEditor
    {
        /// <inheritdoc />
        protected override int OverrideFlags
        {
            get => (int)((ToneMappingSettings)Values[0]).OverrideFlags;
            set
            {
                var settings = (ToneMappingSettings)Values[0];
                settings.OverrideFlags = (ToneMappingSettingsOverride)value;
                SetValue(settings);
            }
        }
    }

    /// <summary>
    /// Editor for <see cref="ColorGradingSettings"/> type.
    /// </summary>
    [CustomEditor(typeof(ColorGradingSettings)), DefaultEditor]
    sealed class ColorGradingSettingsEditor : PostProcessSettingsEditor
    {
        /// <inheritdoc />
        protected override int OverrideFlags
        {
            get => (int)((ColorGradingSettings)Values[0]).OverrideFlags;
            set
            {
                var settings = (ColorGradingSettings)Values[0];
                settings.OverrideFlags = (ColorGradingSettingsOverride)value;
                SetValue(settings);
            }
        }
    }

    /// <summary>
    /// Editor for <see cref="EyeAdaptationSettings"/> type.
    /// </summary>
    [CustomEditor(typeof(EyeAdaptationSettings)), DefaultEditor]
    sealed class EyeAdaptationSettingsEditor : PostProcessSettingsEditor
    {
        /// <inheritdoc />
        protected override int OverrideFlags
        {
            get => (int)((EyeAdaptationSettings)Values[0]).OverrideFlags;
            set
            {
                var settings = (EyeAdaptationSettings)Values[0];
                settings.OverrideFlags = (EyeAdaptationSettingsOverride)value;
                SetValue(settings);
            }
        }
    }

    /// <summary>
    /// Editor for <see cref="CameraArtifactsSettings"/> type.
    /// </summary>
    [CustomEditor(typeof(CameraArtifactsSettings)), DefaultEditor]
    sealed class CameraArtifactsSettingsEditor : PostProcessSettingsEditor
    {
        /// <inheritdoc />
        protected override int OverrideFlags
        {
            get => (int)((CameraArtifactsSettings)Values[0]).OverrideFlags;
            set
            {
                var settings = (CameraArtifactsSettings)Values[0];
                settings.OverrideFlags = (CameraArtifactsSettingsOverride)value;
                SetValue(settings);
            }
        }
    }

    /// <summary>
    /// Editor for <see cref="LensFlaresSettings"/> type.
    /// </summary>
    [CustomEditor(typeof(LensFlaresSettings)), DefaultEditor]
    sealed class LensFlaresSettingsEditor : PostProcessSettingsEditor
    {
        /// <inheritdoc />
        protected override int OverrideFlags
        {
            get => (int)((LensFlaresSettings)Values[0]).OverrideFlags;
            set
            {
                var settings = (LensFlaresSettings)Values[0];
                settings.OverrideFlags = (LensFlaresSettingsOverride)value;
                SetValue(settings);
            }
        }
    }

    /// <summary>
    /// Editor for <see cref="AmbientOcclusionSettings"/> type.
    /// </summary>
    [CustomEditor(typeof(DepthOfFieldSettings)), DefaultEditor]
    sealed class DepthOfFieldSettingsEditor : PostProcessSettingsEditor
    {
        /// <inheritdoc />
        protected override int OverrideFlags
        {
            get => (int)((DepthOfFieldSettings)Values[0]).OverrideFlags;
            set
            {
                var settings = (DepthOfFieldSettings)Values[0];
                settings.OverrideFlags = (DepthOfFieldSettingsOverride)value;
                SetValue(settings);
            }
        }
    }

    /// <summary>
    /// Editor for <see cref="MotionBlurSettings"/> type.
    /// </summary>
    [CustomEditor(typeof(MotionBlurSettings)), DefaultEditor]
    sealed class MotionBlurSettingsEditor : PostProcessSettingsEditor
    {
        /// <inheritdoc />
        protected override int OverrideFlags
        {
            get => (int)((MotionBlurSettings)Values[0]).OverrideFlags;
            set
            {
                var settings = (MotionBlurSettings)Values[0];
                settings.OverrideFlags = (MotionBlurSettingsOverride)value;
                SetValue(settings);
            }
        }
    }

    /// <summary>
    /// Editor for <see cref="ScreenSpaceReflectionsSettings"/> type.
    /// </summary>
    [CustomEditor(typeof(ScreenSpaceReflectionsSettings)), DefaultEditor]
    sealed class ScreenSpaceReflectionsSettingsEditor : PostProcessSettingsEditor
    {
        /// <inheritdoc />
        protected override int OverrideFlags
        {
            get => (int)((ScreenSpaceReflectionsSettings)Values[0]).OverrideFlags;
            set
            {
                var settings = (ScreenSpaceReflectionsSettings)Values[0];
                settings.OverrideFlags = (ScreenSpaceReflectionsSettingsOverride)value;
                SetValue(settings);
            }
        }
    }

    /// <summary>
    /// Editor for <see cref="AntiAliasingSettings"/> type.
    /// </summary>
    [CustomEditor(typeof(AntiAliasingSettings)), DefaultEditor]
    sealed class AntiAliasingSettingsEditor : PostProcessSettingsEditor
    {
        /// <inheritdoc />
        protected override int OverrideFlags
        {
            get => (int)((AntiAliasingSettings)Values[0]).OverrideFlags;
            set
            {
                var settings = (AntiAliasingSettings)Values[0];
                settings.OverrideFlags = (AntiAliasingSettingsOverride)value;
                SetValue(settings);
            }
        }
    }

    /// <summary>
    /// Editor for <see cref="PostFxMaterialsSettings"/> type.
    /// </summary>
    [CustomEditor(typeof(PostFxMaterialsSettings)), DefaultEditor]
    sealed class PostFxMaterialsSettingsEditor : PostProcessSettingsEditor
    {
        /// <inheritdoc />
        protected override int OverrideFlags
        {
            get => 0;
            set { }
        }
    }
}
