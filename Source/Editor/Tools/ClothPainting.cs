// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor;
using FlaxEditor.Gizmo;
using FlaxEditor.SceneGraph;
using FlaxEditor.Viewport.Modes;

namespace FlaxEngine.Tools
{
    sealed class ClothPaintingGizmoMode : EditorGizmoMode
    {
        [HideInEditor]
        public ClothPaintingGizmo Gizmo;

#pragma warning disable CS0649
        /// <summary>
        /// Brush radius (world-space).
        /// </summary>
        [Limit(0)]
        public float BrushSize = 50.0f;

        /// <summary>
        /// Brush paint intensity.
        /// </summary>
        [Limit(0)]
        public float BrushStrength = 2.0f;

        /// <summary>
        /// Brush paint falloff. Hardens or softens painting.
        /// </summary>
        [Limit(0)]
        public float BrushFalloff = 1.5f;

        /// <summary>
        /// Value to paint with. Hold Ctrl hey to paint with inverse value (1 - value).
        /// </summary>
        [Limit(0, 1, 0.01f)]
        public float PaintValue = 0.0f;

        /// <summary>
        /// Enables continuous painting, otherwise single paint on click.
        /// </summary>
        public bool ContinuousPaint;

        /// <summary>
        /// Enables drawing cloth paint debugging with Depth Test enabled (skips occluded vertices).
        /// </summary>
        public bool DebugDrawDepthTest
        {
            get => Gizmo.Cloth?.DebugDrawDepthTest ?? true;
            set
            {
                if (Gizmo.Cloth != null)
                    Gizmo.Cloth.DebugDrawDepthTest = value;
            }
        }
#pragma warning restore CS0649

        public override void Init(IGizmoOwner owner)
        {
            base.Init(owner);

            Gizmo = new ClothPaintingGizmo(owner, this);
        }

        public override void OnActivated()
        {
            base.OnActivated();

            Owner.Gizmos.Active = Gizmo;
        }

        public override void Dispose()
        {
            Owner.Gizmos.Remove(Gizmo);
            Gizmo = null;

            base.Dispose();
        }
    }

    sealed class ClothPaintingGizmo : GizmoBase
    {
        private Model _brushModel;
        private MaterialInstance _brushMaterial;
        private ClothPaintingGizmoMode _gizmoMode;
        private bool _isPainting;
        private int _paintUpdateCount;
        private bool _hasHit;
        private Vector3 _hitLocation;
        private Vector3 _hitNormal;
        private Cloth _cloth;
        private Float3[] _clothParticles;
        private float[] _clothPaint;
        private EditClothPaintAction _undoAction;

        public bool IsPainting => _isPainting;
        public Cloth Cloth => _cloth;

        public ClothPaintingGizmo(IGizmoOwner owner, ClothPaintingGizmoMode mode)
        : base(owner)
        {
            _gizmoMode = mode;
        }

        public void SetPaintCloth(Cloth cloth)
        {
            if (_cloth == cloth)
                return;
            PaintEnd();
            _cloth = cloth;
            _clothParticles = null;
            _clothPaint = null;
            _hasHit = false;
            if (cloth != null)
            {
                _clothParticles = cloth.GetParticles();
                if (_clothParticles == null || _clothParticles.Length == 0)
                {
                    // Setup cloth to get proper particles (eg. if cloth is not on a scene like in Prefab Window)
                    cloth.Rebuild();
                    _clothParticles = cloth.GetParticles();
                }
            }
        }

        public void Fill()
        {
            PaintEnd();
            PaintStart();
            var clothPaint = _clothPaint;
            var paintValue = Mathf.Saturate(_gizmoMode.PaintValue);
            for (int i = 0; i < clothPaint.Length; i++)
                clothPaint[i] = paintValue;
            _cloth.SetPaint(clothPaint);
            PaintEnd();
        }

