// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using FlaxEngine.GUI;

namespace FlaxEngine
{
    partial class UIControl
    {
        private Control _control;
        private static bool _blockEvents; // Used to ignore internal events from C++ UIControl impl when performing state sync with C# UI

        /// <summary>
        /// Gets or sets the GUI control used by this actor.
        /// </summary>
        /// <remarks>
        /// When changing the control, the previous one is disposed. Use <see cref="UnlinkControl"/> to manage it on your own.
        /// </remarks>
        [EditorDisplay("Control", EditorDisplayAttribute.InlineStyle), CustomEditorAlias("FlaxEditor.CustomEditors.Dedicated.UIControlControlEditor"), EditorOrder(50)]
        public Control Control
        {
            get => _control;
            set
            {
                if (_control == value)
                    return;

                // Cleanup previous
                var prevControl = _control;
                if (prevControl != null)
                {
                    prevControl.LocationChanged -= OnControlLocationChanged;
                    prevControl.Dispose();
                }

                // Set value
                _control = value;

                // Link the new one (events and parent)
                if (_control != null)
                {
                    // Setup control
                    _blockEvents = true;
                    var containerControl = _control as ContainerControl;
                    if (containerControl != null)
                        containerControl.UnlockChildrenRecursive();
                    _control.Visible = IsActive;
                    _control.Parent = GetParent();
                    _control.IndexInParent = OrderInParent;
                    _control.Location = new Float2(LocalPosition);
                    _control.LocationChanged += OnControlLocationChanged;

                    // Link children UI controls
                    if (containerControl != null)
                    {
                        var children = ChildrenCount;
                        var parent = Parent;
                        for (int i = 0; i < children; i++)
                        {
                            var child = GetChild(i) as UIControl;
                            if (child != null && child.HasControl && child != parent)
                            {
                                child.Control.Parent = containerControl;
                            }
                        }
                    }

                    // Refresh
                    _blockEvents = false;
                    if (prevControl == null && _control.Parent != null)
                        _control.Parent.PerformLayout();
                    else
                        _control.PerformLayout();
                }
            }
        }

        /// <summary>
        /// Gets a value indicating whether this actor has control.
        /// </summary>
        public bool HasControl => _control != null;

        /// <summary>
        /// Gets the world-space oriented bounding box that contains a 3D control.
        /// </summary>
        public OrientedBoundingBox Bounds
        {
            get
            {
                // Pick a parent canvas
                var canvasRoot = Control?.Root as CanvasRootControl;
                if (canvasRoot == null || canvasRoot.Canvas.Is2D)
                    return new OrientedBoundingBox();

                // Find control bounds limit points in canvas-space
                var p1 = Float2.Zero;
                var p2 = new Float2(0, Control.Height);
                var p3 = new Float2(Control.Width, 0);
                var p4 = Control.Size;
                Control c = Control;
                while (c != canvasRoot)
                {
                    p1 = c.PointToParent(ref p1);
                    p2 = c.PointToParent(ref p2);
                    p3 = c.PointToParent(ref p3);
                    p4 = c.PointToParent(ref p4);
                    c = c.Parent;
                }
                var min = Float2.Min(Float2.Min(p1, p2), Float2.Min(p3, p4));
                var max = Float2.Max(Float2.Max(p1, p2), Float2.Max(p3, p4));
                var size = max - min;

                // Calculate bounds
                var bounds = new OrientedBoundingBox
                {
                    Extents = new Vector3(size * 0.5f, Mathf.Epsilon)
                };
                canvasRoot.Canvas.GetWorldMatrix(out Matrix world);
                Matrix.Translation(min.X + size.X * 0.5f, min.Y + size.Y * 0.5f, 0, out Matrix offset);
                Matrix.Multiply(ref offset, ref world, out var boxWorld);
                boxWorld.Decompose(out bounds.Transformation);
                return bounds;
            }
        }

        /// <summary>
        /// Gets the control object cased to the given type.
        /// </summary>
        /// <typeparam name="T">The type of the control.</typeparam>
        /// <returns>The control object.</returns>
        public T Get<T>() where T : Control
        {
            return (T)_control;
        }

