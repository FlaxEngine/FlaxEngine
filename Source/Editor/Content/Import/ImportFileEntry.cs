// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.IO;

namespace FlaxEditor.Content.Import
{
    /// <summary>
    /// Creates new <see cref="ImportFileEntry"/> for the given source file.
    /// </summary>
    /// <param name="request">The import request.</param>
    /// <returns>The file entry.</returns>
    public delegate ImportFileEntry ImportFileEntryHandler(ref Request request);

    /// <summary>
    /// File import entry.
    /// </summary>
    public class ImportFileEntry : IFileEntryAction
    {
        /// <inheritdoc />
        public string SourceUrl { get; }

        /// <inheritdoc />
        public string ResultUrl { get; private set; }

        /// <summary>
        /// Gets a value indicating whether this entry has settings to modify.
        /// </summary>
        public virtual bool HasSettings => Settings != null;

        /// <summary>
        /// Gets or sets the settings object to modify.
        /// </summary>
        public virtual object Settings => null;

        /// <summary>
        /// Tries the override import settings.
        /// </summary>
        /// <param name="settings">The settings.</param>
        /// <returns>True if settings override was successful and there is no need to edit them in dedicated dialog, otherwise false.</returns>
        public virtual bool TryOverrideSettings(object settings)
        {
            return false;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ImportFileEntry"/> class.
        /// </summary>
        /// <param name="request">The import request.</param>
        public ImportFileEntry(ref Request request)
        {
            SourceUrl = request.InputPath;
            ResultUrl = request.OutputPath;
        }

        /// <summary>
        /// Modifies the result URL filename (keeps destination folder and extension).
        /// </summary>
        /// <param name="filename">The new filename.</param>
        public void ModifyResultFilename(string filename)
        {
            var directory = Path.GetDirectoryName(ResultUrl) ?? string.Empty;
            var extension = Path.GetExtension(ResultUrl);
            ResultUrl = Path.Combine(directory, filename + extension);
        }

        /// <summary>
        /// Performs file importing.
        /// </summary>
        /// <returns>True if failed, otherwise false.</returns>
        public virtual bool Import()
        {
            // Skip if missing
            if (!Directory.Exists(SourceUrl) && !File.Exists(SourceUrl))
                return true;

            // Setup output
            string folder = Path.GetDirectoryName(ResultUrl);
            if (folder != null && !Directory.Exists(folder))
                Directory.CreateDirectory(folder);

            if (Directory.Exists(SourceUrl))
            {
                // Copy directory
                Utilities.Utils.DirectoryCopy(SourceUrl, ResultUrl, true);
                return false;
            }

            // Copy file
            File.Copy(SourceUrl, ResultUrl, true);
            return false;
        }

        /// <summary>
        /// The file types registered for importing. Key is a file extension (without a leading dot).
        /// Allows to plug custom importing options gather for different input file types.
        /// </summary>
        public static readonly Dictionary<string, ImportFileEntryHandler> FileTypes = new Dictionary<string, ImportFileEntryHandler>(32);

        /// <summary>
        /// Creates the entry.
        /// </summary>
        /// <param name="request">The import request.</param>
        /// <returns>Created file entry.</returns>
        public static ImportFileEntry CreateEntry(ref Request request)
        {
            // Get extension (without a dot)
            var extension = Path.GetExtension(request.InputPath);
            if (string.IsNullOrEmpty(extension))
                return new FolderImportEntry(ref request);
            if (extension[0] == '.')
                extension = extension.Remove(0, 1);
            extension = extension.ToLower();

            // Check if use overriden type
            if (FileTypes.TryGetValue(extension, out ImportFileEntryHandler createDelegate))
                return createDelegate(ref request);

            return request.IsInBuilt ? new AssetImportEntry(ref request) : new ImportFileEntry(ref request);
        }

        internal static void RegisterDefaultTypes()
        {
            // Textures
            FileTypes["tga"] = ImportTexture;
            FileTypes["png"] = ImportTexture;
            FileTypes["bmp"] = ImportTexture;
            FileTypes["gif"] = ImportTexture;
            FileTypes["tiff"] = ImportTexture;
            FileTypes["tif"] = ImportTexture;
            FileTypes["jpeg"] = ImportTexture;
            FileTypes["jpg"] = ImportTexture;
            FileTypes["dds"] = ImportTexture;
            FileTypes["hdr"] = ImportTexture;
            FileTypes["raw"] = ImportTexture;
            FileTypes["exr"] = ImportTexture;

            // Models
            FileTypes["obj"] = ImportModel;
            FileTypes["fbx"] = ImportModel;
            FileTypes["x"] = ImportModel;
            FileTypes["dae"] = ImportModel;
            FileTypes["gltf"] = ImportModel;
            FileTypes["glb"] = ImportModel;
            //
            FileTypes["blend"] = ImportModel;
            FileTypes["bvh"] = ImportModel;
            FileTypes["ase"] = ImportModel;
            FileTypes["ply"] = ImportModel;
            FileTypes["dxf"] = ImportModel;
            FileTypes["ifc"] = ImportModel;
            FileTypes["nff"] = ImportModel;
            FileTypes["smd"] = ImportModel;
            FileTypes["vta"] = ImportModel;
            FileTypes["mdl"] = ImportModel;
            FileTypes["md2"] = ImportModel;
            FileTypes["md3"] = ImportModel;
            FileTypes["md5mesh"] = ImportModel;
            FileTypes["q3o"] = ImportModel;
            FileTypes["q3s"] = ImportModel;
            FileTypes["ac"] = ImportModel;
            FileTypes["stl"] = ImportModel;
            FileTypes["lwo"] = ImportModel;
            FileTypes["lws"] = ImportModel;
            FileTypes["lxo"] = ImportModel;

            // Audio
            FileTypes["wav"] = ImportAudio;
            FileTypes["mp3"] = ImportAudio;
            FileTypes["ogg"] = ImportAudio;
        }

        private static ImportFileEntry ImportModel(ref Request request)
        {
            return new ModelImportEntry(ref request);
        }

        private static ImportFileEntry ImportAudio(ref Request request)
        {
            return new AudioImportEntry(ref request);
        }

        private static ImportFileEntry ImportTexture(ref Request request)
        {
            return new TextureImportEntry(ref request);
        }

        /// <inheritdoc />
        public bool Execute()
        {
            return Import();
        }
    }
}
