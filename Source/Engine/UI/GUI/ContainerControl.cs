// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEngine.Assertions;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// Base class for all GUI controls that can contain child controls.
    /// </summary>
    public class ContainerControl : Control
    {
        /// <summary>
        /// The children collection.
        /// </summary>
        [NoSerialize]
        protected readonly List<Control> _children = new List<Control>();

        /// <summary>
        /// The contains focus cached flag.
        /// </summary>
        [NoSerialize]
        protected bool _containsFocus;

        /// <summary>
        /// Initializes a new instance of the <see cref="ContainerControl"/> class.
        /// </summary>
        public ContainerControl()
        {
            IsLayoutLocked = true;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ContainerControl"/> class.
        /// </summary>
        public ContainerControl(float x, float y, float width, float height)
        : base(x, y, width, height)
        {
            IsLayoutLocked = true;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ContainerControl"/> class.
        /// </summary>
        public ContainerControl(Vector2 location, Vector2 size)
        : base(location, size)
        {
            IsLayoutLocked = true;
        }

        /// <inheritdoc />
        public ContainerControl(Rectangle bounds)
        : base(bounds)
        {
            IsLayoutLocked = true;
        }

        /// <summary>
        /// Gets child controls list
        /// </summary>
        public List<Control> Children => _children;

        /// <summary>
        /// Gets amount of the children controls
        /// </summary>
        public int ChildrenCount => _children.Count;

        /// <summary>
        /// Checks if container has any child controls
        /// </summary>
        public bool HasChildren => _children.Count > 0;

        /// <summary>
        /// Gets a value indicating whether the control, or one of its child controls, currently has the input focus.
        /// </summary>
        public override bool ContainsFocus => _containsFocus;

        /// <summary>
        /// True if automatic updates for control layout are locked (useful when creating a lot of GUI control to prevent lags).
        /// </summary>
        [HideInEditor, NoSerialize]
        public bool IsLayoutLocked { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether apply clipping mask on children during rendering.
        /// </summary>
        [EditorOrder(530), Tooltip("If checked, control will apply clipping mask on children during rendering.")]
        public bool ClipChildren { get; set; } = true;

        /// <summary>
        /// Gets or sets a value indicating whether perform view culling on children during rendering.
        /// </summary>
        [EditorOrder(540), Tooltip("If checked, control will perform view culling on children during rendering.")]
        public bool CullChildren { get; set; } = true;

        /// <summary>
        /// Locks all child controls layout and itself.
        /// </summary>
        [NoAnimate]
        public virtual void LockChildrenRecursive()
        {
            // Itself
            IsLayoutLocked = true;

            // Every child container control
            for (int i = 0; i < _children.Count; i++)
            {
                if (_children[i] is ContainerControl child)
                    child.LockChildrenRecursive();
            }
        }

        /// <summary>
        /// Unlocks all the child controls layout and itself.
        /// </summary>
        [NoAnimate]
        public virtual void UnlockChildrenRecursive()
        {
            // Itself
            IsLayoutLocked = false;

            // Every child container control
            for (int i = 0; i < _children.Count; i++)
            {
                if (_children[i] is ContainerControl child)
                    child.UnlockChildrenRecursive();
            }
        }

        /// <summary>
        /// Unlinks all the child controls.
        /// </summary>
        [NoAnimate]
        public virtual void RemoveChildren()
        {
            bool wasLayoutLocked = IsLayoutLocked;
            IsLayoutLocked = true;

            // Delete children
            while (_children.Count > 0)
            {
                _children[0].Parent = null;
            }

            IsLayoutLocked = wasLayoutLocked;
            PerformLayout();
        }

        /// <summary>
        /// Removes and disposes all the child controls
        /// </summary>
        public virtual void DisposeChildren()
        {
            bool wasLayoutLocked = IsLayoutLocked;
            IsLayoutLocked = true;

            // Delete children
            while (_children.Count > 0)
            {
                _children[0].Dispose();
            }

            IsLayoutLocked = wasLayoutLocked;
            PerformLayout();
        }

        /// <summary>
        /// Creates a new control and adds it to the container.
        /// </summary>
        /// <returns>The added control.</returns>
        public T AddChild<T>() where T : Control
        {
            var child = (T)Activator.CreateInstance(typeof(T));
            child.Parent = this;
            return child;
        }

        /// <summary>
        /// Adds the control to the container.
        /// </summary>
        /// <param name="child">The control to add.</param>
        /// <returns>The added control.</returns>
        public T AddChild<T>(T child) where T : Control
        {
            if (child == null)
                throw new ArgumentNullException();
            if (child.Parent == this && _children.Contains(child))
                throw new InvalidOperationException("Argument child cannot be added, if current container is already its parent.");

            // Set child new parent
            child.Parent = this;

            return child;
        }

        /// <summary>
        /// Removes control from the container.
        /// </summary>
        /// <param name="child">The control to remove.</param>
        public void RemoveChild(Control child)
        {
            if (child == null)
                throw new ArgumentNullException();
            if (child.Parent != this)
                throw new InvalidOperationException("Argument child cannot be removed, if current container is not its parent.");

            // Unlink
            child.Parent = null;
        }

        /// <summary>
        /// Gets child control at given index.
        /// </summary>
        /// <param name="index">The control index.</param>
        /// <returns>The child control.</returns>
        public Control GetChild(int index)
        {
            return _children[index];
        }

        /// <summary>
        /// Searches for a child control of a specific type. If there are multiple controls matching the type, only the first one found is returned.
        /// </summary>
        /// <typeparam name="T">The type of the control to search for. Includes any controls derived from the type.</typeparam>
        /// <returns>The control instance if found, otherwise null.</returns>
        public T GetChild<T>() where T : Control
        {
            var type = typeof(T);
            for (int i = 0; i < _children.Count; i++)
            {
                var ct = _children[i].GetType();
                if (type.IsAssignableFrom(ct))
                    return (T)_children[i];
            }
            return null;
        }

        /// <summary>
        /// Gets zero-based index in the list of control children.
        /// </summary>
        /// <param name="child">The child control.</param>
        /// <returns>The zero-based index in the list of control children.</returns>
        public int GetChildIndex(Control child)
        {
            return _children.IndexOf(child);
        }

        internal void ChangeChildIndex(Control child, int newIndex)
        {
            int oldIndex = _children.IndexOf(child);
            if (oldIndex == newIndex)
                return;
            _children.RemoveAt(oldIndex);

            // Check if index is invalid
            if (newIndex < 0 || newIndex >= _children.Count)
            {
                // Append at the end
                _children.Add(child);
            }
            else
            {
                // Change order
                _children.Insert(newIndex, child);
            }

            PerformLayout();
        }

        /// <summary>
        /// Tries to find any child control at given point in control local coordinates.
        /// </summary>
        /// <param name="point">The local point to check.</param>
        /// <returns>The found control index or -1 if failed.</returns>
        public int GetChildIndexAt(Vector2 point)
        {
            int result = -1;
            for (int i = _children.Count - 1; i >= 0; i--)
            {
                var child = _children[i];

                // Check collision
                if (IntersectsChildContent(child, point, out var childLocation))
                {
                    result = i;
                    break;
                }
            }
            return result;
        }

        /// <summary>
        /// Tries to find any child control at given point in control local coordinates
        /// </summary>
        /// <param name="point">The local point to check.</param>
        /// <returns>The found control or null.</returns>
        public Control GetChildAt(Vector2 point)
        {
            Control result = null;
            for (int i = _children.Count - 1; i >= 0; i--)
            {
                var child = _children[i];

                // Check collision
                if (IntersectsChildContent(child, point, out var childLocation))
                {
                    result = child;
                    break;
                }
            }
            return result;
        }

        /// <summary>
        /// Tries to find valid child control at given point in control local coordinates. Uses custom callback method to test controls to pick.
        /// </summary>
        /// <param name="point">The local point to check.</param>
        /// <param name="isValid">The control validation callback.</param>
        /// <returns>The found control or null.</returns>
        public Control GetChildAt(Vector2 point, Func<Control, bool> isValid)
        {
            if (isValid == null)
                throw new ArgumentNullException(nameof(isValid));

            Control result = null;
            for (int i = _children.Count - 1; i >= 0; i--)
            {
                var child = _children[i];

                // Check collision
                if (isValid(child) && IntersectsChildContent(child, point, out var childLocation))
                {
                    result = child;
                    break;
                }
            }
            return result;
        }

        /// <summary>
        /// Tries to find lowest child control at given point in control local coordinates.
        /// </summary>
        /// <param name="point">The local point to check.</param>
        /// <returns>The found control or null.</returns>
        public Control GetChildAtRecursive(Vector2 point)
        {
            Control result = null;
            for (int i = _children.Count - 1; i >= 0; i--)
            {
                var child = _children[i];

                // Check collision
                if (IntersectsChildContent(child, point, out var childLocation))
                {
                    var containerControl = child as ContainerControl;
                    var childAtRecursive = containerControl?.GetChildAtRecursive(childLocation);
                    if (childAtRecursive != null)
                    {
                        child = childAtRecursive;
                    }
                    result = child;
                }
            }
            return result;
        }

        /// <summary>
        /// Gets rectangle in local control coordinates with area for controls (without scroll bars, anchored controls, etc.).
        /// </summary>
        /// <returns>The rectangle in local control coordinates with area for controls (without scroll bars etc.).</returns>
        public Rectangle GetClientArea()
        {
            GetDesireClientArea(out var clientArea);
            return clientArea;
        }

        /// <summary>
        /// Sort child controls list
        /// </summary>
        [NoAnimate]
        public void SortChildren()
        {
            _children.Sort();
            PerformLayout();
        }

        /// <summary>
        /// Sort children using recursion
        /// </summary>
        [NoAnimate]
        public void SortChildrenRecursive()
        {
            SortChildren();

            for (int i = 0; i < _children.Count; i++)
            {
                var child = _children[i] as ContainerControl;
                child?.SortChildrenRecursive();
            }
        }

        /// <summary>
        /// Called when child control gets resized.
        /// </summary>
        /// <param name="control">The resized control.</param>
        public virtual void OnChildResized(Control control)
        {
        }

        /// <summary>
        /// Called when children collection gets changed (child added or removed).
        /// </summary>
        [NoAnimate]
        public virtual void OnChildrenChanged()
        {
            // Check if control isn't during disposing state
            if (!IsDisposing)
            {
                // Arrange child controls
                PerformLayout();
            }
        }

        #region Internal Events

        /// <inheritdoc />
        internal override void CacheRootHandle()
        {
            base.CacheRootHandle();

            for (int i = 0; i < _children.Count; i++)
            {
                _children[i].CacheRootHandle();
            }
        }

        /// <summary>
        /// Adds a child control to the container.
        /// </summary>
        /// <param name="child">The control to add.</param>
        internal virtual void AddChildInternal(Control child)
        {
            Assert.IsNotNull(child, "Invalid control.");

            // Add child
            _children.Add(child);

            OnChildrenChanged();
        }

        /// <summary>
        /// Removes a child control from this container.
        /// </summary>
        /// <param name="child">The control to remove.</param>
        internal virtual void RemoveChildInternal(Control child)
        {
            Assert.IsNotNull(child, "Invalid control.");

            // Remove child
            _children.Remove(child);

            OnChildrenChanged();
        }

        /// <summary>
        /// Gets the desire client area rectangle for all the controls.
        /// </summary>
        /// <param name="rect">The client area rectangle for child controls.</param>
        public virtual void GetDesireClientArea(out Rectangle rect)
        {
            rect = new Rectangle(Vector2.Zero, Size);
        }

        /// <summary>
        /// Checks if given point in this container control space intersects with the child control content.
        /// Also calculates result location in child control space which can be used to feed control with event at that point.
        /// </summary>
        /// <param name="child">The child control to check.</param>
        /// <param name="location">The location in this container control space.</param>
        /// <param name="childSpaceLocation">The output location in child control space.</param>
        /// <returns>True if point is over the control content, otherwise false.</returns>
        public virtual bool IntersectsChildContent(Control child, Vector2 location, out Vector2 childSpaceLocation)
        {
            return child.IntersectsContent(ref location, out childSpaceLocation);
        }

        /// <summary>
        /// Update contain focus state and all it's children
        /// </summary>
        protected void UpdateContainsFocus()
        {
            // Get current state and update all children
            bool result = base.ContainsFocus;

            for (int i = 0; i < _children.Count; i++)
            {
                if (_children[i] is ContainerControl child)
                    child.UpdateContainsFocus();

                if (_children[i].ContainsFocus)
                    result = true;
            }

            // Check if state has been changed
            if (result != _containsFocus)
            {
                // Cache flag
                _containsFocus = result;

                // Fire event
                if (result)
                {
                    OnStartContainsFocus();
                }
                else
                {
                    OnEndContainsFocus();
                }
            }
        }

        /// <summary>
        /// Updates child controls bounds.
        /// </summary>
        protected void UpdateChildrenBounds()
        {
            for (int i = 0; i < _children.Count; i++)
            {
                _children[i].UpdateBounds();
            }
        }

        /// <summary>
        /// Perform layout for that container control before performing it for child controls.
        /// </summary>
        protected virtual void PerformLayoutBeforeChildren()
        {
            UpdateChildrenBounds();
        }

        /// <summary>
        /// Perform layout for that container control after performing it for child controls.
        /// </summary>
        protected virtual void PerformLayoutAfterChildren()
        {
        }

        #endregion

        #region Control

        /// <inheritdoc />
        public override void OnDestroy()
        {
            // Steal focus from children
            if (ContainsFocus)
            {
                Focus();
            }

            base.OnDestroy();

            // Pass event further
            for (int i = 0; i < _children.Count; i++)
            {
                _children[i].OnDestroy();
            }
            _children.Clear();
        }

        /// <inheritdoc />
        public override bool IsMouseOver
        {
            get
            {
                if (base.IsMouseOver)
                    return true;

                for (int i = 0; i < _children.Count && _children.Count > 0; i++)
                {
                    if (_children[i].IsMouseOver)
                        return true;
                }

                return false;
            }
        }

        /// <inheritdoc />
        public override bool IsTouchOver
        {
            get
            {
                if (base.IsTouchOver)
                    return true;
                for (int i = 0; i < _children.Count && _children.Count > 0; i++)
                {
                    if (_children[i].IsTouchOver)
                        return true;
                }
                return false;
            }
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            base.Update(deltaTime);

            // Update all enabled child controls
            for (int i = 0; i < _children.Count; i++)
            {
                if (_children[i].Enabled)
                {
                    _children[i].Update(deltaTime);
                }
            }
        }

        /// <summary>
        /// Draw the control and the children.
        /// </summary>
        public override void Draw()
        {
            DrawSelf();

            if (ClipChildren)
            {
                GetDesireClientArea(out var clientArea);
                Render2D.PushClip(ref clientArea);
                DrawChildren();
                Render2D.PopClip();
            }
            else
            {
                DrawChildren();
            }
        }

        /// <summary>
        /// Draws the control.
        /// </summary>
        public virtual void DrawSelf()
        {
            base.Draw();
        }

        /// <summary>
        /// Draws the children. Can be overridden to provide some customizations. Draw is performed with applied clipping mask for the client area.
        /// </summary>
        protected virtual void DrawChildren()
        {
            // Draw all visible child controls
            if (CullChildren)
            {
                Render2D.PeekClip(out var globalClipping);
                Render2D.PeekTransform(out var globalTransform);
                for (int i = 0; i < _children.Count; i++)
                {
                    var child = _children[i];
                    if (child.Visible)
                    {
                        Matrix3x3.Multiply(ref child._cachedTransform, ref globalTransform, out var globalChildTransform);
                        var childGlobalRect = new Rectangle(globalChildTransform.M31, globalChildTransform.M32, child.Width * globalChildTransform.M11, child.Height * globalChildTransform.M22);
                        if (globalClipping.Intersects(ref childGlobalRect))
                        {
                            Render2D.PushTransform(ref child._cachedTransform);
                            child.Draw();
                            Render2D.PopTransform();
                        }
                    }
                }
            }
            else
            {
                for (int i = 0; i < _children.Count; i++)
                {
                    var child = _children[i];
                    if (child.Visible)
                    {
                        Render2D.PushTransform(ref child._cachedTransform);
                        child.Draw();
                        Render2D.PopTransform();
                    }
                }
            }
        }

        /// <inheritdoc />
        public override void PerformLayout(bool force = false)
        {
            if (IsLayoutLocked && !force)
                return;

            bool wasLocked = IsLayoutLocked;
            if (!wasLocked)
                LockChildrenRecursive();

            PerformLayoutBeforeChildren();

            for (int i = 0; i < _children.Count; i++)
                _children[i].PerformLayout(true);

            PerformLayoutAfterChildren();

            if (!wasLocked)
                UnlockChildrenRecursive();
        }

        /// <inheritdoc />
        public override void OnMouseEnter(Vector2 location)
        {
            // Check all children collisions with mouse and fire events for them
            for (int i = _children.Count - 1; i >= 0 && _children.Count > 0; i--)
            {
                var child = _children[i];
                if (child.Visible && child.Enabled)
                {
                    // Fire event
                    if (IntersectsChildContent(child, location, out var childLocation))
                    {
                        // Enter
                        child.OnMouseEnter(childLocation);
                    }
                }
            }

            base.OnMouseEnter(location);
        }

        /// <inheritdoc />
        public override void OnMouseMove(Vector2 location)
        {
            // Check all children collisions with mouse and fire events for them
            for (int i = _children.Count - 1; i >= 0 && _children.Count > 0; i--)
            {
                var child = _children[i];
                if (child.Visible && child.Enabled)
                {
                    // Fire events
                    if (IntersectsChildContent(child, location, out var childLocation))
                    {
                        if (child.IsMouseOver)
                        {
                            // Move
                            child.OnMouseMove(childLocation);
                        }
                        else
                        {
                            // Enter
                            child.OnMouseEnter(childLocation);
                        }
                    }
                    else if (child.IsMouseOver)
                    {
                        // Leave
                        child.OnMouseLeave();
                    }
                }
            }

            base.OnMouseMove(location);
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            // Check all children collisions with mouse and fire events for them
            for (int i = 0; i < _children.Count && _children.Count > 0; i++)
            {
                var child = _children[i];
                if (child.Visible && child.Enabled && child.IsMouseOver)
                {
                    // Leave
                    child.OnMouseLeave();
                }
            }

            base.OnMouseLeave();
        }

        /// <inheritdoc />
        public override bool OnMouseWheel(Vector2 location, float delta)
        {
            // Check all children collisions with mouse and fire events for them
            for (int i = _children.Count - 1; i >= 0 && _children.Count > 0; i--)
            {
                var child = _children[i];
                if (child.Visible && child.Enabled)
                {
                    // Fire events
                    if (IntersectsChildContent(child, location, out var childLocation))
                    {
                        // Wheel
                        if (child.OnMouseWheel(childLocation, delta))
                        {
                            return true;
                        }
                    }
                }
            }

            return base.OnMouseWheel(location, delta);
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Vector2 location, MouseButton button)
        {
            // Check all children collisions with mouse and fire events for them
            for (int i = _children.Count - 1; i >= 0 && _children.Count > 0; i--)
            {
                var child = _children[i];
                if (child.Visible && child.Enabled)
                {
                    // Fire event
                    if (IntersectsChildContent(child, location, out var childLocation))
                    {
                        // Send event further
                        if (child.OnMouseDown(childLocation, button))
                        {
                            return true;
                        }
                    }
                }
            }

            return base.OnMouseDown(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Vector2 location, MouseButton button)
        {
            // Check all children collisions with mouse and fire events for them
            for (int i = _children.Count - 1; i >= 0 && _children.Count > 0; i--)
            {
                var child = _children[i];
                if (child.Visible && child.Enabled)
                {
                    // Fire event
                    if (IntersectsChildContent(child, location, out var childLocation))
                    {
                        // Send event further
                        if (child.OnMouseUp(childLocation, button))
                        {
                            return true;
                        }
                    }
                }
            }

            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseDoubleClick(Vector2 location, MouseButton button)
        {
            // Check all children collisions with mouse and fire events for them
            for (int i = _children.Count - 1; i >= 0 && _children.Count > 0; i--)
            {
                var child = _children[i];
                if (child.Visible && child.Enabled)
                {
                    // Fire event
                    if (IntersectsChildContent(child, location, out var childLocation))
                    {
                        // Send event further
                        if (child.OnMouseDoubleClick(childLocation, button))
                        {
                            return true;
                        }
                    }
                }
            }

            return base.OnMouseDoubleClick(location, button);
        }

        /// <inheritdoc />
        public override bool IsTouchPointerOver(int pointerId)
        {
            if (base.IsTouchPointerOver(pointerId))
                return true;

            for (int i = 0; i < _children.Count && _children.Count > 0; i++)
            {
                if (_children[i].IsTouchPointerOver(pointerId))
                    return true;
            }

            return false;
        }

        /// <inheritdoc />
        public override void OnTouchEnter(Vector2 location, int pointerId)
        {
            for (int i = _children.Count - 1; i >= 0 && _children.Count > 0; i--)
            {
                var child = _children[i];
                if (child.Visible && child.Enabled && !child.IsTouchPointerOver(pointerId))
                {
                    if (IntersectsChildContent(child, location, out var childLocation))
                    {
                        child.OnTouchEnter(childLocation, pointerId);
                    }
                }
            }

            base.OnTouchEnter(location, pointerId);
        }

        /// <inheritdoc />
        public override bool OnTouchDown(Vector2 location, int pointerId)
        {
            for (int i = _children.Count - 1; i >= 0 && _children.Count > 0; i--)
            {
                var child = _children[i];
                if (child.Visible && child.Enabled)
                {
                    if (IntersectsChildContent(child, location, out var childLocation))
                    {
                        if (!child.IsTouchPointerOver(pointerId))
                        {
                            child.OnTouchEnter(location, pointerId);
                        }
                        if (child.OnTouchDown(childLocation, pointerId))
                        {
                            return true;
                        }
                    }
                }
            }

            return base.OnTouchDown(location, pointerId);
        }

        /// <inheritdoc />
        public override void OnTouchMove(Vector2 location, int pointerId)
        {
            for (int i = _children.Count - 1; i >= 0 && _children.Count > 0; i--)
            {
                var child = _children[i];
                if (child.Visible && child.Enabled)
                {
                    if (IntersectsChildContent(child, location, out var childLocation))
                    {
                        if (child.IsTouchPointerOver(pointerId))
                        {
                            child.OnTouchMove(childLocation, pointerId);
                        }
                        else
                        {
                            child.OnTouchEnter(childLocation, pointerId);
                        }
                    }
                    else if (child.IsTouchPointerOver(pointerId))
                    {
                        child.OnTouchLeave(pointerId);
                    }
                }
            }

            base.OnTouchMove(location, pointerId);
        }

        /// <inheritdoc />
        public override bool OnTouchUp(Vector2 location, int pointerId)
        {
            for (int i = _children.Count - 1; i >= 0 && _children.Count > 0; i--)
            {
                var child = _children[i];
                if (child.Visible && child.Enabled && child.IsTouchPointerOver(pointerId))
                {
                    if (IntersectsChildContent(child, location, out var childLocation))
                    {
                        if (child.OnTouchUp(childLocation, pointerId))
                        {
                            return true;
                        }
                    }
                }
            }

            return base.OnTouchUp(location, pointerId);
        }

        /// <inheritdoc />
        public override void OnTouchLeave(int pointerId)
        {
            for (int i = 0; i < _children.Count && _children.Count > 0; i++)
            {
                var child = _children[i];
                if (child.Visible && child.Enabled && child.IsTouchPointerOver(pointerId))
                {
                    child.OnTouchLeave(pointerId);
                }
            }

            base.OnTouchLeave(pointerId);
        }

        /// <inheritdoc />
        public override bool OnCharInput(char c)
        {
            for (int i = 0; i < _children.Count && _children.Count > 0; i++)
            {
                var child = _children[i];
                if (child.Enabled && child.ContainsFocus)
                {
                    return child.OnCharInput(c);
                }
            }
            return false;
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            for (int i = 0; i < _children.Count && _children.Count > 0; i++)
            {
                var child = _children[i];
                if (child.Enabled && child.ContainsFocus)
                {
                    return child.OnKeyDown(key);
                }
            }
            return false;
        }

        /// <inheritdoc />
        public override void OnKeyUp(KeyboardKeys key)
        {
            for (int i = 0; i < _children.Count && _children.Count > 0; i++)
            {
                var child = _children[i];
                if (child.Enabled && child.ContainsFocus)
                {
                    child.OnKeyUp(key);
                    break;
                }
            }
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragEnter(ref Vector2 location, DragData data)
        {
            // Base
            var result = base.OnDragEnter(ref location, data);

            // Check all children collisions with mouse and fire events for them
            for (int i = _children.Count - 1; i >= 0 && _children.Count > 0; i--)
            {
                var child = _children[i];
                if (child.Visible && child.Enabled)
                {
                    // Fire event
                    if (IntersectsChildContent(child, location, out var childLocation))
                    {
                        // Enter
                        result = child.OnDragEnter(ref childLocation, data);
                        if (result != DragDropEffect.None)
                            break;
                    }
                }
            }

            return result;
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragMove(ref Vector2 location, DragData data)
        {
            // Base
            var result = base.OnDragMove(ref location, data);

            // Check all children collisions with mouse and fire events for them
            for (int i = _children.Count - 1; i >= 0 && _children.Count > 0; i--)
            {
                var child = _children[i];
                if (child.Visible && child.Enabled)
                {
                    // Fire events
                    if (IntersectsChildContent(child, location, out var childLocation))
                    {
                        if (child.IsDragOver)
                        {
                            // Move
                            var tmpResult = child.OnDragMove(ref childLocation, data);
                            if (tmpResult != DragDropEffect.None)
                                result = tmpResult;
                        }
                        else
                        {
                            // Enter
                            var tmpResult = child.OnDragEnter(ref childLocation, data);
                            if (tmpResult != DragDropEffect.None)
                                result = tmpResult;
                        }
                    }
                    else if (child.IsDragOver)
                    {
                        // Leave
                        child.OnDragLeave();
                    }
                }
            }

            return result;
        }

        /// <inheritdoc />
        public override void OnDragLeave()
        {
            // Base
            base.OnDragLeave();

            // Check all children collisions with mouse and fire events for them
            for (int i = 0; i < _children.Count && _children.Count > 0; i++)
            {
                var child = _children[i];
                if (child.IsDragOver)
                {
                    // Leave
                    child.OnDragLeave();
                }
            }
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragDrop(ref Vector2 location, DragData data)
        {
            // Base
            var result = base.OnDragDrop(ref location, data);

            // Check all children collisions with mouse and fire events for them
            for (int i = _children.Count - 1; i >= 0 && _children.Count > 0; i--)
            {
                var child = _children[i];
                if (child.Visible && child.Enabled)
                {
                    // Fire event
                    if (IntersectsChildContent(child, location, out var childLocation))
                    {
                        // Enter
                        result = child.OnDragDrop(ref childLocation, data);
                        if (result != DragDropEffect.None)
                            break;
                    }
                }
            }

            return result;
        }

        /// <inheritdoc />
        protected override void OnSizeChanged()
        {
            // Lock updates to prevent additional layout calculations
            bool wasLayoutLocked = IsLayoutLocked;
            IsLayoutLocked = true;

            // Base
            base.OnSizeChanged();

            // Fire event
            for (int i = 0; i < _children.Count; i++)
            {
                _children[i].OnParentResized();
            }

            // Restore state
            IsLayoutLocked = wasLayoutLocked;

            // Arrange child controls
            PerformLayout();
        }

        #endregion


        /// <inheritdoc />
        public override List<Control> GetAutoNavControls()
        {
            List<Control> list = new List<Control>();
            foreach (Control c in Children)
            {
                list.AddRange(c.GetAutoNavControls());
            }
            list.AddRange(base.GetAutoNavControls());
            return list;
        }
    }
}
