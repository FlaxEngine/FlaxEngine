// Copyright (c) Wojciech Figat. All rights reserved.

using System.Runtime.InteropServices;
using FlaxEngine;

namespace FlaxEditor.Surface
{
    public partial class VisjectSurface
    {
        [StructLayout(LayoutKind.Sequential)]
        internal struct Meta10 // TypeID: 10, for surface
        {
            public Float2 ViewCenterPosition;
            public float Scale;
        }

        [StructLayout(LayoutKind.Sequential)]
        internal struct Meta11 // TypeID: 11, for nodes
        {
            public Float2 Position;
            public bool Selected;
        }

        /// <summary>
        /// Loads surface from the bytes.
        /// </summary>
        /// <remarks>
        /// The method calls the <see cref="ISurfaceContext.SurfaceData"/> getter to load the surface data bytes.
        /// </remarks>
        /// <returns>True if failed, otherwise false.</returns>
        public virtual bool Load()
        {
            Enabled = false;

            // Clean data
            _connectionInstigator = null;
            _lastInstigatorUnderMouse = null;

            var failed = RootContext.Load();

            Enabled = true;

            if (failed)
            {
                // Error
                return true;
            }

            // Update the view from context meta
            ViewCenterPosition = _context.CachedSurfaceMeta.ViewCenterPosition;
            ViewScale = _context.CachedSurfaceMeta.Scale;

            // End
            Owner.OnSurfaceEditedChanged();

            return false;
        }

        /// <summary>
        /// Saves the surface graph to bytes.
        /// </summary>
        /// <remarks>
        /// The method calls the <see cref="ISurfaceContext.SurfaceData"/> setter to assign the result bytes. Sets null value if failed.
        /// </remarks>
        /// <returns>True if failed, otherwise false.</returns>
        public virtual bool Save()
        {
            var wasEdited = IsEdited;

            // Update the current context meta
            _context.CachedSurfaceMeta.ViewCenterPosition = ViewCenterPosition;
            _context.CachedSurfaceMeta.Scale = ViewScale;

            // Save context (and every modified child context)
            if (RootContext.Save())
                return true;

            // Clear flag
            if (wasEdited)
            {
                Owner.OnSurfaceEditedChanged();
            }

            return false;
        }
    }
}
