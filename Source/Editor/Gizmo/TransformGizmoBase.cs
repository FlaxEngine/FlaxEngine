using System;
using System.Collections.Generic;
using System.Diagnostics;
using FlaxEditor.SceneGraph;
using FlaxEngine;

namespace FlaxEditor.Gizmo
{
    /// <summary>
    /// Base class for transformation gizmos that can be used to select objects and transform them.
    /// </summary>
    /// <seealso cref="T:FlaxEditor.Gizmo.GizmoBase" />
    // Token: 0x02000588 RID: 1416
    [HideInEditor]
    public abstract partial class TransformGizmoBase : GizmoBase
    {
        private bool TranslationSnaping
        {
            get
            {
                return ((TranslationSnapEnable || base.Owner.UseSnapping) && base.Owner.IsControlDown) || base.Owner.IsControlDown;
            }
        }

        private bool RotationSnaping
        {
            get
            {
                return ((RotationSnapEnabled || base.Owner.UseSnapping) && base.Owner.IsControlDown) || base.Owner.IsControlDown;
            }
        }

        private bool ScaleSnaping
        {
            get
            {
                return ((ScaleSnapEnabled || base.Owner.UseSnapping) && base.Owner.IsControlDown) || base.Owner.IsControlDown;
            }
        }

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

        /// <inheritdoc />
        public override bool IsControllingMouse
        {
            get
            {
                return _isTransforming;
            }
        }

        /// <summary>
        /// Gets a value indicating whether this tool can transform objects.
        /// </summary>
        protected virtual bool CanTransform
        {
            get
            {
                return true;
            }
        }

        /// <summary>
        /// Gets a value indicating whether this tool can duplicate objects.
        /// </summary>
        protected virtual bool CanDuplicate
        {
            get
            {
                return true;
            }
        }

        /// <summary>
        /// Gets the selected objects count.
        /// </summary>
        protected abstract int SelectionCount { get; }

        /// <summary>
        /// Initializes a new instance of the <see cref="T:FlaxEditor.Gizmo.TransformGizmoBase" /> class.
        /// </summary>
        /// <param name="owner">The gizmos owner.</param>
        public TransformGizmoBase(IGizmoOwner owner) : base(owner)
        {
            resources = Resources.Load();
            ModeChanged = (Action)Delegate.Combine(ModeChanged, new Action(ResetTranslationScale));
            Owner.Viewport.OnCameraMoved += () => ComputeNewTransformScale(_activeMode, _activeTransformSpace); ;
        }

        /// <summary>
        /// Starts the objects transforming (optionally with duplicate).
        /// </summary>
        public void StartTransforming()
        {
            int count = SelectionCount;
            if (count == 0 || _isTransforming || !CanTransform)
                return;

            if (Owner.UseDuplicate && !_isDuplicating && CanDuplicate)
            {
                _isDuplicating = true;
                OnDuplicate();
            }
            else
            {
                _startTransforms.Clear();
                if (_startTransforms.Capacity < count)
                {
                    _startTransforms.Capacity = Mathf.NextPowerOfTwo(count);
                }
                for (int i = 0; i < count; i++)
                {
                    _startTransforms.Add(GetSelectedObject(i));
                }
                GetSelectedObjectsBounds(out _startBounds, out _navigationDirty);
                StartWorldTransform = WorldTransform;
                _isTransforming = true;
                OnStartTransforming();
                ComputeCenterRotacion(Owner);
                switch (_activeAxis)
                {
                    case Axis.X:
                        resources._materialCenter.SetParameterValue("MainColor", Resources.XAxisColor, true);
                        break;
                    case Axis.Y:
                        resources._materialCenter.SetParameterValue("MainColor", Resources.YAxisColor, true);
                        break;
                    case Axis.Z:
                        resources._materialCenter.SetParameterValue("MainColor", Resources.ZAxisColor, true);
                        break;
                }
            }
        }

