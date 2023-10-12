// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#if USE_LARGE_WORLDS
using Real = System.Double;
#else
using Real = System.Single;
#endif

using FlaxEngine;
using System.Collections.Generic;
using FlaxEditor.SceneGraph;

namespace FlaxEditor.Gizmo
{
    public partial class TransformGizmoBase
    {
        /// <summary>
        /// Gets the selection center point (in world space).
        /// </summary>
        /// <returns>Center point or <see cref="F:FlaxEngine.Vector3.Zero" /> if no object selected.</returns>

        public Vector3 GetSelectionCenter()
        {
            int count = SelectionCount;
            bool flag = count == 0;
            Vector3 result;
            if (flag)
            {
                result = Vector3.Zero;
            }
            else
            {
                Vector3 center = Vector3.Zero;
                for (int i = 0; i < count; i++)
                {
                    center += GetSelectedObject(i).Translation;
                }
                result = center / (float)count;
            }
            return result;
        }


        private bool IntersectsRotateCircle(Vector3 normal, float min, float max, ref Ray ray, out float distance)
        {
            Plane plane = new Plane(Vector3.Zero, normal);
            bool flag = !plane.Intersects(ref ray, out distance);
            bool result;
            if (flag)
            {
                result = false;
            }
            else
            {
                float distanceNormalized = (ray.Position + ray.Direction * distance).Length;
                result = Mathf.IsInRange(distanceNormalized, min, max);
            }
            return result;
        }

        private void SelectAxis()
        {
            Ray ray = Owner.MouseRay;

            // Transform ray into local space of the gizmo
            Ray localRay;
            WorldTransform.WorldToLocalVector(ref ray.Direction, out localRay.Direction);
            WorldTransform.WorldToLocal(ref ray.Position, out localRay.Position);

            // Find gizmo collisions with mouse
            Real closestIntersection = Real.MaxValue;
            Real intersection;

            const float RotateCircleAxisMin = 40 - 2;
            const float RotateCircleAxisMax = 40 + 2;

            float CenterRotateCircleMin = 0;
            float CenterRotateCircleMax = 10;

            _activeAxis = Axis.None;
            if (_activeMode == Mode.Rotate)
            {
                CenterRotateCircleMin = RotateCircleAxisMax;
                CenterRotateCircleMax = 50;
                // Circles
                if (IntersectsRotateCircle(Vector3.UnitX, RotateCircleAxisMin, RotateCircleAxisMax, ref localRay, out intersection) && intersection < closestIntersection)
                {
                    _activeAxis = Axis.X;
                    closestIntersection = intersection;
                }
                if (IntersectsRotateCircle(Vector3.UnitY, RotateCircleAxisMin, RotateCircleAxisMax, ref localRay, out intersection) && intersection < closestIntersection)
                {
                    _activeAxis = Axis.Y;
                    closestIntersection = intersection;
                }
                if (IntersectsRotateCircle(Vector3.UnitZ, RotateCircleAxisMin, RotateCircleAxisMax, ref localRay, out intersection) && intersection < closestIntersection)
                {
                    _activeAxis = Axis.Z;
                    closestIntersection = intersection;
                }
                if (IntersectsRotateCircle(MatrixCenter.Forward, 0, CenterRotateCircleMin, ref localRay, out intersection) && intersection < closestIntersection)
                {
                    _activeAxis = Axis.Center;
                }
                else if (IntersectsRotateCircle(MatrixCenter.Forward, CenterRotateCircleMin, CenterRotateCircleMax, ref localRay, out intersection) && intersection < closestIntersection)
                    {
                        _activeAxis = Axis.View;
                    closestIntersection = intersection;
                }
            }
            else
            {

                // Boxes collision
                if (XAxisBox.Intersects(ref localRay, out intersection) && intersection < closestIntersection)
                {
                    _activeAxis = Axis.X;
                    closestIntersection = intersection;
                }
                if (YAxisBox.Intersects(ref localRay, out intersection) && intersection < closestIntersection)
                {
                    _activeAxis = Axis.Y;
                    closestIntersection = intersection;
                }
                if (ZAxisBox.Intersects(ref localRay, out intersection) && intersection < closestIntersection)
                {
                    _activeAxis = Axis.Z;
                    closestIntersection = intersection;
                }

                // Quad planes collision
                if (closestIntersection >= float.MaxValue)
                    closestIntersection = float.MinValue;

                if (XYBox.Intersects(ref localRay, out intersection) && intersection > closestIntersection)
                {
                    _activeAxis = Axis.XY;
                    closestIntersection = intersection;
                }
                if (XZBox.Intersects(ref localRay, out intersection) && intersection > closestIntersection)
                {
                    _activeAxis = Axis.ZX;
                    closestIntersection = intersection;
                }
                if (YZBox.Intersects(ref localRay, out intersection) && intersection > closestIntersection)
                {
                    _activeAxis = Axis.YZ;
                    closestIntersection = intersection;
                }
                if (IntersectsRotateCircle(MatrixGizmoTransform.Forward, CenterRotateCircleMin, CenterRotateCircleMax, ref localRay, out intersection) && intersection < closestIntersection)
                {
                    _activeAxis = Axis.View;
                }
            }
        }
        /// <inheritdoc/>>
        public override void OnSelectionChanged(List<SceneGraphNode> newSelection)
        {
            Debug.Log("NewSelection");
            base.OnSelectionChanged(newSelection);
        }
    }
}
