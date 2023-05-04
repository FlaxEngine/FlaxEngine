// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.Marshalling;
using FlaxEngine;
using FlaxEngine.Interop;

// ReSharper disable InconsistentNaming

namespace FlaxEditor.Content.Import
{
    /// <summary>
    /// Texture format types.
    /// </summary>
    [HideInEditor]
    public enum TextureFormatType : byte
    {
        /// <summary>
        /// The unknown.
        /// </summary>
        Unknown = 0,

        /// <summary>
        /// The color with RGB channels.
        /// </summary>
        ColorRGB = 1,

        /// <summary>
        /// The color with RGBA channels.
        /// </summary>
        ColorRGBA = 2,

        /// <summary>
        /// The normal map (packed and compressed).
        /// </summary>
        NormalMap = 3,

        /// <summary>
        /// The gray scale (R channel).
        /// </summary>
        GrayScale = 4,

        /// <summary>
        /// The HDR color (RGBA channels).
        /// </summary>
        HdrRGBA = 5,

        /// <summary>
        /// The HDR color (RGB channels).
        /// </summary>
        HdrRGB = 6
    }

    /// <summary>
    /// Proxy object to present texture import settings in <see cref="ImportFilesDialog"/>.
    /// </summary>
    [HideInEditor]
    public class TextureImportSettings
    {
        /// <summary>
        /// A custom version of <see cref="TextureFormatType"/> for GUI.
        /// </summary>
        public enum CustomTextureFormatType
        {
            /// <summary>
            /// The color with RGB channels.
            /// </summary>
            ColorRGB = 1,

            /// <summary>
            /// The color with RGBA channels.
            /// </summary>
            ColorRGBA = 2,

            /// <summary>
            /// The normal map (packed and compressed).
            /// </summary>
            NormalMap = 3,

            /// <summary>
            /// The gray scale (R channel).
            /// </summary>
            GrayScale = 4,

            /// <summary>
            /// The HDR color (RGBA channels).
            /// </summary>
            HdrRGBA = 5,

            /// <summary>
            /// The HDR color (RGB channels).
            /// </summary>
            HdrRGB = 6
        }

        /// <summary>
        /// A custom set of max texture import sizes.
        /// </summary>
        public enum CustomMaxSizeType
        {
            /// <summary>
            /// The 32.
            /// </summary>
            _32 = 32,

            /// <summary>
            /// The 64.
            /// </summary>
            _64 = 64,

            /// <summary>
            /// The 128.
            /// </summary>
            _128 = 128,

            /// <summary>
            /// The 256.
            /// </summary>
            _256 = 256,

            /// <summary>
            /// The 512.
            /// </summary>
            _512 = 512,

            /// <summary>
            /// The 1024.
            /// </summary>
            _1024 = 1024,

            /// <summary>
            /// The 2048.
            /// </summary>
            _2048 = 2048,

            /// <summary>
            /// The 4096.
            /// </summary>
            _4096 = 4096,

            /// <summary>
            /// The 8192.
            /// </summary>
            _8192 = 8192,
        }

        /// <summary>
        /// Converts the maximum size to enum.
        /// </summary>
        /// <param name="f">The max size.</param>
        /// <returns>The converted enum.</returns>
        public static CustomMaxSizeType ConvertMaxSize(int f)
        {
            if (!Mathf.IsPowerOfTwo(f))
                f = Mathf.NextPowerOfTwo(f);

            FieldInfo[] fields = typeof(CustomMaxSizeType).GetFields();
            for (int i = 0; i < fields.Length; i++)
            {
                var field = fields[i];
                if (field.Name.Equals("value__"))
                    continue;

                if (f == (int)field.GetRawConstantValue())
                    return (CustomMaxSizeType)f;
            }

            return CustomMaxSizeType._8192;
        }

        /// <summary>
        /// The sprite info.
        /// </summary>
        [StructLayout(LayoutKind.Sequential)]
        public struct SpriteInfo
        {
            /// <summary>
            /// The sprite area.
            /// </summary>
            public Rectangle Area;

            /// <summary>
            /// The sprite name.
            /// </summary>
            public string Name;

            /// <summary>
            /// Initializes a new instance of the <see cref="SpriteInfo"/> struct.
            /// </summary>
            /// <param name="area">The area.</param>
            /// <param name="name">The name.</param>
            public SpriteInfo(Rectangle area, string name)
            {
                Area = area;
                Name = name;
            }
        }

        /// <summary>
        /// Texture format type
        /// </summary>
        [EditorOrder(0), DefaultValue(CustomTextureFormatType.ColorRGB), Tooltip("Texture import format type")]
        public CustomTextureFormatType Type { get; set; } = CustomTextureFormatType.ColorRGB;

