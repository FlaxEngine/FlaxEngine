// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.Linq;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI.Drag
{
    /// <summary>
    /// Handles a list of <see cref="DragHelper{T, U}"/>s
    /// </summary>
    public class DragHandlers : List<DragHelper>
    {
        /// <summary>
        /// Called when drag enter.
        /// </summary>
        /// <param name="data">The data.</param>
        /// <returns>The result.</returns>
        public DragDropEffect OnDragEnter(DragData data)
        {
            DragDropEffect effect = DragDropEffect.None;

            for (var i = 0; i < Count; i++)
            {
                var dragHelper = this[i];
                if (dragHelper.OnDragEnter(data))
                {
                    effect = dragHelper.Effect;
                }
            }

            return effect;
        }

        /// <summary>
        /// Called when drag leaves.
        /// </summary>
        public void OnDragLeave()
        {
            for (var i = 0; i < Count; i++)
            {
                this[i].OnDragLeave();
            }
        }

        /// <summary>
        /// Called when drag drops.
        /// </summary>
        /// <param name="dragEventArgs">The <see cref="DragEventArgs"/> instance containing the event data.</param>
        public void OnDragDrop(DragEventArgs dragEventArgs)
        {
            for (var i = 0; i < Count; i++)
            {
                this[i].OnDragDrop(dragEventArgs);
            }
        }

        /// <summary>
        /// Determines whether has valid drag handler to handle the drag request.
        /// </summary>
        public bool HasValidDrag
        {
            get
            {
                for (var i = 0; i < Count; i++)
                {
                    if (this[i].HasValidDrag)
                    {
                        return true;
                    }
                }
                return false;
            }
        }

        /// <summary>
        /// Gets the first valid drag helper.
        /// </summary>
        /// <returns>The drag helper.</returns>
        public DragHelper WithValidDrag()
        {
            return this
                   .DefaultIfEmpty()
                   .First(helper => helper.HasValidDrag);
        }

        /// <summary>
        /// Gets the valid drag effect to use.
        /// </summary>
        public DragDropEffect Effect
        {
            get
            {
                for (var i = 0; i < Count; i++)
                {
                    var dragHelper = this[i];
                    if (dragHelper.HasValidDrag)
                    {
                        return dragHelper.Effect;
                    }
                }

                return DragDropEffect.None;
            }
        }
    }
}