        /// <summary>
        /// Checks if the control object is of the given type.
        /// </summary>
        /// <typeparam name="T">The type of the control.</typeparam>
        /// <returns>True if control object is of the given type.</returns>
        public bool Is<T>() where T : Control
        {
            return _control is T;
        }

        /// <summary>
        /// Creates a new UIControl with the control of the given type and links it to this control as a child.
        /// </summary>
        /// <remarks>
        /// The current actor has to have a valid container control.
        /// </remarks>
        /// <typeparam name="T">Type of the child control to add.</typeparam>
        /// <returns>The created UIControl that contains a new control of the given type.</returns>
        public UIControl AddChildControl<T>() where T : Control
        {
            if (!(_control is ContainerControl))
                throw new InvalidOperationException("To add child to the control it has to be ContainerControl.");

            var child = new UIControl
            {
                Parent = this,
                Control = (Control)Activator.CreateInstance(typeof(T))
            };
            return child;
        }

        /// <summary>
        /// Unlinks the control from the actor without disposing it or modifying.
        /// </summary>
        [NoAnimate]
        public void UnlinkControl()
        {
            if (_control != null)
            {
                _control.LocationChanged -= OnControlLocationChanged;
                _control = null;
            }
        }

        #region Navigation

        /// <summary>
        /// The explicitly specified target navigation control for <see cref="NavDirection.Up"/> direction.
        /// </summary>
        [Tooltip("The explicitly specified target navigation control for Up direction. Leave empty to use automatic navigation.")]
        [EditorDisplay("Navigation"), EditorOrder(1010), VisibleIf(nameof(HasControl))]
        public UIControl NavTargetUp
        {
            get
            {
                Internal_GetNavTargets(__unmanagedPtr, out UIControl up, out _, out _, out _);
                return up;
            }
            set
            {
                Internal_GetNavTargets(__unmanagedPtr, out UIControl up, out UIControl down, out UIControl left, out UIControl right);
                if (up == value)
                    return;
                up = value;
                Internal_SetNavTargets(__unmanagedPtr, GetUnmanagedPtr(up), GetUnmanagedPtr(down), GetUnmanagedPtr(left), GetUnmanagedPtr(right));
                if (_control != null)
                    _control.NavTargetUp = value != null ? value.Control : null;
            }
        }

        /// <summary>
        /// The explicitly specified target navigation control for <see cref="NavDirection.Down"/> direction.
        /// </summary>
        [Tooltip("The explicitly specified target navigation control for Down direction. Leave empty to use automatic navigation.")]
        [EditorDisplay("Navigation"), EditorOrder(1020), VisibleIf(nameof(HasControl))]
        public UIControl NavTargetDown
        {
            get
            {
                Internal_GetNavTargets(__unmanagedPtr, out _, out UIControl down, out _, out _);
                return down;
            }
            set
            {
                Internal_GetNavTargets(__unmanagedPtr, out UIControl up, out UIControl down, out UIControl left, out UIControl right);
                if (down == value)
                    return;
                down = value;
                Internal_SetNavTargets(__unmanagedPtr, GetUnmanagedPtr(up), GetUnmanagedPtr(down), GetUnmanagedPtr(left), GetUnmanagedPtr(right));
                if (_control != null)
                    _control.NavTargetDown = value != null ? value.Control : null;
            }
        }

        /// <summary>
        /// The explicitly specified target navigation control for <see cref="NavDirection.Left"/> direction.
        /// </summary>
        [Tooltip("The explicitly specified target navigation control for Left direction. Leave empty to use automatic navigation.")]
        [EditorDisplay("Navigation"), EditorOrder(1030), VisibleIf(nameof(HasControl))]
        public UIControl NavTargetLeft
        {
            get
            {
                Internal_GetNavTargets(__unmanagedPtr, out _, out _, out UIControl left, out _);
                return left;
            }
            set
            {
                Internal_GetNavTargets(__unmanagedPtr, out UIControl up, out UIControl down, out UIControl left, out UIControl right);
                if (left == value)
                    return;
                left = value;
                Internal_SetNavTargets(__unmanagedPtr, GetUnmanagedPtr(up), GetUnmanagedPtr(down), GetUnmanagedPtr(left), GetUnmanagedPtr(right));
                if (_control != null)
                    _control.NavTargetLeft = value != null ? value.Control : null;
            }
        }