        /// <summary>
        /// True if texture should be imported as a texture atlas resource
        /// </summary>
        [EditorOrder(10), DefaultValue(false), Tooltip("True if texture should be imported as a texture atlas (with sprites)")]
        public bool IsAtlas { get; set; }

        /// <summary>
        /// True if disable dynamic texture streaming
        /// </summary>
        [EditorOrder(20), DefaultValue(false), Tooltip("True if disable dynamic texture streaming")]
        public bool NeverStream { get; set; }

        /// <summary>
        /// Enables/disables texture data compression.
        /// </summary>
        [EditorOrder(30), DefaultValue(true), Tooltip("True if compress texture data")]
        public bool Compress { get; set; } = true;

        /// <summary>
        /// True if texture channels have independent data
        /// </summary>
        [EditorOrder(40), DefaultValue(false), Tooltip("True if texture channels have independent data (for compression methods)")]
        public bool IndependentChannels { get; set; }

        /// <summary>
        /// True if use sRGB format for texture data. Recommended for color maps and diffuse color textures.
        /// </summary>
        [EditorOrder(50), DefaultValue(false), EditorDisplay(null, "sRGB"), Tooltip("True if use sRGB format for texture data. Recommended for color maps and diffuse color textures.")]
        public bool sRGB { get; set; }

        /// <summary>
        /// True if generate mip maps chain for the texture.
        /// </summary>
        [EditorOrder(60), DefaultValue(true), Tooltip("True if generate mip maps chain for the texture")]
        public bool GenerateMipMaps { get; set; } = true;

        /// <summary>
        /// True if flip Y coordinate of the texture.
        /// </summary>
        [EditorOrder(65), DefaultValue(false), EditorDisplay(null, "Flip Y"), Tooltip("True if flip Y coordinate of the texture.")]
        public bool FlipY { get; set; } = false;

        /// <summary>
        /// The import texture scale.
        /// </summary>
        [EditorOrder(70), DefaultValue(1.0f), Tooltip("Texture scale. Default is 1.")]
        public float Scale { get; set; } = 1.0f;

        /// <summary>
        /// Maximum size of the texture (for both width and height).
        /// Higher resolution textures will be resized during importing process.
        /// </summary>
        [EditorOrder(80), DefaultValue(CustomMaxSizeType._8192), Tooltip("Maximum texture size (will be resized if need to)")]
        public CustomMaxSizeType MaxSize { get; set; } = CustomMaxSizeType._8192;

        /// <summary>
        /// True if resize texture on import. Use Size property to define texture width and height. Texture scale property will be ignored.
        /// </summary>
        [EditorOrder(90), DefaultValue(false), Tooltip("True if resize texture on import. Use Size property to define texture width and height. Texture scale property will be ignored.")]
        public bool Resize { get; set; } = false;

        /// <summary>
        /// Gets or sets the size of the imported texture. If Resize property is set to true then texture will be resized during the import to this value. Otherwise it will be ignored.
        /// </summary>
        [EditorOrder(100), VisibleIf("Resize"), DefaultValue(typeof(Int2), "1024,1024"), Tooltip("The size of the imported texture. If Resize property is set to true then texture will be resized during the import to this value. Otherwise it will be ignored.")]
        public Int2 Size { get; set; } = new Int2(1024, 1024);

        /// <summary>
        /// True if preserve alpha coverage in generated mips for alpha test reference. Scales mipmap alpha values to preserve alpha coverage based on an alpha test reference value.
        /// </summary>
        [EditorOrder(240), DefaultValue(false), Tooltip("Check to preserve alpha coverage in generated mips for alpha test reference. Scales mipmap alpha values to preserve alpha coverage based on an alpha test reference value.")]
        public bool PreserveAlphaCoverage { get; set; } = false;

        /// <summary>
        /// The reference value for the alpha coverage preserving.
        /// </summary>
        [EditorOrder(250), VisibleIf("PreserveAlphaCoverage"), DefaultValue(0.5f), Tooltip("The reference value for the alpha coverage preserving.")]
        public float PreserveAlphaCoverageReference { get; set; } = 0.5f;

        /// <summary>
        /// Texture group for streaming (negative if unused). See Streaming Settings.
        /// </summary>
        [CustomEditor(typeof(CustomEditors.Dedicated.TextureGroupEditor))]
        [EditorOrder(300), Tooltip("Texture group for streaming (negative if unused). See Streaming Settings.")]
        public int TextureGroup = -1;

        /// <summary>
        /// The sprites. Used to keep created sprites on sprite atlas reimport.
        /// </summary>
        [HideInEditor]
        public List<SpriteInfo> Sprites = new List<SpriteInfo>();

