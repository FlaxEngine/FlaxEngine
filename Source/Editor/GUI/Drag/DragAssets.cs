// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.Content;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI.Drag
{
    /// <summary>
    /// Assets collection drag handler.
    /// </summary>
    public sealed class DragAssets : DragAssets<DragEventArgs>
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="DragAssets"/> class.
        /// </summary>
        /// <param name="validateFunction">The validation function</param>
        public DragAssets(Func<AssetItem, bool> validateFunction)
        : base(validateFunction)
        {
        }
    }

    /// <summary>
    /// Helper class for handling <see cref="AssetItem"/> drag and drop.
    /// </summary>
    /// <seealso cref="AssetItem" />
    public class DragAssets<U> : DragHelper<AssetItem, U> where U : DragEventArgs
    {
        /// <summary>
        /// The default prefix for drag data used for <see cref="ContentItem"/>.
        /// </summary>
        public const string DragPrefix = DragItems<DragEventArgs>.DragPrefix;

        /// <summary>
        /// Creates a new DragHelper
        /// </summary>
        /// <param name="validateFunction">The validation function</param>
        public DragAssets(Func<AssetItem, bool> validateFunction)
        : base(validateFunction)
        {
        }

        /// <summary>
        /// Gets the drag data (finds asset item).
        /// </summary>
        /// <param name="asset">The asset.</param>
        /// <returns>The data.</returns>
        public DragData ToDragData(Asset asset) => GetDragData(asset);

        /// <inheritdoc/>
        public override DragData ToDragData(AssetItem item) => GetDragData(item);

        /// <inheritdoc/>
        public override DragData ToDragData(IEnumerable<AssetItem> items) => GetDragData(items);

        /// <summary>
        /// Gets the drag data.
        /// </summary>
        /// <param name="asset">The asset.</param>
        /// <returns>The data.</returns>
        public static DragData GetDragData(Asset asset)
        {
            return DragItems<DragEventArgs>.GetDragData(Editor.Instance.ContentDatabase.Find(asset.ID));
        }

        /// <summary>
        /// Gets the drag data.
        /// </summary>
        /// <param name="item">The item.</param>
        /// <returns>The data.</returns>
        public static DragData GetDragData(AssetItem item)
        {
            return DragItems<DragEventArgs>.GetDragData(item);
        }

        /// <summary>
        /// Gets the drag data.
        /// </summary>
        /// <param name="items">The items.</param>
        /// <returns>The data.</returns>
        public static DragData GetDragData(IEnumerable<AssetItem> items)
        {
            return DragItems<DragEventArgs>.GetDragData(items);
        }

        /// <summary>
        /// Tries to parse the drag data to extract <see cref="AssetItem"/> collection.
        /// </summary>
        /// <param name="data">The data.</param>
        /// <returns>Gathered objects or empty array if cannot get any valid.</returns>
        public override IEnumerable<AssetItem> FromDragData(DragData data)
        {
            if (data is DragDataText dataText)
            {
                if (dataText.Text.StartsWith(DragPrefix))
                {
                    // Remove prefix and parse spitted names
                    var paths = dataText.Text.Remove(0, DragPrefix.Length).Split('\n');
                    var results = new List<AssetItem>(paths.Length);
                    for (int i = 0; i < paths.Length; i++)
                    {
                        // Find element
                        if (Editor.Instance.ContentDatabase.Find(paths[i]) is AssetItem obj)
                            results.Add(obj);
                    }

                    return results.ToArray();
                }
            }
            return new AssetItem[0];
        }
    }
}
