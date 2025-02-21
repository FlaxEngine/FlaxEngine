// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

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
        /// The layout locking flag.
        /// </summary>
        [NoSerialize]
        protected bool _isLayoutLocked;

        private bool _clipChildren = true;
        private bool _cullChildren = true;

        /// <summary>
        /// Initializes a new instance of the <see cref="ContainerControl"/> class.
        /// </summary>
        public ContainerControl()
        {
            _isLayoutLocked = true;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ContainerControl"/> class.
        /// </summary>
        public ContainerControl(float x, float y, float width, float height)
        : base(x, y, width, height)
        {
            _isLayoutLocked = true;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ContainerControl"/> class.
        /// </summary>
        public ContainerControl(Float2 location, Float2 size)
        : base(location, size)
        {
            _isLayoutLocked = true;
        }

        /// <inheritdoc />
        public ContainerControl(Rectangle bounds)
        : base(bounds)
        {
            _isLayoutLocked = true;
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
        [HideInEditor, NoSerialize, NoAnimate]
        public bool IsLayoutLocked
        {
            get => _isLayoutLocked;
            set => _isLayoutLocked = value;
        }

        /// <summary>
        /// Gets or sets a value indicating whether apply clipping mask on children during rendering.
        /// </summary>
        [EditorOrder(530), Tooltip("If checked, control will apply clipping mask on children during rendering.")]
        public bool ClipChildren
        {
            get => _clipChildren;
            set => _clipChildren = value;
        }

        /// <summary>
        /// Gets or sets a value indicating whether perform view culling on children during rendering.
        /// </summary>
        [EditorOrder(540), Tooltip("If checked, control will perform view culling on children during rendering.")]
        public bool CullChildren
        {
            get => _cullChildren;
            set => _cullChildren = value;
        }

        /// <summary>
        /// Locks all child controls layout and itself.
        /// </summary>
        [NoAnimate]
        public void LockChildrenRecursive()
        {
            _isLayoutLocked = true;
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
        public void UnlockChildrenRecursive()
        {
            _isLayoutLocked = false;
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
            bool wasLayoutLocked = _isLayoutLocked;
            _isLayoutLocked = true;

            // Delete children
            while (_children.Count > 0)
            {
                _children[0].Parent = null;
            }

            _isLayoutLocked = wasLayoutLocked;
            PerformLayout();
        }

        /// <summary>
        /// Removes and disposes all the child controls
        /// </summary>
        public virtual void DisposeChildren()
        {
            bool wasLayoutLocked = _isLayoutLocked;
            _isLayoutLocked = true;

            // Delete children
            while (_children.Count > 0)
            {
                _children[0].Dispose();
            }

            _isLayoutLocked = wasLayoutLocked;
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
            if (oldIndex == newIndex || oldIndex == -1)
                return;
            _children.RemoveAt(oldIndex);

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
        public int GetChildIndexAt(Float2 point)
        {
            int result = -1;
            for (int i = _children.Count - 1; i >= 0; i--)
            {
                var child = _children[i];
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
        public Control GetChildAt(Float2 point)
        {
            Control result = null;
            for (int i = _children.Count - 1; i >= 0; i--)
            {
                var child = _children[i];
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
        public Control GetChildAt(Float2 point, Func<Control, bool> isValid)
        {
            if (isValid == null)
                throw new ArgumentNullException(nameof(isValid));
            Control result = null;
            for (int i = _children.Count - 1; i >= 0; i--)
            {
                var child = _children[i];
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
        public Control GetChildAtRecursive(Float2 point)
        {
            Control result = null;
            for (int i = _children.Count - 1; i >= 0; i--)
            {
                var child = _children[i];
                if (child.Visible && IntersectsChildContent(child, point, out var childLocation))
                {
                    var containerControl = child as ContainerControl;
                    var childAtRecursive = containerControl?.GetChildAtRecursive(childLocation);
                    if (childAtRecursive != null && childAtRecursive.Visible)
                    {
                        child = childAtRecursive;
                    }
                    result = child;
                    break;
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
            if (Parent == child)
                throw new InvalidOperationException();

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
            rect = new Rectangle(Float2.Zero, Size);
        }

        /// <summary>
        /// Checks if given point in this container control space intersects with the child control content.
        /// Also calculates result location in child control space which can be used to feed control with event at that point.
        /// </summary>
        /// <param name="child">The child control to check.</param>
        /// <param name="location">The location in this container control space.</param>
        /// <param name="childSpaceLocation">The output location in child control space.</param>
        /// <returns>True if point is over the control content, otherwise false.</returns>
        public virtual bool IntersectsChildContent(Control child, Float2 location, out Float2 childSpaceLocation)
        {
            return child.IntersectsContent(ref location, out childSpaceLocation);
        }

        #region Navigation

        /// <inheritdoc />
        public override Control OnNavigate(NavDirection direction, Float2 location, Control caller, List<Control> visited)
        {
            // Try to focus itself first (only if navigation focus can enter this container)
            if (AutoFocus && !ContainsFocus)
                return this;

            // Try to focus children
            if (_children.Count != 0 && !visited.Contains(this))
            {
                visited.Add(this);

                // Perform automatic navigation based on the layout
                var result = NavigationRaycast(direction, location, visited);
                var rightMostLocation = location;
                if (result == null && (direction == NavDirection.Next || direction == NavDirection.Previous))
                {
                    // Try wrap the navigation over the layout based on the direction
                    var visitedWrap = new List<Control>(visited);
                    result = NavigationWrap(direction, location, visitedWrap, out rightMostLocation);
                }
                if (result != null)
                {
                    // HACK: only the 'previous' direction needs the rightMostLocation so i used a ternary conditional operator.
                    // The rightMostLocation can probably become a 'desired raycast origin' that gets calculated correctly in the NavigationWrap method.
                    var useLocation = direction == NavDirection.Previous ? rightMostLocation : location;
                    result = result.OnNavigate(direction, result.PointFromParent(useLocation), this, visited);
                    if (result != null)
                        return result;
                }
            }

            // Try to focus itself
            if (AutoFocus && !IsFocused || caller == this)
                return this;

            // Route navigation to parent
            var parent = Parent;
            if (AutoFocus && Visible)
            {
                // Focusable container controls use own nav origin instead of the provided one
                location = GetNavOrigin(direction);
            }
            return parent?.OnNavigate(direction, PointToParent(location), caller, visited);
        }

        /// <summary>
        /// Checks if this container control can more with focus navigation into the given child control.
        /// </summary>
        /// <param name="child">The child.</param>
        /// <returns>True if can navigate to it, otherwise false.</returns>
        protected virtual bool CanNavigateChild(Control child)
        {
            return !child.IsFocused && child.Enabled && child.Visible && CanGetAutoFocus(child);
        }

        /// <summary>
        /// Wraps the navigation over the layout.
        /// </summary>
        /// <param name="direction">The navigation direction.</param>
        /// <param name="location">The navigation start location (in the control-space).</param>
        /// <param name="visited">The list with visited controls. Used to skip recursive navigation calls when doing traversal across the UI hierarchy.</param>
        /// <param name="rightMostLocation">Returns the rightmost location of the parent container for the raycast used by the child container</param>
        /// <returns>The target navigation control or null if didn't performed any navigation.</returns>
        protected virtual Control NavigationWrap(NavDirection direction, Float2 location, List<Control> visited, out Float2 rightMostLocation)
        {
            // This searches form a child that calls this navigation event (see Control.OnNavigate) to determinate the layout wrapping size based on that child size
            var currentChild = RootWindow?.FocusedControl;
            visited.Add(this);
            if (currentChild != null)
            {
                var layoutSize = currentChild.Size;
                var predictedLocation = Float2.Minimum;
                switch (direction)
                {
                case NavDirection.Next:
                    predictedLocation = new Float2(0, location.Y + layoutSize.Y);
                    break;
                case NavDirection.Previous:
                    predictedLocation = new Float2(Size.X, location.Y - layoutSize.Y);
                    break;
                }
                if (new Rectangle(Float2.Zero, Size).Contains(ref predictedLocation))
                {
                    var result = NavigationRaycast(direction, predictedLocation, visited);
                    if (result != null)
                    {
                        rightMostLocation = predictedLocation;
                        return result;   
                    }
                }
            }
            rightMostLocation = location;
            return Parent?.NavigationWrap(direction, PointToParent(ref location), visited, out rightMostLocation);
        }

        private static bool CanGetAutoFocus(Control c)
        {
            if (c.AutoFocus)
                return true;
            if (c is ContainerControl cc)
            {
                for (int i = 0; i < cc.Children.Count; i++)
                {
                    if (cc.CanNavigateChild(cc.Children[i]))
                        return true;
                }
            }
            return false;
        }

        private Control NavigationRaycast(NavDirection direction, Float2 location, List<Control> visited)
        {
            Float2 uiDir1 = Float2.Zero, uiDir2 = Float2.Zero;
            switch (direction)
            {
            case NavDirection.Up:
                uiDir1 = uiDir2 = new Float2(0, -1);
                break;
            case NavDirection.Down:
                uiDir1 = uiDir2 = new Float2(0, 1);
                break;
            case NavDirection.Left:
                uiDir1 = uiDir2 = new Float2(-1, 0);
                break;
            case NavDirection.Right:
                uiDir1 = uiDir2 = new Float2(1, 0);
                break;
            case NavDirection.Next:
                uiDir1 = new Float2(1, 0);
                uiDir2 = new Float2(0, 1);
                break;
            case NavDirection.Previous:
                uiDir1 = new Float2(-1, 0);
                uiDir2 = new Float2(0, -1);
                break;
            }
            Control result = null;
            var minDistance = float.MaxValue;
            for (var i = 0; i < _children.Count; i++)
            {
                var child = _children[i];
                if (!CanNavigateChild(child) || visited.Contains(child))
                    continue;
                var childNavLocation = child.Center;
                var childBounds = child.Bounds;
                var childNavDirection = Float2.Normalize(childNavLocation - location);
                var childNavCoherence1 = Float2.Dot(ref uiDir1, ref childNavDirection);
                var childNavCoherence2 = Float2.Dot(ref uiDir2, ref childNavDirection);
                var distance = Rectangle.Distance(childBounds, location);
                if (childNavCoherence1 > Mathf.Epsilon && childNavCoherence2 > Mathf.Epsilon && distance < minDistance)
                {
                    minDistance = distance;
                    result = child;
                }
            }
            return result;
        }

        #endregion

        /// <summary>
        /// Update contain focus state and all it's children
        /// </summary>
        protected void UpdateContainsFocus()
        {
            // Get current state and update all children
            bool result = base.ContainsFocus;

            for (int i = 0; i < _children.Count; i++)
            {
                var control = _children[i];
                if (control is ContainerControl child)
                    child.UpdateContainsFocus();
                if (control.ContainsFocus)
                    result = true;
            }

            // Check if state has been changed
            if (result != _containsFocus)
            {
                _containsFocus = result;
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

            // Disable layout
            if (!_isLayoutLocked)
            {
                LockChildrenRecursive();
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
        public override bool IsTouchOver
        {
            get
            {
                if (base.IsTouchOver)
                    return true;
                for (int i = 0; i < _children.Count; i++)
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

        /// <inheritdoc />
        public override void ClearState()
        {
            base.ClearState();

            // Clear state for any nested controls
            for (int i = 0; i < _children.Count; i++)
            {
                var child = _children[i];
                //if (child.Enabled && child.Enabled)
                    child.ClearState();
            }
        }

        /// <summary>
        /// Draw the control and the children.
        /// </summary>
        public override void Draw()
        {
            DrawSelf();

            if (_clipChildren)
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
            var children = _children;
            if (_cullChildren)
            {
                Render2D.PeekClip(out var globalClipping);
                Render2D.PeekTransform(out var globalTransform);
                for (int i = 0; i < children.Count; i++)
                {
                    var child = children[i];
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
                for (int i = 0; i < children.Count; i++)
                {
                    var child = children[i];
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
        public override bool ContainsPoint(ref Float2 location, bool precise = false)
        {
            if (precise && this.GetType() == typeof(ContainerControl) && BackgroundColor.A <= 0.0f) // Go through transparency
                return false;
            return base.ContainsPoint(ref location, precise);
        }

        /// <inheritdoc />
        public override void PerformLayout(bool force = false)
        {
            if (_isLayoutLocked && !force)
                return;

            bool wasLocked = _isLayoutLocked;
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
        public override bool RayCast(ref Float2 location, out Control hit)
        {
            if (RayCastChildren(ref location, out hit))
                return true;
            return base.RayCast(ref location, out hit);
        }

        internal bool RayCastChildren(ref Float2 location, out Control hit)
        {
            for (int i = _children.Count - 1; i >= 0 && _children.Count > 0; i--)
            {
                var child = _children[i];
                if (child.Visible)
                {
                    IntersectsChildContent(child, location, out var childLocation);
                    if (child.RayCast(ref childLocation, out hit))
                        return true;
                }
            }
            hit = null;
            return false;
        }

        /// <inheritdoc />
        public override void OnMouseEnter(Float2 location)
        {
            // Check all children collisions with mouse and fire events for them
            for (int i = _children.Count - 1; i >= 0 && _children.Count > 0; i--)
            {
                var child = _children[i];
                if (child.Visible && child.Enabled)
                {
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
        public override void OnMouseMove(Float2 location)
        {
            // Check all children collisions with mouse and fire events for them
            for (int i = _children.Count - 1; i >= 0 && _children.Count > 0; i--)
            {
                var child = _children[i];
                if (child.Visible && child.Enabled)
                {
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
            for (int i = 0; i < _children.Count; i++)
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
        public override bool OnMouseWheel(Float2 location, float delta)
        {
            // Check all children collisions with mouse and fire events for them
            for (int i = _children.Count - 1; i >= 0 && _children.Count > 0; i--)
            {
                var child = _children[i];
                if (child.Visible && child.Enabled)
                {
                    if (IntersectsChildContent(child, location, out var childLocation))
                    {
                        if (child.OnMouseWheel(childLocation, delta))
                        {
                            return true;
                        }
                    }
                }
            }
            return false;
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            // Check all children collisions with mouse and fire events for them
            for (int i = _children.Count - 1; i >= 0 && _children.Count > 0; i--)
            {
                var child = _children[i];
                if (child.Visible && child.Enabled)
                {
                    if (IntersectsChildContent(child, location, out var childLocation))
                    {
                        if (child.OnMouseDown(childLocation, button))
                        {
                            return true;
                        }
                    }
                }
            }
            return false;
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            // Check all children collisions with mouse and fire events for them
            for (int i = _children.Count - 1; i >= 0 && _children.Count > 0; i--)
            {
                var child = _children[i];
                if (child.Visible && child.Enabled)
                {
                    if (IntersectsChildContent(child, location, out var childLocation))
                    {
                        if (child.OnMouseUp(childLocation, button))
                        {
                            return true;
                        }
                    }
                }
            }
            return false;
        }

        /// <inheritdoc />
        public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            // Check all children collisions with mouse and fire events for them
            for (int i = _children.Count - 1; i >= 0 && _children.Count > 0; i--)
            {
                var child = _children[i];
                if (child.Visible && child.Enabled)
                {
                    if (IntersectsChildContent(child, location, out var childLocation))
                    {
                        if (child.OnMouseDoubleClick(childLocation, button))
                        {
                            return true;
                        }
                    }
                }
            }
            return false;
        }

        /// <inheritdoc />
        public override bool IsTouchPointerOver(int pointerId)
        {
            if (base.IsTouchPointerOver(pointerId))
                return true;

            for (int i = 0; i < _children.Count; i++)
            {
                if (_children[i].IsTouchPointerOver(pointerId))
                    return true;
            }

            return false;
        }

        /// <inheritdoc />
        public override void OnTouchEnter(Float2 location, int pointerId)
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
        public override bool OnTouchDown(Float2 location, int pointerId)
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
        public override void OnTouchMove(Float2 location, int pointerId)
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
        public override bool OnTouchUp(Float2 location, int pointerId)
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
            for (int i = 0; i < _children.Count; i++)
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
            for (int i = 0; i < _children.Count; i++)
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
            for (int i = 0; i < _children.Count; i++)
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
            for (int i = 0; i < _children.Count; i++)
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
        public override DragDropEffect OnDragEnter(ref Float2 location, DragData data)
        {
            // Base
            var result = base.OnDragEnter(ref location, data);

            // Check all children collisions with mouse and fire events for them
            for (int i = _children.Count - 1; i >= 0 && _children.Count > 0; i--)
            {
                var child = _children[i];
                if (child.Visible && child.Enabled)
                {
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
        public override DragDropEffect OnDragMove(ref Float2 location, DragData data)
        {
            // Base
            var result = base.OnDragMove(ref location, data);

            // Check all children collisions with mouse and fire events for them
            for (int i = _children.Count - 1; i >= 0 && _children.Count > 0; i--)
            {
                var child = _children[i];
                if (child.Visible && child.Enabled)
                {
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
            for (int i = 0; i < _children.Count; i++)
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
        public override DragDropEffect OnDragDrop(ref Float2 location, DragData data)
        {
            // Base
            var result = base.OnDragDrop(ref location, data);

            // Check all children collisions with mouse and fire events for them
            for (int i = _children.Count - 1; i >= 0 && _children.Count > 0; i--)
            {
                var child = _children[i];
                if (child.Visible && child.Enabled)
                {
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
            bool wasLayoutLocked = _isLayoutLocked;
            _isLayoutLocked = true;

            base.OnSizeChanged();

            // Fire event
            for (int i = 0; i < _children.Count; i++)
            {
                _children[i].OnParentResized();
            }

            // Restore state
            _isLayoutLocked = wasLayoutLocked;

            // Arrange child controls
            PerformLayout();
        }

        #endregion
    }
}
