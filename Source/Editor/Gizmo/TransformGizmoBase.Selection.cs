// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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
                center += GetSelectedObject(i).Translation;

            // Return arithmetic average or whatever it means
            return center / count;
        }
        
        private bool IntersectsRotateCircle(Vector3 normal, ref Ray ray, out float distance)
        {
            var plane = new Plane(Vector3.Zero, normal);

            if (!plane.Intersects(ref ray, out distance))
                return false;
            Vector3 hitPoint = ray.Position + ray.Direction * distance;

            float distanceNormalized = hitPoint.Length / RotateRadiusRaw;
            return Mathf.IsInRange(distanceNormalized, 0.9f, 1.1f);
        }

        private void SelectAxis()
        {
            // Get mouse ray
            Ray ray = Owner.MouseRay;

            // Transform ray into local space of the gizmo
            Ray localRay;
            Matrix.Invert(ref _gizmoWorld, out Matrix invGizmoWorld);
            Vector3.TransformNormal(ref ray.Direction, ref invGizmoWorld, out localRay.Direction);
            Vector3.Transform(ref ray.Position, ref invGizmoWorld, out localRay.Position);

            // Find gizmo collisions with mouse
            float closestintersection = float.MaxValue;
            float intersection;
            _activeAxis = Axis.None;
            switch (_activeMode)
            {
            case Mode.Translate:
            {
                // Axis boxes collision
                if (XAxisBox.Intersects(ref localRay, out intersection) && intersection < closestintersection)
                {
                    _activeAxis = Axis.X;
                    closestintersection = intersection;
                }

                if (YAxisBox.Intersects(ref localRay, out intersection) && intersection < closestintersection)
                {
                    _activeAxis = Axis.Y;
                    closestintersection = intersection;
                }

                if (ZAxisBox.Intersects(ref localRay, out intersection) && intersection < closestintersection)
                {
                    _activeAxis = Axis.Z;
                    closestintersection = intersection;
                }

                // Quad planes collision
                if (closestintersection >= float.MaxValue)
                    closestintersection = float.MinValue;
                if (XYBox.Intersects(ref localRay, out intersection) && intersection > closestintersection)
                {
                    _activeAxis = Axis.XY;
                    closestintersection = intersection;
                }

                if (XZBox.Intersects(ref localRay, out intersection) && intersection > closestintersection)
                {
                    _activeAxis = Axis.ZX;
                    closestintersection = intersection;
                }
                if (YZBox.Intersects(ref localRay, out intersection) && intersection > closestintersection)
                {
                    _activeAxis = Axis.YZ;
                    closestintersection = intersection;
                }

                break;
            }

            case Mode.Rotate:
            {
                // Circles
                if (IntersectsRotateCircle(Vector3.UnitX, ref localRay, out intersection) && intersection < closestintersection)
                {
                    _activeAxis = Axis.X;
                    closestintersection = intersection;
                }
                if (IntersectsRotateCircle(Vector3.UnitY, ref localRay, out intersection) && intersection < closestintersection)
                {
                    _activeAxis = Axis.Y;
                    closestintersection = intersection;
                }
                if (IntersectsRotateCircle(Vector3.UnitZ, ref localRay, out intersection) && intersection < closestintersection)
                {
                    _activeAxis = Axis.Z;
                    closestintersection = intersection;
                }

                // Center
                /*if (CenterSphere.Intersects(ref ray, out intersection) && intersection < closestintersection)
                {
                    _activeAxis = Axis.Center;
                    closestintersection = intersection;
                }*/

                break;
            }

            case Mode.Scale:
            {
                // Spheres collision
                if (ScaleXSphere.Intersects(ref ray, out intersection) && intersection < closestintersection)
                {
                    _activeAxis = Axis.X;
                    closestintersection = intersection;
                }
                if (ScaleYSphere.Intersects(ref ray, out intersection) && intersection < closestintersection)
                {
                    _activeAxis = Axis.Y;
                    closestintersection = intersection;
                }
                if (ScaleZSphere.Intersects(ref ray, out intersection) && intersection < closestintersection)
                {
                    _activeAxis = Axis.Z;
                    closestintersection = intersection;
                }

                // Center
                if (CenterBox.Intersects(ref ray, out intersection) && intersection < closestintersection)
                {
                    _activeAxis = Axis.Center;
                    closestintersection = intersection;
                }

                break;
            }
            }
        }
    }
}
