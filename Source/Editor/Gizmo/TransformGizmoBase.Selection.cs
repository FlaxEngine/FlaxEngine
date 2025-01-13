// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if USE_LARGE_WORLDS
using Real = System.Double;
#else
using Real = System.Single;
#endif

using FlaxEngine;

namespace FlaxEditor.Gizmo
{
    public partial class TransformGizmoBase
    {
        /// <summary>
        /// Gets the selection center point (in world space).
        /// </summary>
        /// <returns>Center point or <see cref="Vector3.Zero"/> if no object selected.</returns>
        public Vector3 GetSelectionCenter()
        {
            int count = SelectionCount;

            // Check if there is no objects selected at all
            if (count == 0)
                return Vector3.Zero;

            // Get center point
            Vector3 center = Vector3.Zero;
            for (int i = 0; i < count; i++)
                center += GetSelectedTransform(i).Translation;

            // Return arithmetic average or whatever it means
            return center / count;
        }

        private bool IntersectsRotateCircle(Vector3 normal, ref Ray ray, out Real distance)
        {
            var plane = new Plane(Vector3.Zero, normal);
            if (!plane.Intersects(ref ray, out distance))
                return false;
            Vector3 hitPoint = ray.Position + ray.Direction * distance;
            Real distanceNormalized = hitPoint.Length / RotateRadiusRaw;
            return Mathf.IsInRange(distanceNormalized, 0.9f, 1.1f);
        }

        private void SelectAxis()
        {
            // Get mouse ray
            Ray ray = Owner.MouseRay;

            // Transform ray into local space of the gizmo
            Ray localRay;
            _gizmoWorld.WorldToLocalVector(ref ray.Direction, out localRay.Direction);
            _gizmoWorld.WorldToLocal(ref ray.Position, out localRay.Position);

            // Find gizmo collisions with mouse
            Real closestIntersection = Real.MaxValue;
            Real intersection;
            _activeAxis = Axis.None;
            switch (_activeMode)
            {
            case Mode.Translate:
            {
                // Axis boxes collision
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

                /*// Center
                if (CenterBoxRaw.Intersects(ref localRay, out intersection) && intersection > closestIntersection)
                {
                    _activeAxis = Axis.Center;
                    closestIntersection = intersection;
                }*/

                break;
            }
            case Mode.Rotate:
            {
                // Circles
                if (IntersectsRotateCircle(Vector3.UnitX, ref localRay, out intersection) && intersection < closestIntersection)
                {
                    _activeAxis = Axis.X;
                    closestIntersection = intersection;
                }
                if (IntersectsRotateCircle(Vector3.UnitY, ref localRay, out intersection) && intersection < closestIntersection)
                {
                    _activeAxis = Axis.Y;
                    closestIntersection = intersection;
                }
                if (IntersectsRotateCircle(Vector3.UnitZ, ref localRay, out intersection) && intersection < closestIntersection)
                {
                    _activeAxis = Axis.Z;
                    closestIntersection = intersection;
                }

                break;
            }
            case Mode.Scale:
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

                // Center
                if (CenterBoxRaw.Intersects(ref localRay, out intersection) && intersection > closestIntersection)
                {
                    _activeAxis = Axis.Center;
                    closestIntersection = intersection;
                }

                break;
            }
            }
        }
    }
}
