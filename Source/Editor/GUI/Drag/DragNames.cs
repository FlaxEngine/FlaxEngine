// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI.Drag
{
    /// <summary>
    /// Generic items names collection drag handler.
    /// </summary>
    public sealed class DragNames : DragNames<DragEventArgs>
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="DragNames"/> class.
        /// </summary>
        /// <param name="prefix">The drag data prefix.</param>
        /// <param name="validateFunction">The validation function.</param>
        public DragNames(string prefix, Func<string, bool> validateFunction)
        : base(prefix, validateFunction)
        {
        }
    }

    /// <summary>
    /// Helper class for handling generic items names drag and drop.
    /// </summary>
    public class DragNames<U> : DragHelper<string, U> where U : DragEventArgs
    {
        /// <summary>
        /// Gets the drag data prefix.
        /// </summary>
        public string DragPrefix { get; }

        /// <summary>
        /// Creates a new DragHelper
        /// </summary>
        /// <param name="prefix">The drag data prefix.</param>
        /// <param name="validateFunction">The validation function.</param>
        public DragNames(string prefix, Func<string, bool> validateFunction)
        : base(validateFunction)
        {
            DragPrefix = prefix;
        }

        /// <inheritdoc/>
        public override DragData ToDragData(string track) => GetDragData(DragPrefix, track);

        /// <inheritdoc/>
        public override DragData ToDragData(IEnumerable<string> tracks) => GetDragData(DragPrefix, tracks);

        /// <summary>
        /// Gets the drag data.
        /// </summary>
        /// <param name="dragPrefix">The drag dat prefix.</param>
        /// <param name="name">The name.</param>
        /// <returns>The data.</returns>
        public static DragData GetDragData(string dragPrefix, string name)
        {
            if (name == null)
                throw new ArgumentNullException();

            return new DragDataText(dragPrefix + name);
        }

        /// <summary>
        /// Gets the drag data.
        /// </summary>
        /// <param name="dragPrefix">The drag dat prefix.</param>
        /// <param name="names">The names.</param>
        /// <returns>The data.</returns>
        public static DragData GetDragData(string dragPrefix, IEnumerable<string> names)
        {
            if (names == null)
                throw new ArgumentNullException();

            string text = dragPrefix;
            foreach (var item in names)
                text += item + '\n';
            return new DragDataText(text);
        }

        /// <summary>
        /// Tries to parse the drag data to extract generic items names collection.
        /// </summary>
        /// <param name="data">The data.</param>
        /// <returns>Gathered objects or empty array if cannot get any valid.</returns>
        public override IEnumerable<string> FromDragData(DragData data)
        {
            if (data is DragDataText dataText)
            {
                if (dataText.Text.StartsWith(DragPrefix))
                {
                    // Remove prefix and split names
                    return dataText.Text.Remove(0, DragPrefix.Length).Split('\n');
                }
            }
            return new string[0];
        }
    }
}
