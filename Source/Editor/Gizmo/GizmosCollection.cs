// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEngine;

namespace FlaxEditor.Gizmo
{
    /// <summary>
    /// Represents collection of Gizmo tools where one is active and in use.
    /// </summary>
    /// <seealso cref="GizmoBase" />
    [HideInEditor]
    public class GizmosCollection : List<GizmoBase>
    {
        private GizmoBase _active;

        /// <summary>
        /// Occurs when active gizmo tool gets changed.
        /// </summary>
        public event Action ActiveChanged;

        /// <summary>
        /// Gets or sets the active gizmo.
        /// </summary>
        public GizmoBase Active
        {
            get => _active;
            set
            {
                if (_active == value)
                    return;
                if (value != null && !Contains(value))
                    throw new InvalidOperationException("Invalid Gizmo.");

                _active?.OnDeactivated();
                _active = value;
                _active?.OnActivated();
                ActiveChanged?.Invoke();
            }
        }

        /// <summary>
        /// Removes the specified item.
        /// </summary>
        /// <param name="item">The item.</param>
        public new void Remove(GizmoBase item)
        {
            if (item == _active)
                Active = null;
            base.Remove(item);
        }

        /// <summary>
        /// Clears the collection.
        /// </summary>
        public new void Clear()
        {
            Active = null;
            base.Clear();
        }
    }
}