        /// <summary>
        /// The explicitly specified target navigation control for <see cref="NavDirection.Right"/> direction.
        /// </summary>
        [Tooltip("The explicitly specified target navigation control for Right direction. Leave empty to use automatic navigation.")]
        [EditorDisplay("Navigation"), EditorOrder(1040), VisibleIf(nameof(HasControl))]
        public UIControl NavTargetRight
        {
            get
            {
                Internal_GetNavTargets(__unmanagedPtr, out _, out _, out _, out UIControl right);
                return right;
            }
            set
            {
                Internal_GetNavTargets(__unmanagedPtr, out UIControl up, out UIControl down, out UIControl left, out UIControl right);
                if (right == value)
                    return;
                right = value;
                Internal_SetNavTargets(__unmanagedPtr, GetUnmanagedPtr(up), GetUnmanagedPtr(down), GetUnmanagedPtr(left), GetUnmanagedPtr(right));
                if (_control != null)
                    _control.NavTargetRight = value != null ? value.Control : null;
            }
        }

        #endregion

        private void OnControlLocationChanged(Control control)
        {
            _blockEvents = true;
            LocalPosition = new Vector3(control.Location, LocalPosition.Z);
            _blockEvents = false;
        }

        /// <summary>
        /// The fallback callback used to handle <see cref="UIControl"/> parent container control to link when it fails to find the default parent. Can be used to link the controls into a custom control.
        /// </summary>
        public static Func<UIControl, ContainerControl> FallbackParentGetDelegate;

        private ContainerControl GetParent()
        {
            var parent = Parent;
            if (parent is UIControl uiControl && uiControl.Control is ContainerControl uiContainerControl)
                return uiContainerControl;
            if (parent is UICanvas uiCanvas)
                return uiCanvas.GUI;
            return FallbackParentGetDelegate?.Invoke(this);
        }

        internal string Serialize(out string controlType, UIControl other)
        {
            if (_control == null)
            {
                // No control assigned
                controlType = null;
                return null;
            }

            var type = _control.GetType();
            var prefabDiff = other != null && other._control != null && other._control.GetType() == type;

            // Serialize control type when not using prefab diff serialization
            controlType = prefabDiff ? string.Empty : type.FullName;

            string json;
            if (prefabDiff)
                json = Json.JsonSerializer.SerializeDiff(_control, other._control, true);
            else
                json = Json.JsonSerializer.Serialize(_control, type, true);
            return json;
        }

        internal void Deserialize(string json, Type controlType)
        {
            if ((_control == null || _control.GetType() != controlType) && controlType != null)
            {
                Control = (Control)Activator.CreateInstance(controlType);
            }

            if (_control != null)
            {
                Json.JsonSerializer.Deserialize(_control, json);

                // Synchronize actor with control location
                OnControlLocationChanged(_control);
            }
        }

        internal void ParentChanged()
        {
            if (_control != null && !_blockEvents)
            {
                _control.Visible = IsActive;
                _control.Parent = GetParent();
                _control.IndexInParent = OrderInParent;
            }
        }

        internal void TransformChanged()
        {
            if (_control != null && !_blockEvents)
            {
                _control.Location = new Float2(LocalPosition);
            }
        }

        internal void ActiveChanged()
        {
            if (_control != null && !_blockEvents)
            {
                _control.Visible = IsActive;
            }
        }

        internal void OrderInParentChanged()
        {
            if (_control != null && !_blockEvents)
            {
                _control.IndexInParent = OrderInParent;
            }
        }

        internal void BeginPlay()
        {
            if (_control != null)
            {
                _control.Visible = IsActive && _control.Visible;
                _control.Parent = GetParent();
                _control.IndexInParent = OrderInParent;
                Internal_GetNavTargets(__unmanagedPtr, out UIControl up, out UIControl down, out UIControl left, out UIControl right);
                _control.NavTargetUp = up?.Control;
                _control.NavTargetDown = down?.Control;
                _control.NavTargetLeft = left?.Control;
                _control.NavTargetRight = right?.Control;
            }
        }

        internal void EndPlay()
        {
            if (_control != null)
            {
                _control.Dispose();
                _control = null;
            }
        }
    }
}
