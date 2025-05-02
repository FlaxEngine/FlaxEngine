// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using FlaxEditor.CustomEditors.Editors;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.Tools;

namespace FlaxEngine.Tools
{
    partial class AudioTool
    {
        partial struct Options
        {
            private bool ShowBtiDepth => Format != AudioFormat.Vorbis;
        }
    }
}

namespace FlaxEditor.CustomEditors.Dedicated
{
    /// <summary>
    /// Custom editor for <see cref="FlaxEngine.Tools.AudioTool.Options"/>.
    /// </summary>
    [CustomEditor(typeof(FlaxEngine.Tools.AudioTool.Options)), DefaultEditor]
    public class AudioToolOptionsEditor : GenericEditor
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
    /// Proxy object to present audio import settings in <see cref="ImportFilesDialog"/>.
    /// </summary>
    [HideInEditor]
    public class AudioImportSettings
    {
        /// <summary>
        /// The settings data.
        /// </summary>
        [EditorDisplay(null, EditorDisplayAttribute.InlineStyle)]
        public AudioTool.Options Settings = AudioTool.Options.Default;
    }

    /// <summary>
    /// Audio asset import entry.
    /// </summary>
    /// <seealso cref="AssetImportEntry" />
    public partial class AudioImportEntry : AssetImportEntry
    {
        private AudioImportSettings _settings = new();

        /// <summary>
        /// Initializes a new instance of the <see cref="AudioImportEntry"/> class.
        /// </summary>
        /// <param name="request">The import request.</param>
        public AudioImportEntry(ref Request request)
        : base(ref request)
        {
            // Try to restore target asset Audio import options (useful for fast reimport)
            Editor.TryRestoreImportOptions(ref _settings.Settings, ResultUrl);
        }

        /// <inheritdoc />
        public override object Settings => _settings;

        /// <inheritdoc />
        public override bool TryOverrideSettings(object settings)
        {
            if (settings is AudioImportSettings s)
            {
                _settings.Settings = s.Settings;
                return true;
            }
            if (settings is AudioTool.Options o)
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
