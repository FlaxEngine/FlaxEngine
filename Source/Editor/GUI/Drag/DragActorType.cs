// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.SceneGraph;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Utilities;

namespace FlaxEditor.GUI.Drag
{
    /// <summary>
    /// Actor type drag handler.
    /// </summary>
    public sealed class DragActorType : DragActorType<DragEventArgs>
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="DragActorType"/> class.
        /// </summary>
        /// <param name="validateFunction">The validation function</param>
        public DragActorType(Func<ScriptType, bool> validateFunction)
        : base(validateFunction)
        {
        }
    }

    /// <summary>
    /// Helper class for handling actor type drag and drop (for spawning).
    /// </summary>
    /// <seealso cref="Actor" />
    /// <seealso cref="ActorNode" />
    public class DragActorType<U> : DragHelper<ScriptType, U> where U : DragEventArgs
    {
        /// <summary>
        /// The default prefix for drag data used for actor type drag and drop.
        /// </summary>
        public const string DragPrefix = "ATYPE!?";

        /// <summary>
        /// Creates a new DragHelper
        /// </summary>
        /// <param name="validateFunction">The validation function</param>
        public DragActorType(Func<ScriptType, bool> validateFunction)
        : base(validateFunction)
        {
        }

        /// <inheritdoc/>
        public override DragData ToDragData(ScriptType item) => GetDragData(item);

        /// <inheritdoc/>
        public override DragData ToDragData(IEnumerable<ScriptType> items)
        {
            throw new NotImplementedException();
        }

        /// <summary>
        /// Gets the drag data.
        /// </summary>
        /// <param name="item">The actor type.</param>
        /// <returns>The data</returns>
        public static DragData GetDragData(Type item)
        {
            if (item == null)
                throw new ArgumentNullException();
            return new DragDataText(DragPrefix + item.FullName);
        }

        /// <summary>
        /// Gets the drag data.
        /// </summary>
        /// <param name="item">The actor type.</param>
        /// <returns>The data</returns>
        public static DragData GetDragData(ScriptType item)
        {
            if (item == ScriptType.Null)
                throw new ArgumentNullException();
            return new DragDataText(DragPrefix + item.TypeName);
        }

        /// <summary>
        /// Tries to parse the drag data to extract <see cref="Type"/> collection.
        /// </summary>
        /// <param name="data">The data.</param>
        /// <returns>Gathered objects or empty array if cannot get any valid.</returns>
        public override IEnumerable<ScriptType> FromDragData(DragData data)
        {
            if (data is DragDataText dataText)
            {
                if (dataText.Text.StartsWith(DragPrefix))
                {
                    // Remove prefix and parse spitted names
                    var types = dataText.Text.Remove(0, DragPrefix.Length).Split('\n');
                    var results = new List<ScriptType>(types.Length);
                    for (int i = 0; i < types.Length; i++)
                    {
                        // Find type
                        var obj = TypeUtils.GetType(types[i]);
                        if (obj)
                            results.Add(obj);
                    }
                    return results;
                }
            }
            return Utils.GetEmptyArray<ScriptType>();
        }
    }
}
