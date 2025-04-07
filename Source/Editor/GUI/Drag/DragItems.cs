// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.Content;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI.Drag
{
    /// <summary>
    /// Drag content items handler.
    /// </summary>
    public sealed class DragItems : DragItems<DragEventArgs>
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="DragItems"/> class.
        /// </summary>
        /// <param name="validateFunction">The validation function</param>
        public DragItems(Func<ContentItem, bool> validateFunction)
        : base(validateFunction)
        {
        }
    }

    /// <summary>
    /// Helper class for handling <see cref="ContentItem"/> drag and drop.
    /// </summary>
    /// <seealso cref="ContentItem" />
    public class DragItems<U> : DragHelper<ContentItem, U> where U : DragEventArgs
    {
        /// <summary>
        /// The default prefix for drag data used for <see cref="ContentItem"/>.
        /// </summary>
        public const string DragPrefix = "ASSET!?";

        /// <summary>
        /// Creates a new DragHelper
        /// </summary>
        /// <param name="validateFunction">The validation function</param>
        public DragItems(Func<ContentItem, bool> validateFunction)
        : base(validateFunction)
        {
        }

        /// <summary>
        /// Gets the drag data for the given file.
        /// </summary>
        /// <param name="path">The path.</param>
        /// <returns>The data.</returns>
        public DragData ToDragData(string path) => GetDragData(path);

        /// <inheritdoc/>
        public override DragData ToDragData(ContentItem item) => GetDragData(item);

        /// <inheritdoc/>
        public override DragData ToDragData(IEnumerable<ContentItem> items) => GetDragData(items);

        /// <summary>
        /// Gets the drag data.
        /// </summary>
        /// <param name="path">The path.</param>
        /// <returns>The data.</returns>
        public static DragData GetDragData(string path)
        {
            if (path == null)
                throw new ArgumentNullException();

            return new DragDataText(DragPrefix + path);
        }

        /// <summary>
        /// Gets the drag data.
        /// </summary>
        /// <param name="item">The item.</param>
        /// <returns>The data.</returns>
        public static DragDataText GetDragData(ContentItem item)
        {
            if (item == null)
                throw new ArgumentNullException();

            return new DragDataText(DragPrefix + item.Path);
        }

        /// <summary>
        /// Gets the drag data.
        /// </summary>
        /// <param name="items">The items.</param>
        /// <returns>The data.</returns>
        public static DragData GetDragData(IEnumerable<ContentItem> items)
        {
            if (items == null)
                throw new ArgumentNullException();

            string text = DragPrefix;
            foreach (var item in items)
                text += item.Path + '\n';
            return new DragDataText(text);
        }

        /// <inheritdoc/>
        public override IEnumerable<ContentItem> FromDragData(DragData data)
        {
            if (data is DragDataText dataText)
            {
                if (dataText.Text.StartsWith(DragPrefix))
                {
                    // Remove prefix and parse spitted names
                    var paths = dataText.Text.Remove(0, DragPrefix.Length).Split('\n');
                    var results = new List<ContentItem>(paths.Length);
                    for (int i = 0; i < paths.Length; i++)
                    {
                        // Find element
                        var obj = Editor.Instance.ContentDatabase.Find(paths[i]);

                        // Check it
                        if (obj != null)
                            results.Add(obj);
                    }

                    return results.ToArray();
                }
            }
            return new ContentItem[0];
        }
    }
}
