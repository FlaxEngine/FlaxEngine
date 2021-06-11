// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEngine;

namespace FlaxEditor.Gizmo
{
    /// <summary>
    /// Base class for transformation gizmos that can be used to select objects and transform them.
    /// </summary>
    /// <seealso cref="FlaxEditor.Gizmo.GizmoBase" />
    [HideInEditor]
    public abstract partial class TransformGizmoBase : GizmoBase
    {
        /// <summary>
        /// The start transforms list cached for selected objects before transformation apply. Can be used to create undo operations.
        /// </summary>
        protected readonly List<Transform> _startTransforms = new List<Transform>();

        /// <summary>
        /// Flag used to indicate that navigation data was modified.
        /// </summary>
        protected bool _navigationDirty;

        /// <summary>
        /// The initial world bounds of the selected objects before performing any transformations. Used to find the dirty volume of the world during editing.
        /// </summary>
        protected BoundingBox _startBounds = BoundingBox.Empty;

        private Vector3 _accMoveDelta;
        private Matrix _axisAlignedWorld = Matrix.Identity;

        private Matrix _gizmoWorld = Matrix.Identity;
        private Vector3 _intersectPosition;
        private bool _isActive;
        private bool _isDuplicating;

        private bool _isTransforming;
        private Vector3 _lastIntersectionPosition;

        private Vector3 _localForward = Vector3.Forward;
        private Vector3 _localRight = Vector3.Right;
        private Vector3 _localUp = Vector3.Up;

        private Matrix _objectOrientedWorld = Matrix.Identity;

        private Quaternion _rotationDelta = Quaternion.Identity;
        private Matrix _rotationMatrix;
        private float _rotationSnapDelta;
        private Vector3 _scaleDelta;
        private float _screenScale;

        private Matrix _screenScaleMatrix;
        private Vector3 _tDelta;
        private Vector3 _translationDelta;
        private Vector3 _translationScaleSnapDelta;

        /// <summary>
        /// Gets the gizmo position.
        /// </summary>
        public Vector3 Position { get; private set; }

        /// <summary>
        /// Gets the last transformation delta.
        /// </summary>
        public Transform LastDelta { get; private set; }

        /// <summary>
        /// Occurs when transforming selection started.
        /// </summary>
        public event Action TransformingStarted;

        /// <summary>
        /// Occurs when transforming selection ended.
        /// </summary>
        public event Action TransformingEnded;

        /// <summary>
        /// Initializes a new instance of the <see cref="TransformGizmoBase" /> class.
        /// </summary>
        /// <param name="owner">The gizmos owner.</param>
        public TransformGizmoBase(IGizmoOwner owner)
        : base(owner)
        {
            InitDrawing();
        }

        /// <summary>
        /// Starts the objects transforming (optionally with duplicate).
        /// </summary>
        protected void StartTransforming()
        {
            // Check if can start new action
            var count = SelectionCount;
            if (count == 0 || _isTransforming || !CanTransform)
                return;

            // Check if duplicate objects
            if (Owner.UseDuplicate && !_isDuplicating && CanDuplicate)
            {
                _isDuplicating = true;
                OnDuplicate();
                return;
            }

            // Cache 'before' state
            _startTransforms.Clear();
            if (_startTransforms.Capacity < count)
                _startTransforms.Capacity = Mathf.NextPowerOfTwo(count);
            for (var i = 0; i < count; i++)
            {
                _startTransforms.Add(GetSelectedObject(i));
            }
            GetSelectedObjectsBounds(out _startBounds, out _navigationDirty);

            // Start
            _isTransforming = true;
            OnStartTransforming();
        }

        /// <summary>
        /// Ends the objects transforming.
        /// </summary>
        protected void EndTransforming()
        {
            // Check if wasn't working at all
            if (!_isTransforming)
                return;

            // End action
            _isTransforming = false;
            _isDuplicating = false;
            OnEndTransforming();
            _startTransforms.Clear();
        }

        private void UpdateGizmoPosition()
        {
            switch (_activePivotType)
            {
            case PivotType.ObjectCenter:
                if (SelectionCount > 0)
                    Position = GetSelectedObject(0).Translation;
                break;
            case PivotType.SelectionCenter:
                Position = GetSelectionCenter();
                break;
            case PivotType.WorldOrigin:
                Position = Vector3.Zero;
                break;
            }
            Position += _translationDelta;
        }

