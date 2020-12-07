// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.Viewport.Modes;

namespace FlaxEditor.Viewport
{
    public partial class MainEditorGizmoViewport
    {
        private EditorGizmoMode _activeMode;
        private readonly List<EditorGizmoMode> _modes = new List<EditorGizmoMode>();

        /// <summary>
        /// Gets the active gizmo mode.
        /// </summary>
        public EditorGizmoMode ActiveMode => _activeMode;

        /// <summary>
        /// Occurs when active mode gets changed.
        /// </summary>
        public event Action<EditorGizmoMode> ActiveModeChanged;

        /// <summary>
        /// The sculpt terrain gizmo.
        /// </summary>
        public Tools.Terrain.SculptTerrainGizmoMode SculptTerrainGizmo;

        /// <summary>
        /// The paint terrain gizmo.
        /// </summary>
        public Tools.Terrain.PaintTerrainGizmoMode PaintTerrainGizmo;

        /// <summary>
        /// The edit terrain gizmo.
        /// </summary>
        public Tools.Terrain.EditTerrainGizmoMode EditTerrainGizmo;

        /// <summary>
        /// The paint foliage gizmo.
        /// </summary>
        public Tools.Foliage.PaintFoliageGizmoMode PaintFoliageGizmo;

        /// <summary>
        /// The edit foliage gizmo.
        /// </summary>
        public Tools.Foliage.EditFoliageGizmoMode EditFoliageGizmo;

        private void InitModes()
        {
            // Add default modes used by the editor
            _modes.Add(new TransformGizmoMode());
            _modes.Add(new NoGizmoMode());
            _modes.Add(SculptTerrainGizmo = new Tools.Terrain.SculptTerrainGizmoMode());
            _modes.Add(PaintTerrainGizmo = new Tools.Terrain.PaintTerrainGizmoMode());
            _modes.Add(EditTerrainGizmo = new Tools.Terrain.EditTerrainGizmoMode());
            _modes.Add(PaintFoliageGizmo = new Tools.Foliage.PaintFoliageGizmoMode());
            _modes.Add(EditFoliageGizmo = new Tools.Foliage.EditFoliageGizmoMode());
            for (int i = 0; i < _modes.Count; i++)
            {
                _modes[i].Init(this);
            }

            // Activate transform mode first
            _activeMode = _modes[0];
        }

        private void DisposeModes()
        {
            // Cleanup
            _activeMode = null;
            for (int i = 0; i < _modes.Count; i++)
                _modes[i].Dispose();
            _modes.Clear();
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
            if (mode.Viewport != null)
                throw new ArgumentException("Already added to other viewport.");

            _modes.Add(mode);
            mode.Init(this);
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
            if (mode.Viewport != this)
                throw new ArgumentException("Not added to this viewport.");

            if (_activeMode == mode)
                SetActiveMode(null);

            _modes.Remove(mode);
        }

        /// <summary>
        /// Sets the active mode.
        /// </summary>
        /// <param name="mode">The mode.</param>
        public void SetActiveMode(EditorGizmoMode mode)
        {
            if (mode == _activeMode)
                return;
            if (mode != null)
            {
                if (!_modes.Contains(mode))
                    throw new ArgumentException("Not added.");
                if (mode.Viewport != this)
                    throw new ArgumentException("Not added to this viewport.");
            }

            _activeMode?.OnDeactivated();

            Gizmos.Active = null;
            _activeMode = mode;

            _activeMode?.OnActivated();

            ActiveModeChanged?.Invoke(mode);
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
                    SetActiveMode(mode);
                    return mode;
                }
            }

            throw new ArgumentException("Not added mode to activate.");
        }
    }
}
