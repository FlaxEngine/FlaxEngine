// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;

namespace FlaxEditor.Content.Create
{
    /// <summary>
    /// Visual Script asset creating handler. Allows to specify base class to inherit from.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.Create.CreateFileEntry" />
    public class VisualScriptCreateEntry : CreateFileEntry
    {
        /// <inheritdoc/>
        public override bool CanBeCreated => _options.BaseClass != null;

        /// <summary>
        /// The create options.
        /// </summary>
        public class Options
        {
            /// <summary>
            /// The template.
            /// </summary>
            [TypeReference(typeof(FlaxEngine.Object), nameof(IsValid))]
            [Tooltip("The base class of the new Visual Script to inherit from.")]
            public Type BaseClass = typeof(Script);

            private static bool IsValid(Type type)
            {
                return (type.IsPublic || type.IsNestedPublic) && !type.IsSealed && !type.IsGenericType;
            }
        }

        private readonly Options _options = new Options();

        /// <inheritdoc />
        public override object Settings => _options;

        /// <summary>
        /// Initializes a new instance of the <see cref="VisualScriptCreateEntry"/> class.
        /// </summary>
        /// <param name="resultUrl">The result file url.</param>
        public VisualScriptCreateEntry(string resultUrl)
        : base("Settings", resultUrl)
        {
        }

        /// <inheritdoc />
        public override bool Create()
        {
            return Editor.CreateVisualScript(ResultUrl, _options.BaseClass?.FullName);
        }
    }
}