        [StructLayout(LayoutKind.Sequential)]
        [NativeMarshalling(typeof(InternalOptionsMarshaller))]
        internal struct InternalOptions
        {
            public TextureFormatType Type;
            public byte IsAtlas;
            public byte NeverStream;
            public byte Compress;
            public byte IndependentChannels;
            public byte sRGB;
            public byte GenerateMipMaps;
            public byte FlipY;
            public byte Resize;
            public byte PreserveAlphaCoverage;
            public float PreserveAlphaCoverageReference;
            public float Scale;
            public int MaxSize;
            public int TextureGroup;
            public Int2 Size;
            public Rectangle[] SpriteAreas;
            public string[] SpriteNames;
        }

        [CustomMarshaller(typeof(InternalOptions), MarshalMode.Default, typeof(InternalOptionsMarshaller))]
        internal static class InternalOptionsMarshaller
        {
            [StructLayout(LayoutKind.Sequential)]
            internal struct InternalOptionsNative
            {
                public byte Type;
                public byte IsAtlas;
                public byte NeverStream;
                public byte Compress;
                public byte IndependentChannels;
                public byte sRGB;
                public byte GenerateMipMaps;
                public byte FlipY;
                public byte Resize;
                public byte PreserveAlphaCoverage;
                public float PreserveAlphaCoverageReference;
                public float Scale;
                public int MaxSize;
                public int TextureGroup;
                public Int2 Size;
                public IntPtr SpriteAreas;
                public IntPtr SpriteNames;
            }

            internal static InternalOptions ConvertToManaged(InternalOptionsNative unmanaged) => ToManaged(unmanaged);
            internal static InternalOptionsNative ConvertToUnmanaged(InternalOptions managed) => ToNative(managed);

            internal static InternalOptions ToManaged(InternalOptionsNative managed)
            {
                return new InternalOptions()
                {
                    Type = (TextureFormatType)managed.Type,
                    IsAtlas = managed.IsAtlas,
                    NeverStream = managed.NeverStream,
                    Compress = managed.Compress,
                    IndependentChannels = managed.IndependentChannels,
                    sRGB = managed.sRGB,
                    GenerateMipMaps = managed.GenerateMipMaps,
                    FlipY = managed.FlipY,
                    Resize = managed.Resize,
                    PreserveAlphaCoverage = managed.PreserveAlphaCoverage,
                    PreserveAlphaCoverageReference = managed.PreserveAlphaCoverageReference,
                    Scale = managed.Scale,
                    MaxSize = managed.MaxSize,
                    TextureGroup = managed.TextureGroup,
                    Size = managed.Size,
                    SpriteAreas = managed.SpriteAreas != IntPtr.Zero ? ((ManagedArray)ManagedHandle.FromIntPtr(managed.SpriteAreas).Target).ToArray<Rectangle>() : null,
                    SpriteNames = managed.SpriteNames != IntPtr.Zero ? NativeInterop.GCHandleArrayToManagedArray<string>((ManagedArray)ManagedHandle.FromIntPtr(managed.SpriteNames).Target) : null,
                };
            }
            internal static InternalOptionsNative ToNative(InternalOptions managed)
            {
                return new InternalOptionsNative()
                {
                    Type = (byte)managed.Type,
                    IsAtlas = managed.IsAtlas,
                    NeverStream = managed.NeverStream,
                    Compress = managed.Compress,
                    IndependentChannels = managed.IndependentChannels,
                    sRGB = managed.sRGB,
                    GenerateMipMaps = managed.GenerateMipMaps,
                    FlipY = managed.FlipY,
                    Resize = managed.Resize,
                    PreserveAlphaCoverage = managed.PreserveAlphaCoverage,
                    PreserveAlphaCoverageReference = managed.PreserveAlphaCoverageReference,
                    Scale = managed.Scale,
                    MaxSize = managed.MaxSize,
                    TextureGroup = managed.TextureGroup,
                    Size = managed.Size,
                    SpriteAreas = managed.SpriteAreas?.Length > 0 ? ManagedHandle.ToIntPtr(NativeInterop.ManagedArrayToGCHandleWrappedArray(managed.SpriteAreas)) : IntPtr.Zero,
                    SpriteNames = managed.SpriteNames?.Length > 0 ? ManagedHandle.ToIntPtr(NativeInterop.ManagedArrayToGCHandleWrappedArray(managed.SpriteNames)) : IntPtr.Zero,
                };
            }
            internal static void Free(InternalOptionsNative unmanaged)
            {
            }
        }