        public void Reset()
        {
            PaintEnd();
            PaintStart();
            var clothPaint = _clothPaint;
            for (int i = 0; i < clothPaint.Length; i++)
                clothPaint[i] = 1.0f;
            _cloth.SetPaint(clothPaint);
            PaintEnd();
        }

        private void PaintStart()
        {
            if (IsPainting)
                return;

            if (Owner.Undo.Enabled)
                _undoAction = new EditClothPaintAction(_cloth);
            _isPainting = true;
            _paintUpdateCount = 0;

            // Get initial cloth paint state
            var clothParticles = _clothParticles;
            var clothPaint = _cloth.GetPaint();
            if (clothPaint == null || clothPaint.Length != clothParticles.Length)
            {
                _clothPaint = clothPaint = new float[clothParticles.Length];
                for (int i = 0; i < clothPaint.Length; i++)
                    clothPaint[i] = 1.0f;
            }
            _clothPaint = clothPaint;
        }

        private void PaintUpdate()
        {
            if (!_gizmoMode.ContinuousPaint && _paintUpdateCount > 0)
                return;
            Profiler.BeginEvent("Cloth Paint");

            // Edit the cloth paint
            var clothParticles = _clothParticles;
            var clothPaint = _clothPaint;
            if (clothParticles == null || clothPaint == null)
                throw new Exception();
            var instanceTransform = _cloth.Transform;
            var brushSphere = new BoundingSphere(_hitLocation, _gizmoMode.BrushSize);
            var paintValue = Mathf.Saturate(_gizmoMode.PaintValue);
            if (Owner.IsControlDown)
                paintValue = 1.0f - paintValue;
            var modified = false;
            for (int i = 0; i < clothParticles.Length; i++)
            {
                var pos = instanceTransform.LocalToWorld(clothParticles[i]);
                var dst = Vector3.Distance(ref pos, ref brushSphere.Center);
                if (dst > brushSphere.Radius)
                    continue;
                float strength = _gizmoMode.BrushStrength * Mathf.Lerp(1.0f, 1.0f - (float)dst / (float)brushSphere.Radius, _gizmoMode.BrushFalloff);
                if (strength > Mathf.Epsilon)
                {
                    // Paint the particle
                    ref var paint = ref clothPaint[i];
                    paint = Mathf.Saturate(Mathf.Lerp(paint, paintValue, Mathf.Saturate(strength)));
                    modified = true;
                }
            }
            _paintUpdateCount++;
            if (modified)
            {
                // Update cloth particles state
                _cloth.SetPaint(clothPaint);
            }

            Profiler.EndEvent();
        }

        private void PaintEnd()
        {
            if (!IsPainting)
                return;

            if (_undoAction != null)
            {
                _undoAction.RecordEnd();
                Owner.Undo.AddAction(_undoAction);
                _undoAction = null;
            }
            _isPainting = false;
            _paintUpdateCount = 0;
            _clothPaint = null;
        }

        public override bool IsControllingMouse => IsPainting;

        public override BoundingSphere FocusBounds => _cloth?.Sphere ?? base.FocusBounds;

        public override void Update(float dt)
        {
            _hasHit = false;
            if (!IsActive)
            {
                SetPaintCloth(null);
                return;
            }
            var cloth = _cloth;
            if (cloth == null)
                return;
            var hasPaintInput = Owner.IsLeftMouseButtonDown && !Owner.IsAltKeyDown;

            // Perform detailed tracing to find cursor location for the brush
            var ray = Owner.MouseRay;
            if (cloth.IntersectsItself(ray, out var closest, out var hitNormal))
            {
                // Cursor hit cloth
                _hasHit = true;
                _hitLocation = ray.GetPoint(closest);
                _hitNormal = hitNormal;
            }
            else
            {
                // Cursor hit other object or nothing
                PaintEnd();

                if (hasPaintInput)
                {
                    // Select something else
                    var view = new Ray(Owner.ViewPosition, Owner.ViewDirection);
                    var rayCastFlags = SceneGraphNode.RayCastData.FlagTypes.SkipColliders | SceneGraphNode.RayCastData.FlagTypes.SkipEditorPrimitives;
                    var hit = Owner.SceneGraphRoot.RayCast(ref ray, ref view, out _, rayCastFlags);
                    if (hit != null && hit is ActorNode)
                        Owner.Select(new List<SceneGraphNode> { hit });
                }
                return;
            }

            // Handle painting
            if (hasPaintInput)
                PaintStart();
            else
                PaintEnd();
            if (IsPainting)
                PaintUpdate();
        }

