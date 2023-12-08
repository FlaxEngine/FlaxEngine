// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#if USE_LARGE_WORLDS
using Real = System.Double;
#else
using Real = System.Single;
#endif

using System;
using System.Collections.Generic;
using FlaxEditor.SceneGraph;
using FlaxEngine;
using FlaxEditor.Surface;

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
        private Transform _gizmoWorld = Transform.Identity;
        private Vector3 _intersectPosition;
        private bool _isActive;
        private bool _isDuplicating;

        private bool _isTransforming;
        private Vector3 _lastIntersectionPosition;

        private Quaternion _rotationDelta = Quaternion.Identity;
        private float _rotationSnapDelta;
        private Vector3 _scaleDelta;
        private float _screenScale;

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
            ModeChanged += ResetTranslationScale;
        }

        /// <summary>
        /// Starts the objects transforming (optionally with duplicate).
        /// </summary>
        public void StartTransforming()
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
                _startTransforms.Add(GetSelectedObject(i).Transform);
            }
            GetSelectedObjectsBounds(out _startBounds, out _navigationDirty);

            // Start
            _isTransforming = true;
            OnStartTransforming();
        }

        /// <summary>
        /// Ends the objects transforming.
        /// </summary>
        public void EndTransforming()
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
                        Position = GetSelectedObject(0).Transform.Translation;
                    break;
                case PivotType.SelectionCenter:
                    Position = GetSelectionCenter();
                    break;
                case PivotType.WorldOrigin:
                    Position = Vector3.Zero;
                    break;
            }
            if(verts != null)
            {
                Transform t = thisTransform;
                Vector3 selected = ((verts[selectedvert].Position * t.Orientation) * t.Scale) + t.Translation;
                Position += -(Position - selected);
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
            Vector3 position = Position;
            if (Owner.Viewport.UseOrthographicProjection)
            {
                //[hack] this is far form ideal the View Position is in wrong location, any think using the View Position will have problem
                //the camera system needs rewrite the to be a camera on springarm, similar how the ArcBallCamera is handled
                //the ortho projection cannot exist with fps camera because there is no
                // - focus point to calculate correct View Position with Orthographic Scale as a reference and Orthographic Scale from View Position
                // with make the camera jump
                // - and deaph so w and s movment in orto mode moves the cliping plane now
                float gizmoSize = Editor.Instance.Options.Options.Visual.GizmoSize;
                _screenScale = gizmoSize * (50 * Owner.Viewport.OrthographicScale);
            }
            else
            {
                Vector3 vLength = Owner.ViewPosition - position;
                float gizmoSize = Editor.Instance.Options.Options.Visual.GizmoSize;
                _screenScale = (float)(vLength.Length / GizmoScaleFactor * gizmoSize);
            }
            // Setup world
            Quaternion orientation = GetSelectedObject(0).Transform.Orientation;
            _gizmoWorld = new Transform(position, orientation, new Float3(_screenScale));
            if (_activeTransformSpace == TransformSpace.World && _activeMode != Mode.Scale)
            {
                _gizmoWorld.Orientation = Quaternion.Identity;
            }
        }

        private void UpdateTranslateScale()
        {
            bool isScaling = _activeMode == Mode.Scale;
            Vector3 delta = Vector3.Zero;
            Ray ray = Owner.MouseRay;

            Matrix.RotationQuaternion(ref _gizmoWorld.Orientation, out var rotationMatrix);
            Matrix.Invert(ref rotationMatrix, out var invRotationMatrix);
            ray.Position = Vector3.Transform(ray.Position, invRotationMatrix);
            Vector3.TransformNormal(ref ray.Direction, ref invRotationMatrix, out ray.Direction);

            var planeXY = new Plane(Vector3.Backward, Vector3.Transform(Position, invRotationMatrix).Z);
            var planeYZ = new Plane(Vector3.Left, Vector3.Transform(Position, invRotationMatrix).X);
            var planeZX = new Plane(Vector3.Down, Vector3.Transform(Position, invRotationMatrix).Y);
            var dir = Vector3.Normalize(ray.Position - Position);
            var planeDotXY = Mathf.Abs(Vector3.Dot(planeXY.Normal, dir));
            var planeDotYZ = Mathf.Abs(Vector3.Dot(planeYZ.Normal, dir));
            var planeDotZX = Mathf.Abs(Vector3.Dot(planeZX.Normal, dir));

            Real intersection;
            switch (_activeAxis)
            {
                case Axis.X:
                    {
                        var plane = planeDotXY > planeDotZX ? planeXY : planeZX;
                        if (ray.Intersects(ref plane, out intersection))
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
                        if (ray.Intersects(ref plane, out intersection))
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
                        if (ray.Intersects(ref plane, out intersection))
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
                        if (ray.Intersects(ref planeYZ, out intersection))
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
                        if (ray.Intersects(ref planeXY, out intersection))
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
                        if (ray.Intersects(ref planeZX, out intersection))
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
                        if (ray.Intersects(ref plane, out intersection))
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
                var snapValue = new Vector3(isScaling ? ScaleSnapValue : TranslationSnapValue);
                _translationScaleSnapDelta += delta;
                if (!isScaling && snapValue.X < 0.0f)
                {
                    // Snap to object bounding box
                    GetSelectedObjectsBounds(out var b, out _);
                    if (b.Minimum.X < 0.0f)
                        snapValue.X = (Real)Math.Abs(b.Minimum.X) + b.Maximum.X;
                    else
                        snapValue.X = (Real)b.Minimum.X - b.Maximum.X;
                    if (b.Minimum.Y < 0.0f)
                        snapValue.Y = (Real)Math.Abs(b.Minimum.Y) + b.Maximum.Y;
                    else
                        snapValue.Y = (Real)b.Minimum.Y - b.Maximum.Y;
                    if (b.Minimum.Z < 0.0f)
                        snapValue.Z = (Real)Math.Abs(b.Minimum.Z) + b.Maximum.Z;
                    else
                        snapValue.Z = (Real)b.Minimum.Z - b.Maximum.Z;
                }
                delta = new Vector3(
                                    (int)(_translationScaleSnapDelta.X / snapValue.X) * snapValue.X,
                                    (int)(_translationScaleSnapDelta.Y / snapValue.Y) * snapValue.Y,
                                    (int)(_translationScaleSnapDelta.Z / snapValue.Z) * snapValue.Z);
                _translationScaleSnapDelta -= delta;
            }

            if (_activeMode == Mode.Translate)
            {
                // Transform (local or world)
                delta = Vector3.Transform(delta, rotationMatrix);
                _translationDelta = delta;
            }
            else if (_activeMode == Mode.Scale)
            {
                // Scale
                _scaleDelta = delta;
            }
        }

        private void ResetTranslationScale()
        {
            _translationScaleSnapDelta.Normalize();
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
                        Float3 dir;
                        if (_activeAxis == Axis.X)
                            dir = Float3.Right * _gizmoWorld.Orientation;
                        else if (_activeAxis == Axis.Y)
                            dir = Float3.Up * _gizmoWorld.Orientation;
                        else
                            dir = Float3.Forward * _gizmoWorld.Orientation;

                        Float3 viewDir = Owner.ViewPosition - Position;
                        Float3.Dot(ref viewDir, ref dir, out float dot);
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

        //vertex snaping stff
        Mesh.Vertex[] verts;
        Mesh.Vertex[] otherVerts;
        Transform otherTransform;
        StaticModel SelectedModel;
        Transform thisTransform => SelectedModel.Transform;
        bool hasSelectedVertex;
        int selectedvert;
        int otherSelectedvert;
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
                SnapToGround();
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
                        case Mode.Translate:
                            UpdateTranslateScale();
                            VertexSnap();
                            break;
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
                    // If nothing selected, try to select any axis
                    if (!isLeftBtnDown && !Owner.IsRightMouseButtonDown)
                    {
                        if (Owner.IsAltKeyDown && _activeMode == Mode.Translate)
                            SelectVertexSnaping();
                        SelectAxis();
                    }
                    else
                    {
                        verts = null;
                        otherVerts = null;
                    }
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
                    if (anyValid || (_isTransforming && Owner.UseDuplicate))
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
        void SelectVertexSnaping()
        {
            Vector3 point = Vector3.Zero;
            var ray = Owner.MouseRay;
            Real lastdistance = Real.MaxValue;
            StaticModel Lastmodel = null;
            int index = 0;

            int i = 0;
            //get first
            for (; i < SelectionCount; i++)
            {
                if (GetSelectedObject(i).EditableObject is StaticModel model)
                {
                    var bb = model.EditorBox;
                    if (CollisionsHelper.RayIntersectsBox(ref ray, ref bb, out Vector3 p))
                    {
                        Lastmodel = model;
                        point = p;
                        index = 0;
                        break;
                    }
                }
            }

            if (Lastmodel == null) // nothing to do return
                return;

            //find closest bounding box in selection
            for (; i < SelectionCount; i++)
            {
                if (GetSelectedObject(i).EditableObject is StaticModel model)
                {
                    var bb = model.EditorBox;
                    //check for other we might have one closer
                    var d = Vector3.Distance(model.Transform.Translation, ray.Position);
                    if (lastdistance < d)
                    {
                        if (CollisionsHelper.RayIntersectsBox(ref ray, ref bb, out Vector3 p))
                        {
                            lastdistance = d;
                            Lastmodel = model;
                            point = p;
                            index = i;
                        }
                    }
                }
            }
            SelectedModel = Lastmodel;

            //find closest vertex to bounding box point (collision detection approximation)
            //[ToDo] replace this with collision detection with is suporting concave shapes (compute shader) 
            point = thisTransform.WorldToLocal(point);

            //[To Do] comlite this  there is not suport for multy mesh model
            verts = Lastmodel.Model.LODs[0].Meshes[0].DownloadVertexBuffer();

            lastdistance = Vector3.Distance(point, verts[0].Position);
            for (int j = 0; j < verts.Length; j++)
            {
                var d = Vector3.Distance(point, verts[j].Position);
                if (d <= lastdistance)
                {
                    lastdistance = d;
                    selectedvert = j;
                }

            }
        }
        void VertexSnap()
        {
            Profiler.BeginEvent("VertexSnap");
            //ray cast others
            if (verts != null)
            {
                var ray = Owner.MouseRay;

                SceneGraphNode.RayCastData rayCast = new SceneGraphNode.RayCastData()
                {
                    Ray = ray,
                    Exclude = new List<Actor>() {},
                    Scan = new List<Type>() { typeof(StaticModel) }
                };
                for (int i = 0; i < SelectionCount; i++)
                {
                    rayCast.Exclude.Add((Actor)GetSelectedObject(i).EditableObject);
                }
                //grab scene and raycast
                var actor = GetSelectedObject(0).ParentScene.RayCast(ref rayCast, out var distance, out var _);
                if (actor != null)
                {
                    if (actor.EditableObject is StaticModel model)
                    {
                        otherTransform = model.Transform;
                        Vector3 p = rayCast.Ray.Position + (rayCast.Ray.Direction * distance);

                        //[To Do] comlite this  there is not suport for multy mesh model
                        otherVerts = model.Model.LODs[0].Meshes[0].DownloadVertexBuffer();

                        //find closest vertex to bounding box point (collision detection approximation)
                        //[ToDo] replace this with collision detection with is suporting concave shapes (compute shader)
                        p = actor.Transform.WorldToLocal(p);
                        Real lastdistance = Vector3.Distance(p, otherVerts[0].Position);
                        for (int i = 0; i < otherVerts.Length; i++)
                        {
                            var d = Vector3.Distance(p, otherVerts[i].Position);
                            if (d <= lastdistance)
                            {
                                lastdistance = d;
                                otherSelectedvert = i;
                            }
                        }

                        if(lastdistance > 25)
                        {
                            otherSelectedvert = -1;
                            otherVerts = null;
                        }
                    }
                }
            }

            if (verts != null && otherVerts != null)
            {
                Transform t = thisTransform;
                Vector3 selected = ((verts[selectedvert].Position * t.Orientation) * t.Scale) + t.Translation;

                t = otherTransform;
                Vector3 other = ((otherVerts[otherSelectedvert].Position * t.Orientation) * t.Scale) + t.Translation;

                // Translation
                var projection = -(selected - other);

                //at some point
                //Quaternion inverse = t.Orientation;
                //inverse.Invert();
                //projection *= inverse;                                       //world to local
                //flip mask
                //var Not = ~_activeAxis;
                //LockAxisWorld(Not, ref projection);                          //lock axis
                //projection *= t.Orientation;                                 //local to world

                _translationDelta = projection;
            }

            Profiler.EndEvent();
        }

        /// <summary>
        /// zeros the <see cref="Vector3"/> <paramref name="v"/> component using the <see cref="Axis"/> <paramref name="lockAxis"/>
        /// </summary>
        public static void LockAxisWorld(Axis lockAxis, ref Vector3 v)
        {
            if (lockAxis.HasFlag(Axis.X))
                v.X = 0;
            if (lockAxis.HasFlag(Axis.Y))
                v.Y = 0;
            if (lockAxis.HasFlag(Axis.Z))
                v.Z = 0;
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
        protected abstract SceneGraphNode GetSelectedObject(int index);

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
