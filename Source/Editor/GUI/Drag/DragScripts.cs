// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI.Drag
{
    /// <summary>
    /// Scripts references drag handler.
    /// </summary>
    public sealed class DragScripts : DragScripts<DragEventArgs>
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="DragScripts"/> class.
        /// </summary>
        /// <param name="validateFunction">The validation function</param>
        public DragScripts(Func<Script, bool> validateFunction)
        : base(validateFunction)
        {
        }
    }

    /// <summary>
    /// Helper class for handling <see cref="Script"/> instance drag and drop.
    /// </summary>
    /// <seealso cref="Script" />
    public class DragScripts<U> : DragHelper<Script, U> where U : DragEventArgs
    {
        /// <summary>
        /// The default prefix for drag data used for <see cref="Script"/>.
        /// </summary>
        public const string DragPrefix = "SCRIPT!?";

        /// <summary>
        /// Creates a new DragHelper
        /// </summary>
        /// <param name="validateFunction">The validation function</param>
        public DragScripts(Func<Script, bool> validateFunction)
        : base(validateFunction)
        {
        }


        /// <inheritdoc/>
        public override DragData ToDragData(Script item) => GetDragData(item);

        /// <inheritdoc/>
        public override DragData ToDragData(IEnumerable<Script> items) => GetDragData(items);

        /// <summary>
        /// Gets the drag data.
        /// </summary>
        /// <param name="item">The script.</param>
        /// <returns>The data.</returns>
        public static DragData GetDragData(Script item)
        {
            if (item == null)
                throw new ArgumentNullException();
            return new DragDataText(DragPrefix + item.ID.ToString("N"));
        }

        /// <summary>
        /// Gets the drag data.
        /// </summary>
        /// <param name="items">The items.</param>
        /// <returns>The data.</returns>
        public static DragData GetDragData(IEnumerable<Script> items)
        {
            if (items == null)
                throw new ArgumentNullException();
            string text = DragPrefix;
            foreach (var item in items)
                text += item.ID.ToString("N") + '\n';
            return new DragDataText(text);
        }

        /// <summary>
        /// Tries to parse the drag data.
        /// </summary>
        /// <param name="data">The data.</param>
        /// <returns>Gathered objects or empty IEnumerable if cannot get any valid.</returns>
        public override IEnumerable<Script> FromDragData(DragData data)
        {
            if (data is DragDataText dataText)
            {
                if (dataText.Text.StartsWith(DragPrefix))
                {
                    // Remove prefix and parse spitted names
                    var ids = dataText.Text.Remove(0, DragPrefix.Length).Split('\n');
                    var results = new List<Script>(ids.Length);
                    for (int i = 0; i < ids.Length; i++)
                    {
                        if (Guid.TryParse(ids[i], out Guid id))
                        {
                            var obj = FlaxEngine.Object.Find<Script>(ref id);
                            if (obj != null)
                                results.Add(obj);
                        }
                    }

                    return results.ToArray();
                }
            }
            return Utils.GetEmptyArray<Script>();
        }

        /// <summary>
        /// Tries to parse the drag data to validate if it has valid scripts drag.
        /// </summary>
        /// <param name="data">The data.</param>
        /// <returns>True if drag data has valid scripts, otherwise false.</returns>
        public static bool IsValidData(DragDataText data)
        {
            if (data.Text.StartsWith(DragPrefix))
            {
                // Remove prefix and parse spitted names
                var ids = data.Text.Remove(0, DragPrefix.Length).Split('\n');
                for (int i = 0; i < ids.Length; i++)
                {
                    // Find element
                    if (Guid.TryParse(ids[i], out Guid id))
                    {
                        var obj = FlaxEngine.Object.Find<Script>(ref id);

                        // Check it
                        if (obj != null)
                            return true;
                    }
                }
            }
            return false;
        }
    }
}