        private void UpdateMatrices()
        {
            // Check there is no need to perform update
            if (SelectionCount == 0)
                return;

            // Set positions of the gizmo
            UpdateGizmoPosition();

            // Scale gizmo to fit on-screen
            Vector3 vLength = Owner.ViewPosition - Position;
            float gizmoSize = Editor.Instance.Options.Options.Visual.GizmoSize;
            _screenScale = vLength.Length / GizmoScaleFactor * gizmoSize;
            Matrix.Scaling(_screenScale, out _screenScaleMatrix);

            Quaternion orientation = GetSelectedObject(0).Orientation;
            Matrix.RotationQuaternion(ref orientation, out Matrix rotation);
            _localForward = rotation.Forward;
            _localUp = rotation.Up;

            // Vector Rotation (Local/World)
            _localForward.Normalize();
            Vector3.Cross(ref _localForward, ref _localUp, out _localRight);
            Vector3.Cross(ref _localRight, ref _localForward, out _localUp);
            _localRight.Normalize();
            _localUp.Normalize();

            // Create both world matrices
            _objectOrientedWorld = _screenScaleMatrix * Matrix.CreateWorld(Position, _localForward, _localUp);
            _axisAlignedWorld = _screenScaleMatrix * Matrix.CreateWorld(Position, Vector3.Backward, Vector3.Up);

            // Assign world
            if (_activeTransformSpace == TransformSpace.World && _activeMode != Mode.Scale)
            {
                _gizmoWorld = _axisAlignedWorld;

                // Align lines, boxes etc. with the grid-lines
                _rotationMatrix = Matrix.Identity;
            }
            else
            {
                _gizmoWorld = _objectOrientedWorld;

                // Align lines, boxes etc. with the selected object
                _rotationMatrix.Forward = _localForward;
                _rotationMatrix.Up = _localUp;
                _rotationMatrix.Right = _localRight;
            }
        }

        private void UpdateTranslateScale()
        {
            bool isScaling = _activeMode == Mode.Scale;
            Vector3 delta = Vector3.Zero;
            Ray ray = Owner.MouseRay;

            Matrix.Invert(ref _rotationMatrix, out var invRotationMatrix);
            ray.Position = Vector3.Transform(ray.Position, invRotationMatrix);
            Vector3.TransformNormal(ref ray.Direction, ref invRotationMatrix, out ray.Direction);

            var planeXY = new Plane(Vector3.Backward, Vector3.Transform(Position, invRotationMatrix).Z);
            var planeYZ = new Plane(Vector3.Left, Vector3.Transform(Position, invRotationMatrix).X);
            var planeZX = new Plane(Vector3.Down, Vector3.Transform(Position, invRotationMatrix).Y);
            var dir = Vector3.Normalize(ray.Position - Position);
            var planeDotXY = Mathf.Abs(Vector3.Dot(planeXY.Normal, dir));
            var planeDotYZ = Mathf.Abs(Vector3.Dot(planeYZ.Normal, dir));
            var planeDotZX = Mathf.Abs(Vector3.Dot(planeZX.Normal, dir));

            switch (_activeAxis)
            {
            case Axis.X:
            {
                var plane = planeDotXY > planeDotZX ? planeXY : planeZX;
                if (ray.Intersects(ref plane, out float intersection))
                {
                    _intersectPosition = ray.Position + ray.Direction * intersection;
                    if (_lastIntersectionPosition != Vector3.Zero)
                        _tDelta = _intersectPosition - _lastIntersectionPosition;
                    delta = new Vector3(_tDelta.X, 0, 0);
                }
                break;
            }
            case Axis.Y:
            {
                var plane = planeDotXY > planeDotYZ ? planeXY : planeYZ;
                if (ray.Intersects(ref plane, out float intersection))
                {
                    _intersectPosition = ray.Position + ray.Direction * intersection;
                    if (_lastIntersectionPosition != Vector3.Zero)
                        _tDelta = _intersectPosition - _lastIntersectionPosition;
                    delta = new Vector3(0, _tDelta.Y, 0);
                }
                break;
            }
            case Axis.Z:
            {
                var plane = planeDotZX > planeDotYZ ? planeZX : planeYZ;
                if (ray.Intersects(ref plane, out float intersection))
                {
                    _intersectPosition = ray.Position + ray.Direction * intersection;
                    if (_lastIntersectionPosition != Vector3.Zero)
                        _tDelta = _intersectPosition - _lastIntersectionPosition;
                    delta = new Vector3(0, 0, _tDelta.Z);
                }
                break;
            }
            case Axis.YZ:
            {
                if (ray.Intersects(ref planeYZ, out float intersection))
                {
                    _intersectPosition = ray.Position + ray.Direction * intersection;
                    if (_lastIntersectionPosition != Vector3.Zero)
                        _tDelta = _intersectPosition - _lastIntersectionPosition;
                    delta = new Vector3(0, _tDelta.Y, _tDelta.Z);
                }
                break;
            }
            case Axis.XY:
            {
                if (ray.Intersects(ref planeXY, out float intersection))
                {
                    _intersectPosition = ray.Position + ray.Direction * intersection;
                    if (_lastIntersectionPosition != Vector3.Zero)
                        _tDelta = _intersectPosition - _lastIntersectionPosition;
                    delta = new Vector3(_tDelta.X, _tDelta.Y, 0);
                }
                break;
            }
            case Axis.ZX:
            {
                if (ray.Intersects(ref planeZX, out float intersection))
                {
                    _intersectPosition = ray.Position + ray.Direction * intersection;
                    if (_lastIntersectionPosition != Vector3.Zero)
                        _tDelta = _intersectPosition - _lastIntersectionPosition;
                    delta = new Vector3(_tDelta.X, 0, _tDelta.Z);
                }
                break;
            }
            case Axis.Center:
            {
                var gizmoToView = Position - Owner.ViewPosition;
                var plane = new Plane(-Vector3.Normalize(gizmoToView), gizmoToView.Length);
                if (ray.Intersects(ref plane, out float intersection))
                {
                    _intersectPosition = ray.Position + ray.Direction * intersection;
                    if (_lastIntersectionPosition != Vector3.Zero)
                        _tDelta = _intersectPosition - _lastIntersectionPosition;
                }
                delta = _tDelta;
                break;
            }
            }

            // Modifiers
            if (isScaling)
                delta *= 0.01f;
            if (Owner.IsAltKeyDown)
                delta *= 0.5f;
            if ((isScaling ? ScaleSnapEnabled : TranslationSnapEnable) || Owner.UseSnapping)
            {
                float snapValue = isScaling ? ScaleSnapValue : TranslationSnapValue;
                _translationScaleSnapDelta += delta;
                delta = new Vector3(
                                    (int)(_translationScaleSnapDelta.X / snapValue) * snapValue,
                                    (int)(_translationScaleSnapDelta.Y / snapValue) * snapValue,
                                    (int)(_translationScaleSnapDelta.Z / snapValue) * snapValue);
                _translationScaleSnapDelta -= delta;
            }

            if (_activeMode == Mode.Translate)
            {
                // Transform (local or world)
                delta = Vector3.Transform(delta, _rotationMatrix);
                _translationDelta = delta;
            }
            else if (_activeMode == Mode.Scale)
            {
                // Scale
                _scaleDelta = delta;
            }
        }

