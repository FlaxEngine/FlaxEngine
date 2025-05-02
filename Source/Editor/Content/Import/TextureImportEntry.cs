// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.ComponentModel;
using System.Reflection;
using FlaxEditor.CustomEditors.Editors;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.Tools;

namespace FlaxEngine.Tools
{
    partial class TextureTool
    {
        partial struct Options
        {
#pragma warning disable CS1591
            public enum CustomMaxSizes
            {
                _32 = 32,
                _64 = 64,
                _128 = 128,
                _256 = 256,
                _512 = 512,
                _1024 = 1024,
                _2048 = 2048,
                _4096 = 4096,
                _8192 = 8192,
                _16384 = 16384,
            }
#pragma warning restore CS1591

            /// <summary>
            /// The size of the imported texture. If Resize property is set to true then texture will be resized during the import to this value. Otherwise it will be ignored.
            /// </summary>
            [EditorOrder(110), VisibleIf(nameof(Resize)), DefaultValue(typeof(Int2), "1024,1024")]
            public Int2 Size
            {
                get => new Int2(SizeX, SizeY);
                set
                {
                    SizeX = value.X;
                    SizeY = value.Y;
                }
            }

            /// <summary>
            /// Maximum size of the texture (for both width and height). Higher resolution textures will be resized during importing process.
            /// </summary>
            [EditorOrder(90), DefaultValue(CustomMaxSizes._8192), EditorDisplay(null, "Max Size")]
            public CustomMaxSizes CustomMaxSize
            {
                get
                {
                    var value = MaxSize;
                    if (!Mathf.IsPowerOfTwo(value))
                        value = Mathf.NextPowerOfTwo(value);
                    FieldInfo[] fields = typeof(CustomMaxSizes).GetFields();
                    for (int i = 0; i < fields.Length; i++)
                    {
                        var @field = fields[i];
                        if (@field.Name.Equals("value__"))
                            continue;
                        if (value == (int)@field.GetRawConstantValue())
                            return (CustomMaxSizes)value;
                    }
                    return CustomMaxSizes._8192;
                }
                set => MaxSize = (int)value;
            }
        }
    }
}

namespace FlaxEditor.CustomEditors.Dedicated
{
    /// <summary>
    /// Custom editor for <see cref="FlaxEngine.Tools.TextureTool.Options"/>.
    /// </summary>
    [CustomEditor(typeof(FlaxEngine.Tools.TextureTool.Options)), DefaultEditor]
    public class TextureToolOptionsEditor : GenericEditor
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
    /// Proxy object to present texture import settings in <see cref="ImportFilesDialog"/>.
    /// </summary>
    [HideInEditor]
    public class TextureImportSettings
    {
        /// <summary>
        /// The settings data.
        /// </summary>
        [EditorDisplay(null, EditorDisplayAttribute.InlineStyle)]
        public TextureTool.Options Settings = TextureTool.Options.Default;
    }

    /// <summary>
    /// Texture asset import entry.
    /// </summary>
    /// <seealso cref="AssetImportEntry" />
    public class TextureImportEntry : AssetImportEntry
    {
        private TextureImportSettings _settings = new();

        /// <summary>
        /// Initializes a new instance of the <see cref="TextureImportEntry"/> class.
        /// </summary>
        /// <param name="request">The import request.</param>
        public TextureImportEntry(ref Request request)
        : base(ref request)
        {
            // Try to guess format type based on file name
            var snl = System.IO.Path.GetFileNameWithoutExtension(SourceUrl).ToLower();
            var extension = System.IO.Path.GetExtension(SourceUrl).ToLower();
            if (extension == ".raw")
            {
                // Raw image data in 16bit gray-scale, preserve the quality
                _settings.Settings.Type = TextureFormatType.HdrRGBA;
                _settings.Settings.Compress = false;
            }
            else if (extension == ".exr")
            {
                // HDR image
                _settings.Settings.Type = TextureFormatType.HdrRGBA;
            }
            else if (extension == ".hdr")
            {
                // HDR sky texture
                _settings.Settings.Type = TextureFormatType.HdrRGB;
            }
            else if (_settings.Settings.Type != TextureFormatType.ColorRGB)
            {
                // Skip checking
            }
            else if (snl.EndsWith("_n")
                     || snl.EndsWith("nrm")
                     || snl.EndsWith("nm")
                     || snl.EndsWith("norm")
                     || snl.Contains("normal")
                     || snl.EndsWith("normals"))
            {
                // Normal map
                _settings.Settings.Type = TextureFormatType.NormalMap;
            }
            else if (snl.EndsWith("_d")
                     || snl.Contains("diffuse")
                     || snl.Contains("diff")
                     || snl.Contains("color")
                     || snl.Contains("_col")
                     || snl.Contains("basecolor")
                     || snl.Contains("albedo"))
            {
                // Albedo or diffuse map
                _settings.Settings.Type = TextureFormatType.ColorRGB;
                _settings.Settings.sRGB = true;
            }
            else if (snl.EndsWith("ao")
                     || snl.EndsWith("ambientocclusion")
                     || snl.EndsWith("gloss")
                     || snl.EndsWith("_r")
                     || snl.EndsWith("_displ")
                     || snl.EndsWith("_disp")
                     || snl.EndsWith("roughness")
                     || snl.EndsWith("_rgh")
                     || snl.EndsWith("_met")
                     || snl.EndsWith("metalness")
                     || snl.EndsWith("displacement")
                     || snl.EndsWith("spec")
                     || snl.EndsWith("specular")
                     || snl.EndsWith("occlusion")
                     || snl.EndsWith("height")
                     || snl.EndsWith("heights")
                     || snl.EndsWith("cavity")
                     || snl.EndsWith("metalic")
                     || snl.EndsWith("metallic"))
            {
                // Glossiness, metalness, ambient occlusion, displacement, height, cavity or specular
                _settings.Settings.Type = TextureFormatType.GrayScale;
            }

            // Try to restore target asset texture import options (useful for fast reimport)
            Editor.TryRestoreImportOptions(ref _settings.Settings, ResultUrl);
        }

        /// <inheritdoc />
        public override object Settings => _settings;

        /// <inheritdoc />
        public override bool TryOverrideSettings(object settings)
        {
            if (settings is TextureImportSettings s)
            {
                var sprites = s.Settings.Sprites ?? _settings.Settings.Sprites; // Preserve sprites if not specified to override
                _settings.Settings = s.Settings;
                _settings.Settings.Sprites = sprites;
                return true;
            }
            if (settings is TextureTool.Options o)
            {
                var sprites = o.Sprites ?? _settings.Settings.Sprites; // Preserve sprites if not specified to override
                _settings.Settings = o;
                _settings.Settings.Sprites = sprites;
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
