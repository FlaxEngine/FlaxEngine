// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;

namespace FlaxEditor.Gizmo
{
    public partial class TransformGizmoBase
    {
        /// <summary>
        /// Scale of the gizmo itself
        /// </summary>
        private const float GizmoScaleFactor = 24;

        /// <summary>
        /// The length of each axis (outwards)
        /// </summary>
        private const float AxisLength = 3.5f;

        /// <summary>
        /// Offset to move axis away from center
        /// </summary>
        private const float AxisOffset = 1.2f;

        /// <summary>
        /// How thick the axis should be
        /// </summary>
        private const float AxisThickness = 0.3f;

        /// <summary>
        /// Center box scale
        /// </summary>
        private const float CenterBoxScale = 0.8f;

        /// <summary>
        /// The inner minimum of the multiscale
        /// </summary>
        private const float InnerExtend = AxisOffset + 0.5f;

        /// <summary>
        /// The outer maximum of the multiscale
        /// </summary>
        private const float OuterExtend = AxisOffset * 3.5f;

        // Cube with the size AxisThickness, then moves it along the axis (AxisThickness) and finally makes it really long (AxisLength)
        private BoundingBox XAxisBox = new BoundingBox(new Vector3(-AxisThickness), new Vector3(AxisThickness)).MakeOffsetted(AxisOffset * Vector3.UnitX).Merge(AxisLength * Vector3.UnitX);
        private BoundingBox YAxisBox = new BoundingBox(new Vector3(-AxisThickness), new Vector3(AxisThickness)).MakeOffsetted(AxisOffset * Vector3.UnitY).Merge(AxisLength * Vector3.UnitY);
        private BoundingBox ZAxisBox = new BoundingBox(new Vector3(-AxisThickness), new Vector3(AxisThickness)).MakeOffsetted(AxisOffset * Vector3.UnitZ).Merge(AxisLength * Vector3.UnitZ);

        private BoundingBox XZBox = new BoundingBox(new Vector3(InnerExtend, 0, InnerExtend), new Vector3(OuterExtend, 0, OuterExtend));
        private BoundingBox XYBox = new BoundingBox(new Vector3(InnerExtend, InnerExtend, 0), new Vector3(OuterExtend, OuterExtend, 0));
        private BoundingBox YZBox = new BoundingBox(new Vector3(0, InnerExtend, InnerExtend), new Vector3(0, OuterExtend, OuterExtend));

        private BoundingBox CenterBoxRaw = new BoundingBox(new Vector3(-0.5f * CenterBoxScale), new Vector3(0.5f * CenterBoxScale));
        private OrientedBoundingBox CenterBox => new OrientedBoundingBox(CenterBoxRaw) * _gizmoWorld;
        private const float RotateRadiusRaw = 4.0f;

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
        /// Gets or sets the current gizmo mode.
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