        private void UpdateRotate(float dt)
        {
            float mouseDelta = _activeAxis == Axis.Y ? -Owner.MouseDelta.X : Owner.MouseDelta.X;

            float delta = mouseDelta * dt;

            if (RotationSnapEnabled || Owner.UseSnapping)
            {
                float snapValue = RotationSnapValue * Mathf.DegreesToRadians;
                _rotationSnapDelta += delta;

                float snapped = Mathf.Round(_rotationSnapDelta / snapValue) * snapValue;
                _rotationSnapDelta -= snapped;

                delta = snapped;
            }

            switch (_activeAxis)
            {
            case Axis.X:
            case Axis.Y:
            case Axis.Z:
            {
                Vector3 dir;
                if (_activeAxis == Axis.X)
                    dir = _rotationMatrix.Right;
                else if (_activeAxis == Axis.Y)
                    dir = _rotationMatrix.Up;
                else
                    dir = _rotationMatrix.Forward;

                Vector3 viewDir = Owner.ViewPosition - Position;
                Vector3.Dot(ref viewDir, ref dir, out float dot);
                if (dot < 0.0f)
                    delta *= -1;

                Quaternion.RotationAxis(ref dir, delta, out _rotationDelta);
                break;
            }

            default:
                _rotationDelta = Quaternion.Identity;
                break;
            }
        }

        /// <inheritdoc />
        public override bool IsControllingMouse => _isTransforming;