        public override void Draw(ref RenderContext renderContext)
        {
            if (!IsActive || !_cloth)
                return;

            base.Draw(ref renderContext);

            // TODO: impl this
            if (_hasHit)
            {
                var viewOrigin = renderContext.View.Origin;

                // Draw paint brush
                if (!_brushModel)
                    _brushModel = Content.LoadAsyncInternal<Model>("Editor/Primitives/Sphere");
                if (!_brushMaterial)
                    _brushMaterial = Content.LoadAsyncInternal<Material>(EditorAssets.FoliageBrushMaterial)?.CreateVirtualInstance();
                if (_brushModel && _brushMaterial)
                {
                    _brushMaterial.SetParameterValue("Color", new Color(1.0f, 0.85f, 0.0f)); // TODO: expose to editor options
                    _brushMaterial.SetParameterValue("DepthBuffer", Owner.RenderTask.Buffers.DepthBuffer);
                    Quaternion rotation = RootNode.RaycastNormalRotation(ref _hitNormal);
                    Matrix transform = Matrix.Scaling(_gizmoMode.BrushSize * 0.01f) * Matrix.RotationQuaternion(rotation) * Matrix.Translation(_hitLocation - viewOrigin);
                    _brushModel.Draw(ref renderContext, _brushMaterial, ref transform);
                }
            }
        }

        public override void OnActivated()
        {
            base.OnActivated();

            _hasHit = false;
        }

        public override void OnDeactivated()
        {
            base.OnDeactivated();

            PaintEnd();
            SetPaintCloth(null);
            Object.Destroy(ref _brushMaterial);
            _brushModel = null;
        }
    }

    sealed class EditClothPaintAction : IUndoAction
    {
        private Guid _actorId;
        private string _before, _after;

        public EditClothPaintAction(Cloth cloth)
        {
            _actorId = cloth.ID;
            _before = GetState(cloth);
        }

        public static bool IsValidState(string state)
        {
            return state != null && state.Contains("\"Paint\":");
        }

        public static string GetState(Cloth cloth)
        {
            var json = cloth.ToJson();
            var start = json.IndexOf("\"Paint\":");
            if (start == -1)
                return null;
            var end = json.IndexOf('\"', json.IndexOf('\"', start + 8) + 1);
            json = "{" + json.Substring(start, end - start) + "\"}";
            return json;
        }

        public static void SetState(Cloth cloth, string state)
        {
            if (state == null)
                cloth.SetPaint(null);
            else
                Editor.Internal_DeserializeSceneObject(Object.GetUnmanagedPtr(cloth), state);
        }

        public void RecordEnd()
        {
            var cloth = Object.Find<Cloth>(ref _actorId);
            _after = GetState(cloth);
            Editor.Instance.Scene.MarkSceneEdited(cloth.Scene);
        }

        private void Set(string state)
        {
            var cloth = Object.Find<Cloth>(ref _actorId);
            SetState(cloth, state);
            Editor.Instance.Scene.MarkSceneEdited(cloth.Scene);
        }

        public string ActionString => "Edit Cloth Paint";

        public void Do()
        {
            Set(_after);
        }

        public void Undo()
        {
            Set(_before);
        }

        public void Dispose()
        {
            _before = _after = null;
        }
    }
}
