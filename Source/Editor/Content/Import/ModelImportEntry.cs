// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using FlaxEditor.CustomEditors.Editors;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.Tools;

namespace FlaxEngine.Tools
{
    partial class ModelTool
    {
        partial struct Options
        {
            private bool ShowGeometry => Type == ModelType.Model || Type == ModelType.SkinnedModel || Type == ModelType.Prefab;
            private bool ShowModel => Type == ModelType.Model || Type == ModelType.Prefab;
            private bool ShowSkinnedModel => Type == ModelType.SkinnedModel || Type == ModelType.Prefab;
            private bool ShowAnimation => Type == ModelType.Animation || Type == ModelType.Prefab;
            private bool ShowRootMotion => ShowAnimation && RootMotion != RootMotionMode.None;
            private bool ShowSmoothingNormalsAngle => ShowGeometry && CalculateNormals;
            private bool ShowSmoothingTangentsAngle => ShowGeometry && CalculateTangents;
            private bool ShowFramesRange => ShowAnimation && Duration == AnimationDuration.Custom;
            private bool ShowSplitting => Type != ModelType.Prefab;
        }
    }
}

namespace FlaxEditor.CustomEditors.Dedicated
{
    /// <summary>
    /// Custom editor for <see cref="FlaxEngine.Tools.ModelTool.Options"/>.
    /// </summary>
    [CustomEditor(typeof(FlaxEngine.Tools.ModelTool.Options)), DefaultEditor]
    public class ModelToolOptionsEditor : GenericEditor
    {
        /// <inheritdoc />
        protected override List<ItemInfo> GetItemsForType(ScriptType type)
        {
            // Show both fields and properties
            return GetItemsForType(type, true, true);
        }
    }
}

namespace FlaxEditor.Content.Import
{
    /// <summary>
    /// Proxy object to present model import settings in <see cref="ImportFilesDialog"/>.
    /// </summary>
    [HideInEditor]
    public class ModelImportSettings
    {
        /// <summary>
        /// The settings data.
        /// </summary>
        [EditorDisplay(null, EditorDisplayAttribute.InlineStyle)]
        public ModelTool.Options Settings = ModelTool.Options.Default;
    }

    /// <summary>
    /// Model asset import entry.
    /// </summary>
    /// <seealso cref="AssetImportEntry" />
    public class ModelImportEntry : AssetImportEntry
    {
        private ModelImportSettings _settings = new();

        /// <summary>
        /// Initializes a new instance of the <see cref="ModelImportEntry"/> class.
        /// </summary>
        /// <param name="request">The import request.</param>
        public ModelImportEntry(ref Request request)
        : base(ref request)
        {
            // Try to restore target asset model import options (useful for fast reimport)
            Editor.TryRestoreImportOptions(ref _settings.Settings, ResultUrl);
        }

        /// <inheritdoc />
        public override object Settings => _settings;

        /// <inheritdoc />
        public override bool TryOverrideSettings(object settings)
        {
            if (settings is ModelImportSettings s)
            {
                _settings.Settings = s.Settings;
                return true;
            }
            if (settings is ModelTool.Options o)
            {
                _settings.Settings = o;
                return true;
            }
            return false;
        }

        /// <inheritdoc />
        public override bool Import()
        {
            return Editor.Import(SourceUrl, ResultUrl, _settings.Settings);
        }
    }
}