        internal void ToInternal(out InternalOptions options)
        {
            options = new InternalOptions
            {
                Type = (TextureFormatType)(int)Type,
                IsAtlas = (byte)(IsAtlas ? 1 : 0),
                NeverStream = (byte)(NeverStream ? 1 : 0),
                Compress = (byte)(Compress ? 1 : 0),
                IndependentChannels = (byte)(IndependentChannels ? 1 : 0),
                sRGB = (byte)(sRGB ? 1 : 0),
                GenerateMipMaps = (byte)(GenerateMipMaps ? 1 : 0),
                FlipY = (byte)(FlipY ? 1 : 0),
                Resize = (byte)(Resize ? 1 : 0),
                PreserveAlphaCoverage = (byte)(PreserveAlphaCoverage ? 1 : 0),
                PreserveAlphaCoverageReference = PreserveAlphaCoverageReference,
                Scale = Scale,
                Size = Size,
                MaxSize = (int)MaxSize,
                TextureGroup = TextureGroup,
            };
            if (Sprites != null && Sprites.Count > 0)
            {
                int count = Sprites.Count;
                options.SpriteAreas = new Rectangle[count];
                options.SpriteNames = new string[count];
                for (int i = 0; i < count; i++)
                {
                    options.SpriteAreas[i] = Sprites[i].Area;
                    options.SpriteNames[i] = Sprites[i].Name;
                }
            }
            else
            {
                options.SpriteAreas = null;
                options.SpriteNames = null;
            }
        }

        internal void FromInternal(ref InternalOptions options)
        {
            Type = (CustomTextureFormatType)(int)options.Type;
            IsAtlas = options.IsAtlas != 0;
            NeverStream = options.NeverStream != 0;
            Compress = options.Compress != 0;
            IndependentChannels = options.IndependentChannels != 0;
            sRGB = options.sRGB != 0;
            GenerateMipMaps = options.GenerateMipMaps != 0;
            FlipY = options.FlipY != 0;
            Resize = options.Resize != 0;
            PreserveAlphaCoverage = options.PreserveAlphaCoverage != 0;
            PreserveAlphaCoverageReference = options.PreserveAlphaCoverageReference;
            Scale = options.Scale;
            MaxSize = ConvertMaxSize(options.MaxSize);
            TextureGroup = options.TextureGroup;
            Size = options.Size;
            if (options.SpriteAreas != null)
            {
                int spritesCount = options.SpriteAreas.Length;
                Sprites.Capacity = spritesCount;
                for (int i = 0; i < spritesCount; i++)
                {
                    Sprites.Add(new SpriteInfo(options.SpriteAreas[i], options.SpriteNames[i]));
                }
            }
        }

        /// <summary>
        /// Tries the restore the asset import options from the target resource file.
        /// </summary>
        /// <param name="options">The options.</param>
        /// <param name="assetPath">The asset path.</param>
        /// <returns>True settings has been restored, otherwise false.</returns>
        public static bool TryRestore(ref TextureImportSettings options, string assetPath)
        {
            if (TextureImportEntry.Internal_GetTextureImportOptions(assetPath, out var internalOptions))
            {
                // Restore settings
                options.FromInternal(ref internalOptions);
                return true;
            }
            return false;
        }
    }

    /// <summary>
    /// Texture asset import entry.
    /// </summary>
    /// <seealso cref="AssetImportEntry" />
    public partial class TextureImportEntry : AssetImportEntry
    {
        private TextureImportSettings _settings = new TextureImportSettings();

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
                _settings.Type = TextureImportSettings.CustomTextureFormatType.HdrRGBA;
                _settings.Compress = false;
            }
            else if (extension == ".hdr")
            {
                // HDR sky texture
                _settings.Type = TextureImportSettings.CustomTextureFormatType.HdrRGB;
            }
            else if (_settings.Type != TextureImportSettings.CustomTextureFormatType.ColorRGB)
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
                _settings.Type = TextureImportSettings.CustomTextureFormatType.NormalMap;
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
                _settings.Type = TextureImportSettings.CustomTextureFormatType.ColorRGB;
                _settings.sRGB = true;
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
                _settings.Type = TextureImportSettings.CustomTextureFormatType.GrayScale;
            }

            // Try to restore target asset texture import options (useful for fast reimport)
            TextureImportSettings.TryRestore(ref _settings, ResultUrl);
        }

        /// <inheritdoc />
        public override object Settings => _settings;

        /// <inheritdoc />
        public override bool TryOverrideSettings(object settings)
        {
            if (settings is TextureImportSettings o)
            {
                var sprites = o.Sprites ?? _settings.Sprites; // Preserve sprites if not specified to override
                _settings = o;
                _settings.Sprites = sprites;
                return true;
            }
            return false;
        }

        /// <inheritdoc />
        public override bool Import()
        {
            return Editor.Import(SourceUrl, ResultUrl, _settings);
        }

        #region Internal Calls

        [LibraryImport("FlaxEngine", EntryPoint = "TextureImportEntryInternal_GetTextureImportOptions", StringMarshalling = StringMarshalling.Custom, StringMarshallingCustomType = typeof(StringMarshaller))]
        [return: MarshalAs(UnmanagedType.U1)]
        internal static partial bool Internal_GetTextureImportOptions(string path, out TextureImportSettings.InternalOptions result);

        #endregion
    }
}
