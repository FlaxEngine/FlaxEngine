// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.SceneGraph;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI.Drag
{
    /// <summary>
    /// Actors references collection drag handler.
    /// </summary>
    public sealed class DragActors : DragActors<DragEventArgs>
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="DragActors"/> class.
        /// </summary>
        /// <param name="validateFunction">The validation function</param>
        public DragActors(Func<ActorNode, bool> validateFunction)
        : base(validateFunction)
        {
        }
    }

    /// <summary>
    /// Helper class for handling <see cref="ActorNode"/> drag and drop.
    /// </summary>
    /// <seealso cref="Actor" />
    /// <seealso cref="ActorNode" />
    public class DragActors<U> : DragHelper<ActorNode, U> where U : DragEventArgs
    {
        /// <summary>
        /// The default prefix for drag data used for <see cref="ActorNode"/>.
        /// </summary>
        public const string DragPrefix = "ACTOR!?";

        /// <summary>
        /// Creates a new DragHelper
        /// </summary>
        /// <param name="validateFunction">The validation function</param>
        public DragActors(Func<ActorNode, bool> validateFunction)
        : base(validateFunction)
        {
        }

        /// <summary>
        /// Gets the drag data.
        /// </summary>
        /// <param name="actor">The actor.</param>
        /// <returns>The data.</returns>
        public DragData ToDragData(Actor actor) => GetDragData(actor);

        /// <inheritdoc/>
        public override DragData ToDragData(ActorNode item) => GetDragData(item);

        /// <inheritdoc/>
        public override DragData ToDragData(IEnumerable<ActorNode> items) => GetDragData(items);

        /// <summary>
        /// Gets the drag data.
        /// </summary>
        /// <param name="actor">The actor.</param>
        /// <returns>The data.</returns>
        public static DragData GetDragData(Actor actor)
        {
            if (actor == null)
                throw new ArgumentNullException();

            return new DragDataText(DragPrefix + actor.ID.ToString("N"));
        }

        /// <summary>
        /// Gets the drag data.
        /// </summary>
        /// <param name="item">The item.</param>
        /// <returns>The data.</returns>
        public static DragData GetDragData(ActorNode item)
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
        public static DragData GetDragData(IEnumerable<ActorNode> items)
        {
            if (items == null)
                throw new ArgumentNullException();

            string text = DragPrefix;
            foreach (var item in items)
                text += item.ID.ToString("N") + '\n';
            return new DragDataText(text);
        }

        /// <inheritdoc/>
        public override IEnumerable<ActorNode> FromDragData(DragData data)
        {
            if (data is DragDataText dataText)
            {
                if (dataText.Text.StartsWith(DragPrefix))
                {
                    // Remove prefix and parse spitted names
                    var ids = dataText.Text.Remove(0, DragPrefix.Length).Split('\n');
                    var results = new List<ActorNode>(ids.Length);
                    for (int i = 0; i < ids.Length; i++)
                    {
                        // Find element
                        if (Guid.TryParse(ids[i], out Guid id))
                        {
                            var obj = Editor.Instance.Scene.GetActorNode(id);

                            // Check it
                            if (obj != null)
                                results.Add(obj);
                        }
                    }

                    return results.ToArray();
                }
            }

            return new ActorNode[0];
        }
    }
}
