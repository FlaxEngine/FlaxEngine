// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.Viewport.Modes;
using FlaxEngine;

namespace FlaxEditor.Gizmo
{
    /// <summary>
    /// Represents collection of gizmo tools where one is active and in use.
    /// </summary>
    /// <seealso cref="GizmoBase" />
    [HideInEditor]
    public class GizmosCollection : List<GizmoBase>
    {
        private IGizmoOwner _owner;
        private GizmoBase _active;
        private EditorGizmoMode _activeMode;
        private readonly List<EditorGizmoMode> _modes = new List<EditorGizmoMode>();

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
                    throw new ArgumentException("Not added.");

                _active?.OnDeactivated();
                _active = value;
                _active?.OnActivated();
                ActiveChanged?.Invoke();
            }
        }

        /// <summary>
        /// Gets the active gizmo mode.
        /// </summary>
        public EditorGizmoMode ActiveMode
        {
            get => _activeMode;
            set
            {
                if (_activeMode == value)
                    return;
                if (value != null)
                {
                    if (!_modes.Contains(value))
                        throw new ArgumentException("Not added.");
                    if (value.Owner != _owner)
                        throw new InvalidOperationException();
                }

                _activeMode?.OnDeactivated();
                Active = null;
                _activeMode = value;
                _activeMode?.OnActivated();
                ActiveModeChanged?.Invoke(value);
            }
        }

        /// <summary>
        /// Occurs when active mode gets changed.
        /// </summary>
        public event Action<EditorGizmoMode> ActiveModeChanged;

        /// <summary>
        /// Init.
        /// </summary>
        /// <param name="owner">The gizmos owner interface.</param>
        public GizmosCollection(IGizmoOwner owner)
        {
            _owner = owner;
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
            ActiveMode = null;
            foreach (var mode in _modes)
                mode.Dispose();
            _modes.Clear();

            base.Clear();
        }

        /// <summary>
        /// Adds the mode to the viewport.
        /// </summary>
        /// <param name="mode">The mode.</param>
        public void AddMode(EditorGizmoMode mode)
        {
            if (mode == null)
                throw new ArgumentNullException(nameof(mode));
            if (_modes.Contains(mode))
                throw new ArgumentException("Already added.");
            if (mode.Owner != null)
                throw new ArgumentException("Already added to other viewport.");

            _modes.Add(mode);
            mode.Init(_owner);
        }

        /// <summary>
        /// Removes the mode from the viewport.
        /// </summary>
        /// <param name="mode">The mode.</param>
        public void RemoveMode(EditorGizmoMode mode)
        {
            if (mode == null)
                throw new ArgumentNullException(nameof(mode));
            if (!_modes.Contains(mode))
                throw new ArgumentException("Not added.");
            if (mode.Owner != _owner)
                throw new ArgumentException("Not added to this viewport.");

            if (_activeMode == mode)
                ActiveMode = null;
            _modes.Remove(mode);
        }

        /// <summary>
        /// Sets the active mode.
        /// </summary>
        /// <typeparam name="T">The mode type.</typeparam>
        /// <returns>The activated mode.</returns>
        public T SetActiveMode<T>() where T : EditorGizmoMode
        {
            for (int i = 0; i < _modes.Count; i++)
            {
                if (_modes[i] is T mode)
                {
                    ActiveMode = mode;
                    return mode;
                }
            }
            throw new ArgumentException("Not added mode to activate.");
        }

        /// <summary>
        /// Gets the gizmo of a given type or returns null if not added.
        /// </summary>
        /// <typeparam name="T">Type of the gizmo.</typeparam>
        /// <returns>Found gizmo or null.</returns>
        public T Get<T>() where T : GizmoBase
        {
            foreach (var e in this)
            {
                if (e is T asT)
                    return asT;
            }
            return null;
        }
    }
}