        /// <inheritdoc />
        public override void Update(float dt)
        {
            LastDelta = Transform.Identity;

            if (!IsActive)
                return;

            bool isLeftBtnDown = Owner.IsLeftMouseButtonDown;

            // Snap to ground
            if (_activeAxis == Axis.None && SelectionCount != 0 && Owner.SnapToGround)
            {
                if (Physics.RayCast(Position, Vector3.Down, out var hit, float.MaxValue, uint.MaxValue, false))
                {
                    StartTransforming();
                    var translationDelta = hit.Point - Position;
                    var rotationDelta = Quaternion.Identity;
                    var scaleDelta = Vector3.Zero;
                    OnApplyTransformation(ref translationDelta, ref rotationDelta, ref scaleDelta);
                    EndTransforming();
                }
            }
            // Only when is active
            else if (_isActive)
            {
                // Backup position
                _lastIntersectionPosition = _intersectPosition;
                _intersectPosition = Vector3.Zero;

                // Check if user is holding left mouse button and any axis is selected
                if (isLeftBtnDown && _activeAxis != Axis.None)
                {
                    switch (_activeMode)
                    {
                    case Mode.Scale:
                    case Mode.Translate:
                        UpdateTranslateScale();
                        break;
                    case Mode.Rotate:
                        UpdateRotate(dt);
                        break;
                    }
                }
                else
                {
                    // If nothing selected, try to select any axis
                    if (!isLeftBtnDown && !Owner.IsRightMouseButtonDown)
                        SelectAxis();
                }

                // Set positions of the gizmo
                UpdateGizmoPosition();

                // Trigger Translation, Rotation & Scale events
                if (isLeftBtnDown)
                {
                    var anyValid = false;

                    // Translation
                    Vector3 translationDelta = Vector3.Zero;
                    if (_translationDelta.LengthSquared > 0.000001f)
                    {
                        anyValid = true;
                        translationDelta = _translationDelta;
                        _translationDelta = Vector3.Zero;

                        // Prevent from moving objects too far away, like to a different galaxy or sth
                        Vector3 prevMoveDelta = _accMoveDelta;
                        _accMoveDelta += _translationDelta;
                        if (_accMoveDelta.Length > Owner.ViewFarPlane * 0.7f)
                            _accMoveDelta = prevMoveDelta;
                    }

                    // Rotation
                    Quaternion rotationDelta = Quaternion.Identity;
                    if (!_rotationDelta.IsIdentity)
                    {
                        anyValid = true;
                        rotationDelta = _rotationDelta;
                        _rotationDelta = Quaternion.Identity;
                    }

                    // Scale
                    Vector3 scaleDelta = Vector3.Zero;
                    if (_scaleDelta.LengthSquared > 0.000001f)
                    {
                        anyValid = true;
                        scaleDelta = _scaleDelta;
                        _scaleDelta = Vector3.Zero;

                        if (ActiveAxis == Axis.Center)
                            scaleDelta = new Vector3(scaleDelta.AvgValue);
                    }

                    // Apply transformation (but to the parents, not whole selection pool)
                    if (anyValid)
                    {
                        StartTransforming();

                        LastDelta = new Transform(translationDelta, rotationDelta, scaleDelta);
                        OnApplyTransformation(ref translationDelta, ref rotationDelta, ref scaleDelta);
                    }
                }
                else
                {
                    // Clear cache
                    _accMoveDelta = Vector3.Zero;
                    EndTransforming();
                }
            }

            // Check if has no objects selected
            if (SelectionCount == 0)
            {
                // Deactivate
                _isActive = false;
                _activeAxis = Axis.None;
                return;
            }

            // Helps solve visual lag (1-frame-lag) after selecting a new entity
            if (!_isActive)
                UpdateGizmoPosition();

            // Activate
            _isActive = true;

            // Update
            UpdateMatrices();
        }

        /// <summary>
        /// Gets a value indicating whether this tool can transform objects.
        /// </summary>
        protected virtual bool CanTransform => true;

        /// <summary>
        /// Gets a value indicating whether this tool can duplicate objects.
        /// </summary>
        protected virtual bool CanDuplicate => true;

        /// <summary>
        /// Gets the selected objects count.
        /// </summary>
        protected abstract int SelectionCount { get; }

        /// <summary>
        /// Gets the selected object transformation.
        /// </summary>
        /// <param name="index">The selected object index.</param>
        protected abstract Transform GetSelectedObject(int index);

        /// <summary>
        /// Gets the selected objects bounding box (contains the whole selection).
        /// </summary>
        /// <param name="bounds">The bounds of the selected objects (merged bounds).</param>
        /// <param name="navigationDirty">True if editing the selected objects transformations marks the navigation system area dirty (for auto-rebuild), otherwise skip update.</param>
        protected abstract void GetSelectedObjectsBounds(out BoundingBox bounds, out bool navigationDirty);

        /// <summary>
        /// Called when user starts transforming selected objects.
        /// </summary>
        protected virtual void OnStartTransforming()
        {
            TransformingStarted?.Invoke();
        }

        /// <summary>
        /// Called when gizmo tools wants to apply transformation delta to the selected objects pool.
        /// </summary>
        /// <param name="translationDelta">The translation delta.</param>
        /// <param name="rotationDelta">The rotation delta.</param>
        /// <param name="scaleDelta">The scale delta.</param>
        protected virtual void OnApplyTransformation(ref Vector3 translationDelta, ref Quaternion rotationDelta, ref Vector3 scaleDelta)
        {
        }

        /// <summary>
        /// Called when user ends transforming selected objects.
        /// </summary>
        protected virtual void OnEndTransforming()
        {
            TransformingEnded?.Invoke();
        }

        /// <summary>
        /// Called when user duplicates selected objects.
        /// </summary>
        protected virtual void OnDuplicate()
        {
        }
    }
}
