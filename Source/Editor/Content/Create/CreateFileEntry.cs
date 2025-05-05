// Copyright (c) Wojciech Figat. All rights reserved.

namespace FlaxEditor.Content.Create
{
    /// <summary>
    /// File create entry.
    /// </summary>
    public abstract class CreateFileEntry : IFileEntryAction
    {
        /// <inheritdoc />
        public string SourceUrl { get; }

        /// <inheritdoc />
        public string ResultUrl { get; }

        /// <summary>
        /// Gets a value indicating wether a file can be created based on the current settings.
        /// </summary>
        public abstract bool CanBeCreated { get; }

        /// <summary>
        /// Gets a value indicating whether this entry has settings to modify.
        /// </summary>
        public virtual bool HasSettings => Settings != null;

        /// <summary>
        /// Gets or sets the settings object to modify.
        /// </summary>
        public virtual object Settings => null;

        /// <summary>
        /// Initializes a new instance of the <see cref="CreateFileEntry"/> class.
        /// </summary>
        /// <param name="outputType">The output file type.</param>
        /// <param name="resultUrl">The result file url.</param>
        protected CreateFileEntry(string outputType, string resultUrl)
        {
            SourceUrl = outputType;
            ResultUrl = resultUrl;
        }

        /// <summary>
        /// Creates the result file.
        /// </summary>
        /// <returns>True if failed, otherwise false.</returns>
        public abstract bool Create();

        /// <inheritdoc />
        public bool Execute()
        {
            return Create();
        }
    }
}