        /// <summary>
        /// Ends the objects transforming.
        /// </summary>
        public void EndTransforming()
        {
            if (_isTransforming)
            {
                resources._materialCenter.SetParameterValue("RotacionStartEnd", new Float2(0f, 0f), true);
                resources._materialCenter.SetParameterValue("MainColor", Resources.CenterColor, true);
                EndRot = 0f;
                _isTransforming = false;
                _isDuplicating = false;
                OnEndTransforming();
                _startTransforms.Clear();
            }
        }

        private void UpdateGizmoPosition()
        {
            switch (_activePivotType)
            {
                case PivotType.ObjectCenter:
                    {
                        if (SelectionCount > 0)
                        {
                            Position = GetSelectedObject(0).Translation;
                        }
                        break;
                    }
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
            if (SelectionCount != 0)
            {
                UpdateGizmoPosition();
                Vector3 position = Position;
                // this is incorrect for orto because the camera has flat projection but works for now
                Vector3 vLength = base.Owner.ViewPosition - position; 
                float gizmoSize = Editor.Instance.Options.Options.Visual.GizmoSize;
                bool useOrthographicProjection = base.Owner.Viewport.Camera.UseOrthographicProjection;
                if (useOrthographicProjection)
                {
                    _screenScale = base.Owner.Viewport.Camera.OrthographicScale * gizmoSize;
                }
                else
                {
                    _screenScale = vLength.Length * gizmoSize * 0.005f;
                }
                Quaternion orientation = GetSelectedObject(0).Orientation;
                WorldTransform.Translation = position;
                WorldTransform.Orientation = orientation;
                WorldTransform.Scale = new Float3(_screenScale);
                if (_activeTransformSpace == TransformSpace.World)
                {
                    WorldTransform.Orientation = Quaternion.Identity;
                }
            }
        }

        private void UpdateTranslateScale()
        {
            bool isScaling = _activeMode == Mode.Scale;
            Vector3 delta = Vector3.Zero;
            Ray ray = base.Owner.MouseRay;
            Matrix rotationMatrix;
            Matrix.RotationQuaternion(ref WorldTransform.Orientation, out rotationMatrix);
            Matrix invRotationMatrix;
            Matrix.Invert(ref rotationMatrix, out invRotationMatrix);
            ray.Position = Vector3.Transform(ray.Position, invRotationMatrix);
            Vector3.TransformNormal(ref ray.Direction, ref invRotationMatrix, out ray.Direction);
            Plane planeXY = new Plane(Vector3.Backward, Vector3.Transform(Position, invRotationMatrix).Z);
            Plane planeYZ = new Plane(Vector3.Left, Vector3.Transform(Position, invRotationMatrix).X);
            Plane planeZX = new Plane(Vector3.Down, Vector3.Transform(Position, invRotationMatrix).Y);
            Vector3 dir = Vector3.Normalize(ray.Position - Position);
            float planeDotXY = Mathf.Abs(Vector3.Dot(planeXY.Normal, dir));
            float planeDotYZ = Mathf.Abs(Vector3.Dot(planeYZ.Normal, dir));
            float planeDotZX = Mathf.Abs(Vector3.Dot(planeZX.Normal, dir));
            Axis activeAxis = _activeAxis;
            Axis axis = activeAxis;
            switch (axis)
            {
                case Axis.X:
                    {
                        Plane plane = (planeDotXY > planeDotZX) ? planeXY : planeZX;
                        float intersection;
                        bool flag = ray.Intersects(ref plane, out intersection);
                        if (flag)
                        {
                            _intersectPosition = ray.Position + ray.Direction * intersection;
                            bool flag2 = _lastIntersectionPosition != Vector3.Zero;
                            if (flag2)
                            {
                                _tDelta = _intersectPosition - _lastIntersectionPosition;
                            }
                            delta = new Vector3(_tDelta.X, 0f, 0f);
                        }
                        break;
                    }
                case Axis.Y:
                    {
                        Plane plane2 = (planeDotXY > planeDotYZ) ? planeXY : planeYZ;
                        float intersection;
                        bool flag3 = ray.Intersects(ref plane2, out intersection);
                        if (flag3)
                        {
                            _intersectPosition = ray.Position + ray.Direction * intersection;
                            bool flag4 = _lastIntersectionPosition != Vector3.Zero;
                            if (flag4)
                            {
                                _tDelta = _intersectPosition - _lastIntersectionPosition;
                            }
                            delta = new Vector3(0f, _tDelta.Y, 0f);
                        }
                        break;
                    }
                case Axis.XY:
                    {
                        float intersection;
                        bool flag5 = ray.Intersects(ref planeXY, out intersection);
                        if (flag5)
                        {
                            _intersectPosition = ray.Position + ray.Direction * intersection;
                            bool flag6 = _lastIntersectionPosition != Vector3.Zero;
                            if (flag6)
                            {
                                _tDelta = _intersectPosition - _lastIntersectionPosition;
                            }
                            delta = new Vector3(_tDelta.X, _tDelta.Y, 0f);
                        }
                        break;
                    }
                case Axis.Z:
                    {
                        Plane plane3 = (planeDotZX > planeDotYZ) ? planeZX : planeYZ;
                        float intersection;
                        bool flag7 = ray.Intersects(ref plane3, out intersection);
                        if (flag7)
                        {
                            _intersectPosition = ray.Position + ray.Direction * intersection;
                            bool flag8 = _lastIntersectionPosition != Vector3.Zero;
                            if (flag8)
                            {
                                _tDelta = _intersectPosition - _lastIntersectionPosition;
                            }
                            delta = new Vector3(0f, 0f, _tDelta.Z);
                        }
                        break;
                    }
                case Axis.ZX:
                    {
                        float intersection;
                        bool flag9 = ray.Intersects(ref planeZX, out intersection);
                        if (flag9)
                        {
                            _intersectPosition = ray.Position + ray.Direction * intersection;
                            bool flag10 = _lastIntersectionPosition != Vector3.Zero;
                            if (flag10)
                            {
                                _tDelta = _intersectPosition - _lastIntersectionPosition;
                            }
                            delta = new Vector3(_tDelta.X, 0f, _tDelta.Z);
                        }
                        break;
                    }
                case Axis.YZ:
                    {
                        float intersection;
                        bool flag11 = ray.Intersects(ref planeYZ, out intersection);
                        if (flag11)
                        {
                            _intersectPosition = ray.Position + ray.Direction * intersection;
                            bool flag12 = _lastIntersectionPosition != Vector3.Zero;
                            if (flag12)
                            {
                                _tDelta = _intersectPosition - _lastIntersectionPosition;
                            }
                            delta = new Vector3(0f, _tDelta.Y, _tDelta.Z);
                        }
                        break;
                    }
                default:
                    if (axis == Axis.View)
                    {
                        Vector3 gizmoToView = Position - base.Owner.ViewPosition;
                        Plane plane4 = new Plane(-Vector3.Normalize(gizmoToView), gizmoToView.Length);
                        float intersection;
                        bool flag13 = ray.Intersects(ref plane4, out intersection);
                        if (flag13)
                        {
                            _intersectPosition = ray.Position + ray.Direction * intersection;
                            bool flag14 = _lastIntersectionPosition != Vector3.Zero;
                            if (flag14)
                            {
                                _tDelta = _intersectPosition - _lastIntersectionPosition;
                            }
                        }
                        delta = _tDelta;
                    }
                    break;
            }
            bool flag15 = isScaling;
            if (flag15)
            {
                delta *= 0.01f;
            }
            bool isAltKeyDown = base.Owner.IsAltKeyDown;
            if (isAltKeyDown)
            {
                delta *= 0.5f;
            }
            bool flag16 = (isScaling ? ScaleSnapEnabled : TranslationSnapEnable) || base.Owner.UseSnapping;
            if (flag16)
            {
                Vector3 snapValue = new Vector3(isScaling ? ScaleSnapValue : TranslationSnapValue);
                _translationScaleSnapDelta += delta;
                bool flag17 = !isScaling && snapValue.X < 0f;
                if (flag17)
                {
                    BoundingBox b;
                    bool flag18;
                    GetSelectedObjectsBounds(out b, out flag18);
                    bool flag19 = b.Minimum.X < 0f;
                    if (flag19)
                    {
                        snapValue.X = Math.Abs(b.Minimum.X) + b.Maximum.X;
                    }
                    else
                    {
                        snapValue.X = b.Minimum.X - b.Maximum.X;
                    }
                    bool flag20 = b.Minimum.Y < 0f;
                    if (flag20)
                    {
                        snapValue.Y = Math.Abs(b.Minimum.Y) + b.Maximum.Y;
                    }
                    else
                    {
                        snapValue.Y = b.Minimum.Y - b.Maximum.Y;
                    }
                    bool flag21 = b.Minimum.Z < 0f;
                    if (flag21)
                    {
                        snapValue.Z = Math.Abs(b.Minimum.Z) + b.Maximum.Z;
                    }
                    else
                    {
                        snapValue.Z = b.Minimum.Z - b.Maximum.Z;
                    }
                }
                delta = new Vector3((float)((int)(_translationScaleSnapDelta.X / snapValue.X)) * snapValue.X, (float)((int)(_translationScaleSnapDelta.Y / snapValue.Y)) * snapValue.Y, (float)((int)(_translationScaleSnapDelta.Z / snapValue.Z)) * snapValue.Z);
                _translationScaleSnapDelta -= delta;
            }
            bool flag22 = _activeMode == Mode.Translate;
            if (flag22)
            {
                delta = Vector3.Transform(delta, rotationMatrix);
                _translationDelta = delta;
            }
            else
            {
                bool flag23 = _activeMode == Mode.Scale;
                if (flag23)
                {
                    _scaleDelta = delta;
                }
            }
        }

        private void ResetTranslationScale()
        {
            _translationScaleSnapDelta.Normalize();
        }

        private void UpdateRotate(float dt)
        {
            float mouseDelta = (_activeAxis == Axis.Y) ? (-base.Owner.MouseDelta.X) : base.Owner.MouseDelta.X;
            float delta = mouseDelta * dt;
            bool rotationSnaping = RotationSnaping;
            if (rotationSnaping)
            {
                float snapValue = RotationSnapValue * Mathf.DegreesToRadians;
                _rotationSnapDelta += delta;
                float snapped = Mathf.Round(_rotationSnapDelta / snapValue) * snapValue;
                _rotationSnapDelta -= snapped;
                delta = snapped;
            }
            Float3 dir = Center.Forward;
            Quaternion.RotationAxis(ref dir, delta, out _rotationDelta);
            EndRot -= delta * Mathf.RadiansToDegrees;
        }

        /// <inheritdoc />
        public override void Update(float dt)
        {
            LastDelta = Transform.Identity;
            if (IsActive)
            {
                bool isLeftBtnDown = base.Owner.IsLeftMouseButtonDown;
                bool flag2 = _activeAxis == Axis.None && SelectionCount != 0 && base.Owner.SnapToGround;
                if (flag2)
                {
                    SnapToGround();
                }
                else
                {
                    bool isActive = _isActive;
                    if (isActive)
                    {
                        _lastIntersectionPosition = _intersectPosition;
                        _intersectPosition = Vector3.Zero;
                        bool flag3 = isLeftBtnDown && _activeAxis > Axis.None;
                        if (flag3)
                        {
                            switch (_activeMode)
                            {
                                case Mode.Translate:
                                case Mode.Scale:
                                    UpdateTranslateScale();
                                    break;
                                case Mode.Rotate:
                                    UpdateRotate(dt);
                                    break;
                            }
                        }
                        else
                        {
                            bool flag4 = !isLeftBtnDown && !base.Owner.IsRightMouseButtonDown;
                            if (flag4)
                            {
                                SelectAxis();
                            }
                        }
                        UpdateGizmoPosition();
                        bool flag5 = isLeftBtnDown;
                        if (flag5)
                        {
                            bool anyValid = false;
                            Vector3 translationDelta = Vector3.Zero;
                            bool flag6 = _translationDelta.LengthSquared > 1E-06f;
                            if (flag6)
                            {
                                anyValid = true;
                                translationDelta = _translationDelta;
                                _translationDelta = Vector3.Zero;
                                Vector3 prevMoveDelta = _accMoveDelta;
                                _accMoveDelta += _translationDelta;
                                bool flag7 = _accMoveDelta.Length > base.Owner.ViewFarPlane * 0.7f;
                                if (flag7)
                                {
                                    _accMoveDelta = prevMoveDelta;
                                }
                            }
                            Quaternion rotationDelta = Quaternion.Identity;
                            bool flag8 = !_rotationDelta.IsIdentity;
                            if (flag8)
                            {
                                anyValid = true;
                                rotationDelta = _rotationDelta;
                                _rotationDelta = Quaternion.Identity;
                            }
                            Vector3 scaleDelta = Vector3.Zero;
                            bool flag9 = _scaleDelta.LengthSquared > 1E-06f;
                            if (flag9)
                            {
                                anyValid = true;
                                scaleDelta = _scaleDelta;
                                _scaleDelta = Vector3.Zero;
                                bool flag10 = ActiveAxis == Axis.Center;
                                if (flag10)
                                {
                                    scaleDelta = new Vector3(scaleDelta.AvgValue);
                                }
                            }
                            bool flag11 = anyValid || (_isTransforming && base.Owner.UseDuplicate);
                            if (flag11)
                            {
                                StartTransforming();
                                LastDelta = new Transform(translationDelta, rotationDelta, scaleDelta);
                                OnApplyTransformation(ref translationDelta, ref rotationDelta, ref scaleDelta);
                            }
                        }
                        else
                        {
                            _accMoveDelta = Vector3.Zero;
                            EndTransforming();
                        }
                    }
                }
                bool flag12 = SelectionCount == 0;
                if (flag12)
                {
                    _isActive = false;
                    _activeAxis = Axis.None;
                }
                else
                {
                    bool flag13 = !_isActive;
                    if (flag13)
                    {
                        UpdateGizmoPosition();
                    }
                    _isActive = true;
                    UpdateMatrices();
                }
            }
        }

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
        /// Checks if the specified object is selected.
        /// </summary>
        /// <param name="obj">The object to check.</param>
        /// <returns>True if it's selected, otherwise false.</returns>
        protected abstract bool IsSelected(SceneGraphNode obj);

        /// <summary>
        /// Called when user starts transforming selected objects.
        /// </summary>
        protected virtual void OnStartTransforming()
        {
            Action transformingStarted = TransformingStarted;
            if (transformingStarted != null)
            {
                transformingStarted();
            }
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
            Action transformingEnded = TransformingEnded;
            if (transformingEnded != null)
            {
                transformingEnded();
            }
        }

        /// <summary>
        /// Called when user duplicates selected objects.
        /// </summary>
        protected virtual void OnDuplicate()
        {
        }

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
        private Transform StartWorldTransform = Transform.Identity;
        private Transform WorldTransform = Transform.Identity;
        private Vector3 _intersectPosition;
        private bool _isActive;
        private bool _isDuplicating;
        private float EndRot = 0f;
        private bool _isTransforming;
        private Vector3 _lastIntersectionPosition;
        private Quaternion _rotationDelta = Quaternion.Identity;
        private float _rotationSnapDelta;
        private Vector3 _scaleDelta;
        private float _screenScale;
        private Vector3 _tDelta;
        private Vector3 _translationDelta;
        private Vector3 _translationScaleSnapDelta;
    }
}
