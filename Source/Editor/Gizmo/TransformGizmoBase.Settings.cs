// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;

namespace FlaxEditor.Gizmo
{
    public partial class TransformGizmoBase
    {
        private const float GizmoScaleFactor = 18;
        private const float LineLength = 3.0f;
        private const float LineOffset = 1.0f;
        private const float MultiAxisThickness = 0.05f;
        private const float SingleAxisThickness = 0.3f;
        private const float ScaleSpheresRadius = 0.7f;
        private const float CenterBoxSize = 0.8f;
        private const float CenterSphereRadius = 0.1f;
        private const float HalfLineOffset = LineOffset / 2;

        private readonly Vector3[] _translationLineVertices =
        {
            // -- X Axis -- // index 0 - 5
            new Vector3(HalfLineOffset, 0, 0),
            new Vector3(LineLength, 0, 0),
            new Vector3(LineOffset, 0, 0),
            new Vector3(LineOffset, LineOffset, 0),
            new Vector3(LineOffset, 0, 0),
            new Vector3(LineOffset, 0, LineOffset),

            // -- Y Axis -- // index 6 - 11
            new Vector3(0, HalfLineOffset, 0),
            new Vector3(0, LineLength, 0),
            new Vector3(0, LineOffset, 0),
            new Vector3(LineOffset, LineOffset, 0),
            new Vector3(0, LineOffset, 0),
            new Vector3(0, LineOffset, LineOffset),

            // -- Z Axis -- // index 12 - 17
            new Vector3(0, 0, HalfLineOffset),
            new Vector3(0, 0, LineLength),
            new Vector3(0, 0, LineOffset),
            new Vector3(LineOffset, 0, LineOffset),
            new Vector3(0, 0, LineOffset),
            new Vector3(0, LineOffset, LineOffset)
        };

        private BoundingBox XAxisBox = new BoundingBox(new Vector3(LineOffset, -SingleAxisThickness, -SingleAxisThickness), new Vector3(LineOffset + LineLength, SingleAxisThickness, SingleAxisThickness));
        private BoundingBox YAxisBox = new BoundingBox(new Vector3(-SingleAxisThickness, LineOffset, -SingleAxisThickness), new Vector3(SingleAxisThickness, LineOffset + LineLength, SingleAxisThickness));
        private BoundingBox ZAxisBox = new BoundingBox(new Vector3(-SingleAxisThickness, -SingleAxisThickness, LineOffset), new Vector3(SingleAxisThickness, SingleAxisThickness, LineOffset + LineLength));
        private BoundingBox XZBox = new BoundingBox(Vector3.Zero, new Vector3(LineOffset, MultiAxisThickness, LineOffset));
        private BoundingBox XYBox = new BoundingBox(Vector3.Zero, new Vector3(LineOffset, LineOffset, MultiAxisThickness));
        private BoundingBox YZBox = new BoundingBox(Vector3.Zero, new Vector3(MultiAxisThickness, LineOffset, LineOffset));
        private BoundingBox CenterBoxRaw = new BoundingBox(new Vector3(-0.5f * CenterBoxSize), new Vector3(0.5f * CenterBoxSize));
        private float RotateRadiusRaw = 4.0f;

        private BoundingSphere ScaleXSphere => new BoundingSphere(Vector3.Transform(_translationLineVertices[1], _gizmoWorld), ScaleSpheresRadius * _screenScale);
        private BoundingSphere ScaleYSphere => new BoundingSphere(Vector3.Transform(_translationLineVertices[7], _gizmoWorld), ScaleSpheresRadius * _screenScale);
        private BoundingSphere ScaleZSphere => new BoundingSphere(Vector3.Transform(_translationLineVertices[13], _gizmoWorld), ScaleSpheresRadius * _screenScale);
        private OrientedBoundingBox CenterBox => new OrientedBoundingBox(CenterBoxRaw) * _gizmoWorld;

        private Mode _activeMode = Mode.Translate;
        private Axis _activeAxis = Axis.None;
        private TransformSpace _activeTransformSpace = TransformSpace.World;
        private PivotType _activePivotType = PivotType.SelectionCenter;

        /// <summary>
        /// True if enable grid snapping when moving objects
        /// </summary>
        public bool TranslationSnapEnable = false;

        /// <summary>
        /// True if enable grid snapping when rotating objects
        /// </summary>
        public bool RotationSnapEnabled = false;

        /// <summary>
        /// True if enable grid snapping when scaling objects
        /// </summary>
        public bool ScaleSnapEnabled = false;

        /// <summary>
        /// Translation snap value
        /// </summary>
        public float TranslationSnapValue = 10;

        /// <summary>
        /// Rotation snap value
        /// </summary>
        public float RotationSnapValue = 15;

        /// <summary>
        /// Scale snap value
        /// </summary>
        public float ScaleSnapValue = 1.0f;

        /// <summary>
        /// Gets the current pivot type.
        /// </summary>
        public PivotType ActivePivot => _activePivotType;

        /// <summary>
        /// Gets the current axis type.
        /// </summary>
        public Axis ActiveAxis => _activeAxis;

        /// <summary>
        /// Gets or sts the current gizmo mode.
        /// </summary>
        public Mode ActiveMode
        {
            get => _activeMode;
            set
            {
                if (_activeMode != value)
                {
                    _activeMode = value;
                    ModeChanged?.Invoke();
                }
            }
        }

        /// <summary>
        /// Event fired when active gizmo mode gets changed.
        /// </summary>
        public Action ModeChanged;

        /// <summary>
        /// Gets or sets the current gizmo transform space.
        /// </summary>
        public TransformSpace ActiveTransformSpace
        {
            get => _activeTransformSpace;
            set
            {
                if (_activeTransformSpace != value)
                {
                    _activeTransformSpace = value;
                    TransformSpaceChanged?.Invoke();
                }
            }
        }

        /// <summary>
        /// Event fired when active transform space gets changed.
        /// </summary>
        public Action TransformSpaceChanged;

        /// <summary>
        /// Toggles gizmo transform space
        /// </summary>
        public void ToggleTransformSpace()
        {
            ActiveTransformSpace = _activeTransformSpace == TransformSpace.World ? TransformSpace.Local : TransformSpace.World;
        }
    }
}
