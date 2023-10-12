// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.Gizmo
{
    public partial class TransformGizmoBase
    {
        internal Matrix MatrixCenter;
        internal Matrix MatrixGizmoTransform;

        /// <inheritdoc />
        public override void Draw(ref RenderContext renderContext)
        {
            if (IsActive && SelectionCount != 0)
            {
                if (_isTransforming)
                {
                    Vector3 dir = Vector3.Zero;
                    //draw axis lines when trasforming
                    if (_activeAxis.HasFlag(Axis.X))
                    {
                        dir = WorldTransform.Right;
                        DebugDraw.DrawLine(WorldTransform.Right * 100000f + WorldTransform.Translation, WorldTransform.Left * 100000f + WorldTransform.Translation, XAxisColor, 0f, true);
                    }
                    if (_activeAxis.HasFlag(Axis.Y))
                    {
                        dir = WorldTransform.Up;
                        DebugDraw.DrawLine(WorldTransform.Up * 100000f + WorldTransform.Translation, WorldTransform.Down * 100000f + WorldTransform.Translation, YAxisColor, 0f, true);
                    }
                    if (_activeAxis.HasFlag(Axis.Z))
                    {
                        dir = WorldTransform.Forward;
                        DebugDraw.DrawLine(WorldTransform.Forward * 100000f + WorldTransform.Translation, WorldTransform.Backward * 100000f + WorldTransform.Translation, ZAxisColor, 0f, true);
                    }

                    if (_activeMode == Mode.Rotate)
                    {
                        if (!dir.IsZero)
                        {
                            ComputeCenterMatrix(Quaternion.FromDirection(dir));
                        }
                        _materialCenter.SetParameterValue("RotacionStartEnd", new Float2(Vector3.Angle(StartWorldTransform.Up, dir), EndRot), true);
                        _modelPlane.Draw(ref renderContext, _materialCenter, ref MatrixCenter, StaticFlags.None, false, 0);
                    }
                    else
                    {
                        //simple bilboard
                        var q = Owner.Viewport.Camera.Orientation * Quaternion.Euler(0, 180, 0);

                        var distance = StartWorldTransform.Translation - WorldTransform.Translation;
                        var scale = WorldTransform.Scale * 0.1f;

                        if (_activeAxis.HasFlag(Axis.X))
                        {
                            var TextX = new Transform(distance * WorldTransform.Right + WorldTransform.Translation, q, scale);
                            DebugDraw.DrawText(distance.X.ToString(), TextX, XAxisColor);
                        }
                        if (_activeAxis.HasFlag(Axis.Y))
                        {
                            var TextY = new Transform(distance * WorldTransform.Up + WorldTransform.Translation, q, scale);
                            DebugDraw.DrawText(distance.Y.ToString(), TextY, YAxisColor);
                            DebugDraw.DrawLine(WorldTransform.Up * 100000f + WorldTransform.Translation, WorldTransform.Down * 100000f + WorldTransform.Translation, YAxisColor, 0f, true);
                        }
                        if (_activeAxis.HasFlag(Axis.Z))
                        {
                            var TextZ = new Transform(distance * WorldTransform.Forward + WorldTransform.Translation, q, scale);
                            DebugDraw.DrawText(distance.Z.ToString(), TextZ, ZAxisColor);
                            DebugDraw.DrawLine(WorldTransform.Forward * 100000f + WorldTransform.Translation, WorldTransform.Backward * 100000f + WorldTransform.Translation, ZAxisColor, 0f, true);
                        }
                    }

                    DebugDraw.DrawLine(StartWorldTransform.Translation, WorldTransform.Translation, CenterColor, 0f, true);
                }
                else
                {
                    ComputeNewMatrixis();
                    switch (_activeMode)
                    {
                        case Mode.Translate:
                            DrawTranslationGizmo(ref renderContext);
                            _materialAxis[0].SetParameterValue("IsSelected", _activeAxis.HasFlag(Axis.X), true);
                            _materialAxis[1].SetParameterValue("IsSelected", _activeAxis.HasFlag(Axis.Y), true);
                            _materialAxis[2].SetParameterValue("IsSelected", _activeAxis.HasFlag(Axis.Z), true);
                            _materialAxisPlane[0].SetParameterValue("IsSelected", _activeAxis.HasFlag(Axis.YZ), true);
                            _materialAxisPlane[1].SetParameterValue("IsSelected", _activeAxis.HasFlag(Axis.ZX), true);
                            _materialAxisPlane[2].SetParameterValue("IsSelected", _activeAxis.HasFlag(Axis.XY), true);
                            _materialCenter.SetParameterValue("IsSelected", _activeAxis.HasFlag(Axis.View), true);
                            _materialCenter.SetParameterValue("SnapingValue", 0, true);
                            break;
                        case Mode.Rotate:
                            DrawRotationGizmo(ref renderContext);
                            _materialAxis[0].SetParameterValue("IsSelected", _activeAxis.HasFlag(Axis.X), true);
                            _materialAxis[1].SetParameterValue("IsSelected", _activeAxis.HasFlag(Axis.Y), true);
                            _materialAxis[2].SetParameterValue("IsSelected", _activeAxis.HasFlag(Axis.Z), true);
                            _materialCenter.SetParameterValue("IsSelected", _activeAxis.HasFlag(Axis.View), true);
                            _materialCenter.SetParameterValue("SnapingValue", RotationSnapEnabled ? RotationSnapValue : 0f, true);
                            _materialRotCenter.SetParameterValue("IsSelected", _activeAxis.HasFlag(Axis.Center), true);
                            break;
                        case Mode.Scale:
                            DrawScaleGizmo(ref renderContext);
                            _materialAxis[0].SetParameterValue("IsSelected", _activeAxis.HasFlag(Axis.X), true);
                            _materialAxis[1].SetParameterValue("IsSelected", _activeAxis.HasFlag(Axis.Y), true);
                            _materialAxis[2].SetParameterValue("IsSelected", _activeAxis.HasFlag(Axis.Z), true);
                            _materialAxisPlane[0].SetParameterValue("IsSelected", _activeAxis.HasFlag(Axis.YZ), true);
                            _materialAxisPlane[1].SetParameterValue("IsSelected", _activeAxis.HasFlag(Axis.ZX), true);
                            _materialAxisPlane[2].SetParameterValue("IsSelected", _activeAxis.HasFlag(Axis.XY), true);
                            _materialCenter.SetParameterValue("IsSelected", _activeAxis.HasFlag(Axis.View), true);
                            _materialCenter.SetParameterValue("SnapingValue", 0, true);
                            break;
                    }
                }
            }
        }

        private void DrawTranslationGizmo(ref RenderContext renderContext)
        {
                _modelPlaneAxisGizmo.Draw(ref renderContext, _materialAxisPlane[0], ref MatrixGizmoTransform, StaticFlags.None, false, 0);
                _modelLocationAxisGizmo.Draw(ref renderContext, null, ref MatrixGizmoTransform, StaticFlags.None, false, 0);
            for (int i = 0; i < _materialAxis.Count; i++)
            {
            }
            DrawCenterPlane(ref renderContext);
        }
        private void DrawScaleGizmo(ref RenderContext renderContext)
        {
            
            _modelPlaneAxisGizmo.Draw(ref renderContext, _materialAxisPlane[0], ref MatrixGizmoTransform, StaticFlags.None, false, 0);
            for (int i = 0; i < _materialAxis.Count; i++)
            {
                _modelPlaneAxisGizmo.LODs[0].Meshes[i].Draw(ref renderContext, _materialAxisPlane[i], ref MatrixGizmoTransform, StaticFlags.None, false, 0);
                _modelScaleAxisGizmo.LODs[0].Meshes[i].Draw(ref renderContext, _materialAxis[i], ref MatrixGizmoTransform, StaticFlags.None, false, 0);
            }
            DrawCenterPlane(ref renderContext);
        }
        private void DrawRotationGizmo(ref RenderContext renderContext)
        {
            for (int i = 0; i < _materialAxis.Count; i++)
            {
                _modelRotationAxisGizmo.LODs[0].Meshes[i].Draw(ref renderContext, _materialAxis[i], ref MatrixGizmoTransform, StaticFlags.None, false, 0);
            }
            _modelSphere.Draw(ref renderContext, _materialRotCenter, ref MatrixGizmoTransform, StaticFlags.None, false, 0);
            DrawCenterPlane(ref renderContext);
        }
        private void DrawCenterPlane(ref RenderContext renderContext)
        {
            _modelPlane.Draw(ref renderContext, _materialCenter, ref MatrixCenter, StaticFlags.None, false, 0);
        }

        /// <summary>
        /// computes matrixes for drawing
        /// </summary>

        internal void ComputeNewMatrixis()
        {
            var Scale = new Float3(_screenScale);
            if (_activeMode == Mode.Rotate)
            {
                var sScale = WorldTransform.Scale * 0.8f;
                _materialCenter.SetParameterValue("Line Thickness", 0.005f, true);
                MatrixGizmoTransform = WorldTransform.GetWorld();
                MatrixCenter = MatrixGizmoTransform;
                MatrixGizmoTransform.ScaleVector *= 0.8f;
                return;
            }
            else
            {
                MatrixGizmoTransform = WorldTransform.GetWorld();
                _materialCenter.SetParameterValue("Line Thickness", 0.05f, true);
            }
            ComputeCenterMatrix(Quaternion.FromDirection(-(WorldTransform.Translation - Owner.ViewPosition).Normalized));
        }

        internal void ComputeCenterMatrix(Quaternion q)
        {
            MatrixCenter = new Transform(WorldTransform.Translation, q, new Float3(_screenScale)).GetWorld();
        }
    }
}
