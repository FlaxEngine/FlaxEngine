// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Surface
{
    /// <summary>
    /// Base class for Visject Surface components like nodes or comments. Used to unify various logic parts across different surface elements.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    [HideInEditor]
    public abstract class SurfaceControl : ContainerControl
    {
        /// <summary>
        /// The mouse position in local control space. Updates by auto.
        /// </summary>
        protected Float2 _mousePosition = Float2.Minimum;

        /// <summary>
        /// The is selected flag for element.
        /// </summary>
        protected bool _isSelected;

        /// <summary>
        /// The context.
        /// </summary>
        public readonly VisjectSurfaceContext Context;

        /// <summary>
        /// The surface.
        /// </summary>
        public readonly VisjectSurface Surface;

        /// <summary>
        /// Gets a value indicating whether this control is selected.
        /// </summary>
        public bool IsSelected
        {
            get => _isSelected;
            internal set
            {
                _isSelected = value;
                OnSelectionChanged();
            }
        }

        /// <summary>
        /// Gets the mouse position (in local control space).
        /// </summary>
        public Float2 MousePosition => _mousePosition;

        /// <summary>
        /// Initializes a new instance of the <see cref="SurfaceControl"/> class.
        /// </summary>
        /// <param name="context">The context.</param>
        /// <param name="width">The initial width.</param>
        /// <param name="height">The initial height.</param>
        protected SurfaceControl(VisjectSurfaceContext context, float width, float height)
        : base(0, 0, width, height)
        {
            ClipChildren = false;

            Surface = context.Surface;
            Context = context;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="SurfaceControl"/> class.
        /// </summary>
        /// <param name="surface">The surface.</param>
        /// <param name="bounds">The initial bounds.</param>
        protected SurfaceControl(VisjectSurface surface, ref Rectangle bounds)
        : base(bounds)
        {
            ClipChildren = false;

            Surface = surface;
        }

        /// <summary>
        /// Determines whether this element can be selected by the mouse at the specified location.
        /// </summary>
        /// <param name="location">The mouse location (in surface space).</param>
        /// <returns><c>true</c> if this instance can be selected by mouse at the specified location; otherwise, <c>false</c>.</returns>
        public abstract bool CanSelect(ref Float2 location);

        /// <summary>
        /// Determines whether selection rectangle is intersecting with the surface control area that can be selected.
        /// </summary>
        /// <param name="selectionRect">The selection rectangle (in surface space, not the control space).</param>
        /// <returns><c>true</c> if the selection rectangle is intersecting with the selectable parts of the control ; otherwise, <c>false</c>.</returns>
        public virtual bool IsSelectionIntersecting(ref Rectangle selectionRect)
        {
            return Bounds.Intersects(ref selectionRect);
        }

        /// <summary>
        /// Called when surface can edit state gets changed. Can be used to enable/disable UI elements that are surface data editing.
        /// </summary>
        /// <param name="canEdit">True if can edit surface data, otherwise false.</param>
        public virtual void OnSurfaceCanEditChanged(bool canEdit)
        {
        }

        /// <summary>
        /// Called when editor is showing secondary context menu. Can be used to inject custom options for node logic.
        /// </summary>
        /// <param name="menu">The menu.</param>
        /// <param name="location">The show location (in control space).</param>
        public virtual void OnShowSecondaryContextMenu(FlaxEditor.GUI.ContextMenu.ContextMenu menu, Float2 location)
        {
        }

        /// <summary>
        /// Called after <see cref="IsSelected"/> changes
        /// </summary>
        protected virtual void OnSelectionChanged()
        {
        }

        /// <summary>
        /// Called when control gets loaded and added to surface.
        /// </summary>
        public virtual void OnLoaded(SurfaceNodeActions action)
        {
        }

        /// <summary>
        /// Called when surface gets loaded and nodes boxes are connected.
        /// </summary>
        public virtual void OnSurfaceLoaded(SurfaceNodeActions action)
        {
            // Snap bounds (with ceil) when using grid snapping
            if (Surface != null && Surface.GridSnappingEnabled)
            {
                var bounds = Bounds;
                bounds.Location = Surface.SnapToGrid(bounds.Location, false);
                bounds.Size = Surface.SnapToGrid(bounds.Size, true);
                Bounds = bounds;
            }

            UpdateRectangles();
        }

        /// <summary>
        /// Called after adding the control to the surface after user spawn (eg. add comment, add new node, etc.).
        /// </summary>
        public virtual void OnSpawned(SurfaceNodeActions action)
        {
        }

        /// <summary>
        /// Called on removing the control from the surface after user delete/cut operation (eg. delete comment, cut node, etc.).
        /// </summary>
        public virtual void OnDeleted(SurfaceNodeActions action)
        {
            Dispose();
        }

        /// <summary>
        /// Updates the cached rectangles on control size change.
        /// </summary>
        protected abstract void UpdateRectangles();

        /// <inheritdoc />
        public override void OnMouseEnter(Float2 location)
        {
            _mousePosition = location;

            base.OnMouseEnter(location);
        }

        /// <inheritdoc />
        public override void OnMouseMove(Float2 location)
        {
            _mousePosition = location;

            base.OnMouseMove(location);
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            _mousePosition = Float2.Minimum;

            base.OnMouseLeave();
        }

        /// <inheritdoc />
        protected override void SetScaleInternal(ref Float2 scale)
        {
            base.SetScaleInternal(ref scale);

            UpdateRectangles();
        }

        /// <inheritdoc />
        protected override void OnSizeChanged()
        {
            base.OnSizeChanged();

            UpdateRectangles();
        }
    }
}
