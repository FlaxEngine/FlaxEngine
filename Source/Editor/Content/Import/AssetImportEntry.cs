// Copyright (c) Wojciech Figat. All rights reserved.

namespace FlaxEditor.Content.Import
{
    /// <summary>
    /// Asset import entry.
    /// </summary>
    public class AssetImportEntry : ImportFileEntry
    {
        /// <inheritdoc />
        public AssetImportEntry(ref Request request)
        : base(ref request)
        {
        }

        /// <inheritdoc />
        public override bool Import()
        {
            // Use engine backend
            return Editor.Import(SourceUrl, ResultUrl);
        }
    }
}
