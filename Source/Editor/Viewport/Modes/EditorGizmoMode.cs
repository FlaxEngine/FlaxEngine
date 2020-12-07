// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;

namespace FlaxEditor.Viewport.Modes
{
    /// <summary>
    /// Editor viewport gizmo mode descriptor. Defines which gizmo tools to show and how to handle scene editing.
    /// </summary>
    /// <remarks>
    /// Only one gizmo mode can be active at the time. It defines the viewport toolset usage.
    /// </remarks>
    [HideInEditor]
    public abstract class EditorGizmoMode
    {
        private MainEditorGizmoViewport _viewport;

        /// <summary>
        /// Gets the viewport.
        /// </summary>
        public MainEditorGizmoViewport Viewport => _viewport;

        /// <summary>
        /// Occurs when mode gets activated.
        /// </summary>
        public event Action Activated;

        /// <summary>
        /// Occurs when mode gets deactivated.
        /// </summary>
        public event Action Deactivated;

        /// <summary>
        /// Initializes the specified mode and links it to the viewport.
        /// </summary>
        /// <param name="viewport">The viewport.</param>
        public virtual void Init(MainEditorGizmoViewport viewport)
        {
            _viewport = viewport;
        }

        /// <summary>
        /// Releases the mode. Called on editor exit or when mode gets removed from the current viewport.
        /// </summary>
        public virtual void Dispose()
        {
            _viewport = null;
        }

        /// <summary>
        /// Called when mode gets activated.
        /// </summary>
        public virtual void OnActivated()
        {
            Activated?.Invoke();
        }

        /// <summary>
        /// Called when mode gets deactivated.
        /// </summary>
        public virtual void OnDeactivated()
        {
            Deactivated?.Invoke();
        }
    }
}
