// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using System.ComponentModel;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using FlaxEngine;

namespace FlaxEditor.Content.Import
{
    /// <summary>
    /// Proxy object to present audio import settings in <see cref="ImportFilesDialog"/>.
    /// </summary>
    public class AudioImportSettings
    {
        /// <summary>
        /// A custom set of bit depth audio import sizes.
        /// </summary>
        public enum CustomBitDepth
        {
            /// <summary>
            /// The 8.
            /// </summary>
            _8 = 8,

            /// <summary>
            /// The 16.
            /// </summary>
            _16 = 16,

            /// <summary>
            /// The 24.
            /// </summary>
            _24 = 24,

            /// <summary>
            /// The 32.
            /// </summary>
            _32 = 32,
        }

        /// <summary>
        /// Converts the bit depth to enum.
        /// </summary>
        /// <param name="f">The bit depth.</param>
        /// <returns>The converted enum.</returns>
        public static CustomBitDepth ConvertBitDepth(int f)
        {
            FieldInfo[] fields = typeof(CustomBitDepth).GetFields();
            for (int i = 0; i < fields.Length; i++)
            {
                var field = fields[i];
                if (field.Name.Equals("value__"))
                    continue;

                if (f == (int)field.GetRawConstantValue())
                    return (CustomBitDepth)f;
            }

            return CustomBitDepth._16;
        }

        /// <summary>
        /// The audio data format to import the audio clip as. 
        /// </summary>
        [EditorOrder(10), DefaultValue(AudioFormat.Vorbis), Tooltip("The audio data format to import the audio clip as.")]
        public AudioFormat Format { get; set; } = AudioFormat.Vorbis;

        /// <summary>
        /// The audio data compression quality. Used only if target format is using compression. Value 0 means the smallest size, value 1 means the best quality.
        /// </summary>
        [EditorOrder(15), DefaultValue(0.4f), Limit(0, 1, 0.01f), Tooltip("The audio data compression quality. Used only if target format is using compression. Value 0 means the smallest size, value 1 means the best quality.")]
        public float CompressionQuality { get; set; } = 0.4f;

        /// <summary>
        /// Disables dynamic audio streaming. The whole clip will be loaded into the memory. Useful for small clips (eg. gunfire sounds).
        /// </summary>
        [EditorOrder(20), DefaultValue(false), Tooltip("Disables dynamic audio streaming. The whole clip will be loaded into the memory. Useful for small clips (eg. gunfire sounds).")]
        public bool DisableStreaming { get; set; } = false;

        /// <summary>
        /// Checks should the clip be played as spatial (3D) audio or as normal audio. 3D audio is stored in Mono format.
        /// </summary>
        [EditorOrder(30), DefaultValue(false), EditorDisplay(null, "Is 3D"), Tooltip("Checks should the clip be played as spatial (3D) audio or as normal audio. 3D audio is stored in Mono format.")]
        public bool Is3D { get; set; } = false;

        /// <summary>
        /// The size of a single sample in bits. The clip will be converted to this bit depth on import.
        /// </summary>
        [EditorOrder(40), DefaultValue(CustomBitDepth._16), Tooltip("The size of a single sample in bits. The clip will be converted to this bit depth on import.")]
        public CustomBitDepth BitDepth { get; set; } = CustomBitDepth._16;

        [StructLayout(LayoutKind.Sequential)]
        internal struct InternalOptions
        {
            public AudioFormat Format;
            public byte DisableStreaming;
            public byte Is3D;
            public int BitDepth;
            public float Quality;
        }

        internal void ToInternal(out InternalOptions options)
        {
            options = new InternalOptions
            {
                Format = Format,
                DisableStreaming = (byte)(DisableStreaming ? 1 : 0),
                Is3D = (byte)(Is3D ? 1 : 0),
                Quality = CompressionQuality,
                BitDepth = (int)BitDepth,
            };
        }

        internal void FromInternal(ref InternalOptions options)
        {
            Format = options.Format;
            DisableStreaming = options.DisableStreaming != 0;
            Is3D = options.Is3D != 0;
            CompressionQuality = options.Quality;
            BitDepth = ConvertBitDepth(options.BitDepth);
        }

        /// <summary>
        /// Tries the restore the asset import options from the target resource file.
        /// </summary>
        /// <param name="options">The options.</param>
        /// <param name="assetPath">The asset path.</param>
        /// <returns>True settings has been restored, otherwise false.</returns>
        public static bool TryRestore(ref AudioImportSettings options, string assetPath)
        {
            if (AudioImportEntry.Internal_GetAudioImportOptions(assetPath, out var internalOptions))
            {
                // Restore settings
                options.FromInternal(ref internalOptions);
                return true;
            }

            return false;
        }
    }

    /// <summary>
    /// Audio asset import entry.
    /// </summary>
    /// <seealso cref="AssetImportEntry" />
    public class AudioImportEntry : AssetImportEntry
    {
        private AudioImportSettings _settings = new AudioImportSettings();

        /// <summary>
        /// Initializes a new instance of the <see cref="AudioImportEntry"/> class.
        /// </summary>
        /// <param name="request">The import request.</param>
        public AudioImportEntry(ref Request request)
        : base(ref request)
        {
            // Try to restore target asset Audio import options (useful for fast reimport)
            AudioImportSettings.TryRestore(ref _settings, ResultUrl);
        }

        /// <inheritdoc />
        public override object Settings => _settings;

        /// <inheritdoc />
        public override bool TryOverrideSettings(object settings)
        {
            if (settings is AudioImportSettings o)
            {
                _settings = o;
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

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool Internal_GetAudioImportOptions(string path, out AudioImportSettings.InternalOptions result);

        #endregion
    }
}
