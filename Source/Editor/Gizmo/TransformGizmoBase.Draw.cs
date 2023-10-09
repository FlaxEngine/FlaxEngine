// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.Gizmo
{
    public partial class TransformGizmoBase
    {

        internal Transform X;
        internal Transform Y;
        internal Transform Z;
        internal Transform PlaneX;
        internal Transform PlaneZ;
        internal Transform PlaneY;
        internal Transform Center;
        internal Transform RotCenter;

        private static readonly Quaternion PlaneXOrientationOffset = Quaternion.Euler(0f, 90f, 0f);
        private static readonly Quaternion PlaneYOrientationOffset = Quaternion.Euler(0f, 0f, -90f);
        private static readonly Quaternion PlaneZOrientationOffset = Quaternion.Euler(-90f, 0f, 0f);
        private static readonly Quaternion XOrientationOffset = Quaternion.Euler(0f, -90f, 0f);
        private static readonly Quaternion YOrientationOffset = Quaternion.Euler(90f, 0f, 0f);
        private static readonly Quaternion ZOrientationOffset = Quaternion.Euler(0f, 180f, 0f);
        private static readonly Quaternion RotXOrientationOffset = Quaternion.Euler(90f, -90f, 0f);
        private static readonly Quaternion RotZOrientationOffset = Quaternion.Euler(90f, 180f, 0f);

        /// <inheritdoc />
        public override void Draw(ref RenderContext renderContext)
        {
            if (IsActive && SelectionCount != 0)
            {
                if (_isTransforming)
                {
                    Vector3 dir = Vector3.Zero;
                    switch (_activeAxis)
                    {
                        case Axis.X:
                            dir = WorldTransform.Right;
                            DebugDraw.DrawLine(Vector3.Right * WorldTransform.Orientation * 100000f + WorldTransform.Translation, Vector3.Left * WorldTransform.Orientation * 100000f + WorldTransform.Translation, Resources.XAxisColor, 0f, true);
                            break;
                        case Axis.Y:
                            dir = WorldTransform.Up;
                            DebugDraw.DrawLine(Vector3.Up * WorldTransform.Orientation * 100000f + WorldTransform.Translation, Vector3.Down * WorldTransform.Orientation * 100000f + WorldTransform.Translation, Resources.YAxisColor, 0f, true);
                            break;
                        case Axis.XY:
                            DebugDraw.DrawLine(Vector3.Right * WorldTransform.Orientation * 100000f + WorldTransform.Translation, Vector3.Left * WorldTransform.Orientation * 100000f + WorldTransform.Translation, Resources.XAxisColor, 0f, true);
                            DebugDraw.DrawLine(Vector3.Up * WorldTransform.Orientation * 100000f + WorldTransform.Translation, Vector3.Down * WorldTransform.Orientation * 100000f + WorldTransform.Translation, Resources.YAxisColor, 0f, true);
                            break;
                        case Axis.Z:
                            dir = WorldTransform.Forward;
                            DebugDraw.DrawLine(Vector3.Forward * WorldTransform.Orientation * 100000f + WorldTransform.Translation, Vector3.Backward * WorldTransform.Orientation * 100000f + WorldTransform.Translation, Resources.ZAxisColor, 0f, true);
                            break;
                        case Axis.ZX:
                            DebugDraw.DrawLine(Vector3.Right * WorldTransform.Orientation * 100000f + WorldTransform.Translation, Vector3.Left * WorldTransform.Orientation * 100000f + WorldTransform.Translation, Resources.XAxisColor, 0f, true);
                            DebugDraw.DrawLine(Vector3.Forward * WorldTransform.Orientation * 100000f + WorldTransform.Translation, Vector3.Backward * WorldTransform.Orientation * 100000f + WorldTransform.Translation, Resources.ZAxisColor, 0f, true);
                            break;
                        case Axis.YZ:
                            DebugDraw.DrawLine(Vector3.Forward * WorldTransform.Orientation * 100000f + WorldTransform.Translation, Vector3.Backward * WorldTransform.Orientation * 100000f + WorldTransform.Translation, Resources.YAxisColor, 0f, true);
                            DebugDraw.DrawLine(Vector3.Up * WorldTransform.Orientation * 100000f + WorldTransform.Translation, Vector3.Down * WorldTransform.Orientation * 100000f + WorldTransform.Translation, Resources.ZAxisColor, 0f, true);
                            break;
                    }
                    if (_activeMode == Mode.Rotate)
                    {
                        if (!dir.IsZero)
                        {
                            Center.Orientation = Quaternion.FromDirection(dir);
                        }
                        resources._materialCenter.SetParameterValue("RotacionStartEnd", new Float2(Vector3.Angle(StartWorldTransform.Up, dir), EndRot), true);
                        Matrix mCenter = Center.GetWorld();
                        Resources._modelPlane.Draw(ref renderContext, resources._materialCenter, ref mCenter, StaticFlags.None, false, 0);
                    }
                }
                else
                {
                    ComputeNewTransform(_activeMode, _activeTransformSpace);
                    ComputeCenterRotacion(base.Owner);
                    switch (_activeMode)
                    {
                        case Mode.Translate:
                            DrawTSHandle(Resources._modelTranslationAxis, ref renderContext);
                            resources._materialAxisX.SetParameterValue("IsSelected", _activeAxis.HasFlag(Axis.X), true);
                            resources._materialAxisY.SetParameterValue("IsSelected", _activeAxis.HasFlag(Axis.Y), true);
                            resources._materialAxisZ.SetParameterValue("IsSelected", _activeAxis.HasFlag(Axis.Z), true);
                            resources._materialAxisPlaneX.SetParameterValue("IsSelected", _activeAxis.HasFlag(Axis.YZ), true);
                            resources._materialAxisPlaneY.SetParameterValue("IsSelected", _activeAxis.HasFlag(Axis.ZX), true);
                            resources._materialAxisPlaneZ.SetParameterValue("IsSelected", _activeAxis.HasFlag(Axis.XY), true);
                            resources._materialCenter.SetParameterValue("IsSelected", _activeAxis.HasFlag(Axis.View), true);
                            resources._materialCenter.SetParameterValue("SnapingValue", 0, true);
                            break;
                        case Mode.Rotate:
                            DrawRHandle(ref renderContext);
                            resources._materialAxisX.SetParameterValue("IsSelected", _activeAxis.HasFlag(Axis.X), true);
                            resources._materialAxisY.SetParameterValue("IsSelected", _activeAxis.HasFlag(Axis.Y), true);
                            resources._materialAxisZ.SetParameterValue("IsSelected", _activeAxis.HasFlag(Axis.Z), true);
                            resources._materialCenter.SetParameterValue("IsSelected", _activeAxis.HasFlag(Axis.View), true);
                            resources._materialCenter.SetParameterValue("SnapingValue", RotationSnapEnabled ? RotationSnapValue : 0f, true);
                            resources._materialRotCenter.SetParameterValue("IsSelected", _activeAxis.HasFlag(Axis.Center), true);
                            break;
                        case Mode.Scale:
                            DrawTSHandle(Resources._modelScaleAxis, ref renderContext);
                            resources._materialAxisX.SetParameterValue("IsSelected", _activeAxis.HasFlag(Axis.X), true);
                            resources._materialAxisY.SetParameterValue("IsSelected", _activeAxis.HasFlag(Axis.Y), true);
                            resources._materialAxisZ.SetParameterValue("IsSelected", _activeAxis.HasFlag(Axis.Z), true);
                            resources._materialAxisPlaneX.SetParameterValue("IsSelected", _activeAxis.HasFlag(Axis.YZ), true);
                            resources._materialAxisPlaneY.SetParameterValue("IsSelected", _activeAxis.HasFlag(Axis.ZX), true);
                            resources._materialAxisPlaneZ.SetParameterValue("IsSelected", _activeAxis.HasFlag(Axis.XY), true);
                            resources._materialCenter.SetParameterValue("IsSelected", _activeAxis.HasFlag(Axis.View), true);
                            resources._materialCenter.SetParameterValue("SnapingValue", 0, true);
                            break;
                    }
                }
            }
        }


        private void DrawTSHandle(Model AxisModel, ref RenderContext renderContext)
        {
            Matrix mY = Y.GetWorld();
            Matrix mX = X.GetWorld();
            Matrix mZ = Z.GetWorld();
            Matrix mPlaneX = PlaneX.GetWorld();
            Matrix mPlaneY = PlaneY.GetWorld();
            Matrix mPlaneZ = PlaneZ.GetWorld();
            Matrix mCenter = Center.GetWorld();
            AxisModel.Draw(ref renderContext, resources._materialAxisX, ref mX, StaticFlags.None, false, 0);
            AxisModel.Draw(ref renderContext, resources._materialAxisY, ref mY, StaticFlags.None, false, 0);
            AxisModel.Draw(ref renderContext, resources._materialAxisZ, ref mZ, StaticFlags.None, false, 0);
            Resources._modelPlane.Draw(ref renderContext, resources._materialAxisPlaneX, ref mPlaneX, StaticFlags.None, false, 0);
            Resources._modelPlane.Draw(ref renderContext, resources._materialAxisPlaneY, ref mPlaneY, StaticFlags.None, false, 0);
            Resources._modelPlane.Draw(ref renderContext, resources._materialAxisPlaneZ, ref mPlaneZ, StaticFlags.None, false, 0);
            Resources._modelPlane.Draw(ref renderContext, resources._materialCenter, ref mCenter, StaticFlags.None, false, 0);
        }

        private void DrawRHandle(ref RenderContext renderContext)
        {
            Matrix mY = Y.GetWorld();
            Matrix mX = X.GetWorld();
            Matrix mZ = Z.GetWorld();
            Matrix mCenter = Center.GetWorld();
            Matrix mRotCenter = RotCenter.GetWorld();
            Resources._modelRotationAxis.Draw(ref renderContext, resources._materialAxisX, ref mX, StaticFlags.None, false, 0);
            Resources._modelRotationAxis.Draw(ref renderContext, resources._materialAxisY, ref mY, StaticFlags.None, false, 0);
            Resources._modelRotationAxis.Draw(ref renderContext, resources._materialAxisZ, ref mZ, StaticFlags.None, false, 0);
            Resources._modelPlane.Draw(ref renderContext, resources._materialCenter, ref mCenter, StaticFlags.None, false, 0);
            Resources._modelSphere.Draw(ref renderContext, resources._materialRotCenter, ref mRotCenter, StaticFlags.None, false, 0);
        }

        /// <summary>
        /// computes all component of the transform
        /// </summary>

        internal void ComputeNewTransform(in Mode curentMode,in TransformSpace curentTransformSpace)
        {
            Quaternion WorldTransformOrientationI = WorldTransform.Orientation;
            if (curentMode == Mode.Rotate)
            {
                resources._materialCenter.SetParameterValue("Line Thickness", 0.005f, true);
                Float3 Scale = WorldTransform.Scale * 0.8f;
                X = new Transform(WorldTransform.Translation, WorldTransformOrientationI * RotXOrientationOffset, Scale);
                Y = new Transform(WorldTransform.Translation, WorldTransformOrientationI, Scale);
                Z = new Transform(WorldTransform.Translation, WorldTransformOrientationI * RotZOrientationOffset, Scale);
                RotCenter = new Transform(WorldTransform.Translation, Quaternion.Identity, Scale);
                Center.Scale = WorldTransform.Scale;
                Center.Translation = WorldTransform.Translation;
                return;
            }
            else
            {
                resources._materialCenter.SetParameterValue("Line Thickness", 0.05f, true);
                Quaternion OrientationXAxis = XOrientationOffset;
                Quaternion OrientationYAxis = YOrientationOffset;
                Quaternion OrientationZAxis = ZOrientationOffset;
                Float3 PlaneScale = WorldTransform.Scale * 0.1f;
                if (curentTransformSpace == TransformSpace.Local)
                {
                    PlaneX = new Transform(new Vector3(0f, 20f, 20f) * WorldTransformOrientationI * WorldTransform.Scale + WorldTransform.Translation, WorldTransformOrientationI * PlaneXOrientationOffset, PlaneScale);
                    PlaneZ = new Transform(new Vector3(20f, 20f, 0f) * WorldTransformOrientationI * WorldTransform.Scale + WorldTransform.Translation, WorldTransformOrientationI * PlaneYOrientationOffset, PlaneScale);
                    PlaneY = new Transform(new Vector3(20f, 0f, 20f) * WorldTransformOrientationI * WorldTransform.Scale + WorldTransform.Translation, WorldTransformOrientationI * PlaneZOrientationOffset, PlaneScale);
                    OrientationXAxis = WorldTransformOrientationI * OrientationXAxis;
                    OrientationYAxis = WorldTransformOrientationI * OrientationYAxis;
                    OrientationZAxis = WorldTransformOrientationI * OrientationZAxis;
                }
                else
                {
                    PlaneX = new Transform(new Vector3(0f, 20f, 20f) * WorldTransform.Scale + WorldTransform.Translation, WorldTransformOrientationI * PlaneXOrientationOffset, PlaneScale);
                    PlaneZ = new Transform(new Vector3(20f, 20f, 0f) * WorldTransform.Scale + WorldTransform.Translation, WorldTransformOrientationI * PlaneYOrientationOffset, PlaneScale);
                    PlaneY = new Transform(new Vector3(20f, 0f, 20f) * WorldTransform.Scale + WorldTransform.Translation, WorldTransformOrientationI * PlaneZOrientationOffset, PlaneScale);
                }
                Vector3 TranslationXAxis = new Vector3(WorldTransform.Scale.X * 0.1f, 0f, 0f) * OrientationXAxis;
                Vector3 TranslationYAxis = new Vector3(WorldTransform.Scale.X * 0.1f, 0f, 0f) * OrientationYAxis;
                Vector3 TranslationZAxis = new Vector3(WorldTransform.Scale.X * 0.1f, 0f, 0f) * OrientationZAxis;
                X = new Transform(WorldTransform.Translation + TranslationXAxis, OrientationXAxis, WorldTransform.Scale);
                Y = new Transform(WorldTransform.Translation + TranslationYAxis, OrientationYAxis, WorldTransform.Scale);
                Z = new Transform(WorldTransform.Translation + TranslationZAxis, OrientationZAxis, WorldTransform.Scale);
                Center.Scale = PlaneScale;
                Center.Translation = WorldTransform.Translation;
            }
        }

        /// <summary>
        /// computes only scale and need Translation of transform
        /// </summary>

        internal void ComputeNewTransformScale(in Mode curentMode,in TransformSpace curentTransformSpace)
        {
            if (curentMode == Mode.Rotate)
            {
                Float3 Scale = WorldTransform.Scale * 0.8f;
                X.Scale = Scale;
                Y.Scale = Scale;
                Z.Scale = Scale;
                Center.Scale = WorldTransform.Scale;
                RotCenter.Scale = Scale;
                return;
            }
            else
            {
                Quaternion OrientationXAxis = XOrientationOffset;
                Quaternion OrientationYAxis = YOrientationOffset;
                Quaternion OrientationZAxis = ZOrientationOffset;
                bool flag2 = curentTransformSpace == TransformSpace.Local;
                if (flag2)
                {
                    OrientationXAxis *= WorldTransform.Orientation;
                    OrientationYAxis *= WorldTransform.Orientation;
                    OrientationZAxis *= WorldTransform.Orientation;
                    PlaneX.Translation = new Vector3(0f, 20f, 20f) * WorldTransform.Scale * WorldTransform.Orientation + WorldTransform.Translation;
                    PlaneZ.Translation = new Vector3(20f, 20f, 0f) * WorldTransform.Scale * WorldTransform.Orientation + WorldTransform.Translation;
                    PlaneY.Translation = new Vector3(20f, 0f, 20f) * WorldTransform.Scale * WorldTransform.Orientation + WorldTransform.Translation;
                }
                else
                {
                    PlaneX.Translation = new Vector3(0f, 20f, 20f) * WorldTransform.Scale + WorldTransform.Translation;
                    PlaneZ.Translation = new Vector3(20f, 20f, 0f) * WorldTransform.Scale + WorldTransform.Translation;
                    PlaneY.Translation = new Vector3(20f, 0f, 20f) * WorldTransform.Scale + WorldTransform.Translation;
                }
                Vector3 TranslationXAxis = new Vector3(WorldTransform.Scale.X * 0.1f, 0f, 0f) * OrientationXAxis;
                Vector3 TranslationYAxis = new Vector3(WorldTransform.Scale.X * 0.1f, 0f, 0f) * OrientationYAxis;
                Vector3 TranslationZAxis = new Vector3(WorldTransform.Scale.X * 0.1f, 0f, 0f) * OrientationZAxis;
                Float3 PlaneScale = WorldTransform.Scale * 0.1f;
                X.Translation = WorldTransform.Translation + TranslationXAxis;
                Y.Translation = WorldTransform.Translation + TranslationYAxis;
                Z.Translation = WorldTransform.Translation + TranslationZAxis;
                X.Scale = WorldTransform.Scale;
                Y.Scale = WorldTransform.Scale;
                Z.Scale = WorldTransform.Scale;
                PlaneX.Scale = PlaneScale;
                PlaneZ.Scale = PlaneScale;
                PlaneY.Scale = PlaneScale;
                Center.Scale = PlaneScale;
            }
        }

        internal void ComputeCenterRotacion(IGizmoOwner owner)
        {
            Center.Orientation = Quaternion.FromDirection(-(WorldTransform.Translation - owner.ViewPosition).Normalized);
        }
    }
}
