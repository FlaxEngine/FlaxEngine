// Copyright (c) Wojciech Figat. All rights reserved.

using System.Text;

namespace FlaxEditor.Content
{
    /// <summary>
    /// Content item that contains script file with source code.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.ContentItem" />
    public abstract class ScriptItem : ContentItem
    {
        /// <summary>
        /// Gets the name of the script (deducted from the asset name).
        /// </summary>
        public string ScriptName => FilterScriptName(ShortName);

        /// <summary>
        /// Checks if the script item references the valid use script type that can be used in a gameplay.
        /// </summary>
        public bool IsValid => ScriptsBuilder.FindScript(ScriptName) != null;

        /// <summary>
        /// Initializes a new instance of the <see cref="ScriptItem"/> class.
        /// </summary>
        /// <param name="path">The path to the item.</param>
        protected ScriptItem(string path)
        : base(path)
        {
            ShowFileExtension = true;
        }

        internal static string FilterScriptName(string input)
        {
            var length = input.Length;
            var sb = new StringBuilder(length);

            // Skip leading '0-9' characters
            for (int i = 0; i < length; i++)
            {
                var c = input[i];

                if (char.IsLetterOrDigit(c) && !char.IsDigit(c))
                    break;
            }

            // Remove all characters that are not '_' or 'a-z' or 'A-Z' or '0-9'
            for (int i = 0; i < length; i++)
            {
                var c = input[i];

                if (c == '_' || char.IsLetterOrDigit(c))
                    sb.Append(c);
            }

            return sb.ToString();
        }

        /// <summary>
        /// Creates the name of the script for the given file.
        /// </summary>
        /// <param name="path">The path.</param>
        /// <returns>Script name</returns>
        public static string CreateScriptName(string path)
        {
            return FilterScriptName(System.IO.Path.GetFileNameWithoutExtension(path));
        }

        /// <inheritdoc />
        public override ContentItemType ItemType => ContentItemType.Script;

        /// <inheritdoc />
        public override ContentItemSearchFilter SearchFilter => ContentItemSearchFilter.Script;

        /// <inheritdoc />
        public override ScriptItem FindScriptWitScriptName(string scriptName)
        {
            return scriptName == ScriptName ? this : null;
        }

        /// <inheritdoc />
        public override void OnPathChanged()
        {
            ScriptsBuilder.MarkWorkspaceDirty();

            base.OnPathChanged();
        }

        /// <inheritdoc />
        public override void OnDelete()
        {
            ScriptsBuilder.MarkWorkspaceDirty();

            base.OnDelete();
        }
    }
}
