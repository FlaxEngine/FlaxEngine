// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using FlaxEditor.Gizmo;
using FlaxEditor.GUI.Timeline.Tracks;
using FlaxEditor.GUI.Timeline.Undo;
using FlaxEditor.SceneGraph;
using FlaxEngine;

namespace FlaxEditor.GUI.Timeline
{
    /// <summary>
    /// Gizmo for editing position curve. Managed by the <see cref="SceneAnimationTimeline"/>.
    /// </summary>
    sealed class EditCurveTrackGizmo : TransformGizmoBase
    {
        public const float KeyframeSize = 10.0f;
        public const float TangentSize = 6.0f;

        private readonly SceneAnimationTimeline _timeline;
        private CurvePropertyTrack _track;
        private int _keyframe = -1;
        private int _item = -1;
        private byte[] _curveEditingStartData;

        public int Keyframe => _keyframe;
        public int Item => _item;

        public EditCurveTrackGizmo(IGizmoOwner owner, SceneAnimationTimeline timeline)
        : base(owner)
        {
            _timeline = timeline;
        }

        public void SelectKeyframe(CurvePropertyTrack track, int keyframe, int item)
        {
            _track = track;
            _keyframe = keyframe;
            _item = item;
        }

        public override BoundingSphere FocusBounds
        {
            get
            {
                if (_track == null)
                    return BoundingSphere.Empty;
                var curve = (BezierCurveEditor<Vector3>)_track.Curve;
                var keyframes = curve.Keyframes;
                var k = keyframes[_keyframe];
                if (_item == 1)
                    k.Value += k.TangentIn;
                else if (_item == 2)
                    k.Value += k.TangentOut;
                return new BoundingSphere(k.Value, KeyframeSize);
            }
        }

        protected override int SelectionCount => _track != null ? 1 : 0;

        protected override SceneGraphNode GetSelectedObject(int index)
        {
            return null;
        }

        protected override Transform GetSelectedTransform(int index)
        {
            if (_track == null)
                return Transform.Identity;
            var curve = (BezierCurveEditor<Vector3>)_track.Curve;
            var keyframes = curve.Keyframes;
            var k = keyframes[_keyframe];
            if (_item == 1)
                k.Value += k.TangentIn;
            else if (_item == 2)
                k.Value += k.TangentOut;
            return new Transform(k.Value);
        }

        protected override void GetSelectedObjectsBounds(out BoundingBox bounds, out bool navigationDirty)
        {
            bounds = BoundingBox.Empty;
            navigationDirty = false;
            if (_track == null)
                return;
            var curve = (BezierCurveEditor<Vector3>)_track.Curve;
            var keyframes = curve.Keyframes;
            var k = keyframes[_keyframe];
            if (_item == 1)
                k.Value += k.TangentIn;
            else if (_item == 2)
                k.Value += k.TangentOut;
            bounds = new BoundingBox(k.Value - KeyframeSize, k.Value + KeyframeSize);
        }

        protected override bool IsSelected(SceneGraphNode obj)
        {
            return false;
        }

        protected override void OnStartTransforming()
        {
            base.OnStartTransforming();

            // Start undo
            _curveEditingStartData = EditTrackAction.CaptureData(_track);
        }

        protected override void OnEndTransforming()
        {
            base.OnEndTransforming();
            
            // End undo
            var after = EditTrackAction.CaptureData(_track);
            if (!Utils.ArraysEqual(_curveEditingStartData, after))
                _track.Timeline.AddBatchedUndoAction(new EditTrackAction(_track.Timeline, _track, _curveEditingStartData, after));
            _curveEditingStartData = null;
        }

        protected override void OnApplyTransformation(ref Vector3 translationDelta, ref Quaternion rotationDelta, ref Vector3 scaleDelta)
        {
            base.OnApplyTransformation(ref translationDelta, ref rotationDelta, ref scaleDelta);

            var curve = (BezierCurveEditor<Vector3>)_track.Curve;
            var keyframes = curve.Keyframes;
            var k = keyframes[_keyframe];
            if (_item == 0)
                k.Value += translationDelta;
            else if (_item == 1)
                k.TangentIn += translationDelta;
            else if (_item == 2)
                k.TangentOut += translationDelta;
            curve.SetKeyframeValue(_keyframe, k.Value, k.TangentIn, k.TangentOut);
        }

        public override void Pick()
        {
            if (_track == null)
                return;
            var selectRay = Owner.MouseRay;
            var curve = (BezierCurveEditor<Vector3>)_track.Curve;
            var keyframes = curve.Keyframes;
            for (var i = 0; i < keyframes.Count; i++)
            {
                var k = keyframes[i];

                var sphere = new BoundingSphere(k.Value, KeyframeSize);
                if (sphere.Intersects(ref selectRay))
                {
                    SelectKeyframe(_track, i, 0);
                    return;
                }

                if (!k.TangentIn.IsZero)
                {
                    var t = k.Value + k.TangentIn;
                    var box = BoundingBox.FromSphere(new BoundingSphere(t, TangentSize));
                    if (box.Intersects(ref selectRay))
                    {
                        SelectKeyframe(_track, i, 1);
                        return;
                    }
                }

                if (!k.TangentOut.IsZero)
                {
                    var t = k.Value + k.TangentOut;
                    var box = BoundingBox.FromSphere(new BoundingSphere(t, TangentSize));
                    if (box.Intersects(ref selectRay))
                    {
                        SelectKeyframe(_track, i, 2);
                        return;
                    }
                }
            }
        }

        public override void Update(float dt)
        {
            base.Update(dt);

            // Deactivate when track gets deselected
            if (_track != null && !_timeline.SelectedTracks.Contains(_track))
                Owner.Gizmos.Active = Owner.Gizmos.Get<TransformGizmo>();
        }

        public override void OnActivated()
        {
            base.OnActivated();

            ActiveMode = Mode.Translate;
        }

        public override void OnDeactivated()
        {
            // Clear selection
            _track = null;
            _keyframe = -1;
            _item = -1;

            base.OnDeactivated();
        }
    }
}
