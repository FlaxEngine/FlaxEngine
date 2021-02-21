// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

// -----------------------------------------------------------------------------
// Original code from SharpDX project. https://github.com/sharpdx/SharpDX/
// Greetings to Alexandre Mutel. Original code published with the following license:
// -----------------------------------------------------------------------------
// Copyright (c) 2010-2014 SharpDX - Alexandre Mutel
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace FlaxEngine
{
    /// <summary>
    /// Defines a frustum which can be used in frustum culling, zoom to Extents (zoom to fit) operations,
    /// (matrix, frustum, camera) interchange, and many kind of intersection testing.
    /// </summary>
    [Serializable]
    [StructLayout(LayoutKind.Sequential, Pack = 4)]
    public struct BoundingFrustum : IEquatable<BoundingFrustum>
    {
        private Matrix pMatrix;
        private Plane pNear;
        private Plane pFar;
        private Plane pLeft;
        private Plane pRight;
        private Plane pTop;
        private Plane pBottom;

        /// <summary>
        /// Gets or sets the Matrix that describes this bounding frustum.
        /// </summary>
        public Matrix Matrix
        {
            get => pMatrix;
            set
            {
                pMatrix = value;
                GetPlanesFromMatrix(ref pMatrix, out pNear, out pFar, out pLeft, out pRight, out pTop, out pBottom);
            }
        }

        /// <summary>
        /// Gets the near plane of the BoundingFrustum.
        /// </summary>
        public Plane Near => pNear;

        /// <summary>
        /// Gets the far plane of the BoundingFrustum.
        /// </summary>
        public Plane Far => pFar;

        /// <summary>
        /// Gets the left plane of the BoundingFrustum.
        /// </summary>
        public Plane Left => pLeft;

        /// <summary>
        /// Gets the right plane of the BoundingFrustum.
        /// </summary>
        public Plane Right => pRight;

        /// <summary>
        /// Gets the top plane of the BoundingFrustum.
        /// </summary>
        public Plane Top => pTop;

        /// <summary>
        /// Gets the bottom plane of the BoundingFrustum.
        /// </summary>
        public Plane Bottom => pBottom;

        /// <summary>
        /// Creates a new instance of BoundingFrustum.
        /// </summary>
        /// <param name="matrix">Combined matrix that usually takes view × projection matrix.</param>
        public BoundingFrustum(Matrix matrix)
        {
            pMatrix = matrix;
            GetPlanesFromMatrix(ref pMatrix, out pNear, out pFar, out pLeft, out pRight, out pTop, out pBottom);
        }

        /// <summary>
        /// Returns a hash code for this instance.
        /// </summary>
        /// <returns>
        /// A hash code for this instance, suitable for use in hashing algorithms and data structures like a hash table. 
        /// </returns>
        public override int GetHashCode()
        {
            return pMatrix.GetHashCode();
        }

        /// <summary>
        /// Determines whether the specified <see cref="BoundingFrustum" /> is equal to this instance.
        /// </summary>
        /// <param name="other">The <see cref="BoundingFrustum" /> to compare with this instance.</param>
        /// <returns>
        /// <c>true</c> if the specified <see cref="BoundingFrustum" /> is equal to this instance; otherwise, <c>false</c>.
        /// </returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(ref BoundingFrustum other)
        {
            return pMatrix == other.pMatrix;
        }

        /// <summary>
        /// Determines whether the specified <see cref="BoundingFrustum" /> is equal to this instance.
        /// </summary>
        /// <param name="other">The <see cref="BoundingFrustum" /> to compare with this instance.</param>
        /// <returns>
        /// <c>true</c> if the specified <see cref="BoundingFrustum" /> is equal to this instance; otherwise, <c>false</c>.
        /// </returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(BoundingFrustum other)
        {
            return Equals(ref other);
        }

        /// <summary>
        /// Determines whether the specified <see cref="System.Object" /> is equal to this instance.
        /// </summary>
        /// <param name="obj">The <see cref="System.Object" /> to compare with this instance.</param>
        /// <returns>
        /// <c>true</c> if the specified <see cref="System.Object" /> is equal to this instance; otherwise, <c>false</c>.
        /// </returns>
        public override bool Equals(object obj)
        {
            if (!(obj is BoundingFrustum))
                return false;

            var strongValue = (BoundingFrustum)obj;
            return Equals(ref strongValue);
        }

        /// <summary>
        /// Implements the operator ==.
        /// </summary>
        /// <param name="left">The left.</param>
        /// <param name="right">The right.</param>
        /// <returns>
        /// The result of the operator.
        /// </returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator ==(BoundingFrustum left, BoundingFrustum right)
        {
            return left.Equals(ref right);
        }

        /// <summary>
        /// Implements the operator !=.
        /// </summary>
        /// <param name="left">The left.</param>
        /// <param name="right">The right.</param>
        /// <returns>
        /// The result of the operator.
        /// </returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator !=(BoundingFrustum left, BoundingFrustum right)
        {
            return !left.Equals(ref right);
        }

        /// <summary>
        /// Returns one of the 6 planes related to this frustum.
        /// </summary>
        /// <param name="index">Plane index where 0 fro Left, 1 for Right, 2 for Top, 3 for Bottom, 4 for Near, 5 for Far</param>
        /// <returns></returns>
        public Plane GetPlane(int index)
        {
            switch (index)
            {
            case 0:
                return pLeft;
            case 1:
                return pRight;
            case 2:
                return pTop;
            case 3:
                return pBottom;
            case 4:
                return pNear;
            case 5:
                return pFar;
            default:
                return new Plane();
            }
        }

        private static void GetPlanesFromMatrix(ref Matrix matrix, out Plane near, out Plane far, out Plane left, out Plane right, out Plane top, out Plane bottom)
        {
            //http://www.chadvernon.com/blog/resources/directx9/frustum-culling/

            // Left plane
            left.Normal.X = matrix.M14 + matrix.M11;
            left.Normal.Y = matrix.M24 + matrix.M21;
            left.Normal.Z = matrix.M34 + matrix.M31;
            left.D = matrix.M44 + matrix.M41;
            left.Normalize();

            // Right plane
            right.Normal.X = matrix.M14 - matrix.M11;
            right.Normal.Y = matrix.M24 - matrix.M21;
            right.Normal.Z = matrix.M34 - matrix.M31;
            right.D = matrix.M44 - matrix.M41;
            right.Normalize();

            // Top plane
            top.Normal.X = matrix.M14 - matrix.M12;
            top.Normal.Y = matrix.M24 - matrix.M22;
            top.Normal.Z = matrix.M34 - matrix.M32;
            top.D = matrix.M44 - matrix.M42;
            top.Normalize();

            // Bottom plane
            bottom.Normal.X = matrix.M14 + matrix.M12;
            bottom.Normal.Y = matrix.M24 + matrix.M22;
            bottom.Normal.Z = matrix.M34 + matrix.M32;
            bottom.D = matrix.M44 + matrix.M42;
            bottom.Normalize();

            // Near plane
            near.Normal.X = matrix.M13;
            near.Normal.Y = matrix.M23;
            near.Normal.Z = matrix.M33;
            near.D = matrix.M43;
            near.Normalize();

            // Far plane
            far.Normal.X = matrix.M14 - matrix.M13;
            far.Normal.Y = matrix.M24 - matrix.M23;
            far.Normal.Z = matrix.M34 - matrix.M33;
            far.D = matrix.M44 - matrix.M43;
            far.Normalize();
        }

        private static Vector3 Get3PlanesInterPoint(ref Plane p1, ref Plane p2, ref Plane p3)
        {
            //P = -d1 * N2xN3 / N1.N2xN3 - d2 * N3xN1 / N2.N3xN1 - d3 * N1xN2 / N3.N1xN2 
            Vector3 v =
            -p1.D * Vector3.Cross(p2.Normal, p3.Normal) / Vector3.Dot(p1.Normal, Vector3.Cross(p2.Normal, p3.Normal))
            - p2.D * Vector3.Cross(p3.Normal, p1.Normal) / Vector3.Dot(p2.Normal, Vector3.Cross(p3.Normal, p1.Normal))
            - p3.D * Vector3.Cross(p1.Normal, p2.Normal) / Vector3.Dot(p3.Normal, Vector3.Cross(p1.Normal, p2.Normal));

            return v;
        }

        /// <summary>
        /// Creates a new frustum relaying on perspective camera parameters
        /// </summary>
        /// <param name="cameraPos">The camera pos.</param>
        /// <param name="lookDir">The look dir.</param>
        /// <param name="upDir">Up dir.</param>
        /// <param name="fov">The fov.</param>
        /// <param name="znear">The znear.</param>
        /// <param name="zfar">The zfar.</param>
        /// <param name="aspect">The aspect.</param>
        /// <returns>The bounding frustum calculated from perspective camera</returns>
        public static BoundingFrustum FromCamera(Vector3 cameraPos, Vector3 lookDir, Vector3 upDir, float fov, float znear, float zfar, float aspect)
        {
            //http://knol.google.com/k/view-frustum

            lookDir = Vector3.Normalize(lookDir);
            upDir = Vector3.Normalize(upDir);

            Vector3 nearCenter = cameraPos + lookDir * znear;
            Vector3 farCenter = cameraPos + lookDir * zfar;
            var nearHalfHeight = (float)(znear * Math.Tan(fov / 2f));
            var farHalfHeight = (float)(zfar * Math.Tan(fov / 2f));
            float nearHalfWidth = nearHalfHeight * aspect;
            float farHalfWidth = farHalfHeight * aspect;

            Vector3 rightDir = Vector3.Normalize(Vector3.Cross(upDir, lookDir));
            Vector3 near1 = nearCenter - nearHalfHeight * upDir + nearHalfWidth * rightDir;
            Vector3 near2 = nearCenter + nearHalfHeight * upDir + nearHalfWidth * rightDir;
            Vector3 near3 = nearCenter + nearHalfHeight * upDir - nearHalfWidth * rightDir;
            Vector3 near4 = nearCenter - nearHalfHeight * upDir - nearHalfWidth * rightDir;
            Vector3 far1 = farCenter - farHalfHeight * upDir + farHalfWidth * rightDir;
            Vector3 far2 = farCenter + farHalfHeight * upDir + farHalfWidth * rightDir;
            Vector3 far3 = farCenter + farHalfHeight * upDir - farHalfWidth * rightDir;
            Vector3 far4 = farCenter - farHalfHeight * upDir - farHalfWidth * rightDir;

            var result = new BoundingFrustum
            {
                pNear = new Plane(near1, near2, near3),
                pFar = new Plane(far3, far2, far1),
                pLeft = new Plane(near4, near3, far3),
                pRight = new Plane(far1, far2, near2),
                pTop = new Plane(near2, far2, far3),
                pBottom = new Plane(far4, far1, near1)
            };

            result.pNear.Normalize();
            result.pFar.Normalize();
            result.pLeft.Normalize();
            result.pRight.Normalize();
            result.pTop.Normalize();
            result.pBottom.Normalize();

            result.pMatrix = Matrix.LookAt(cameraPos, cameraPos + lookDir * 10, upDir) * Matrix.PerspectiveFov(fov, aspect, znear, zfar);

            return result;
        }

        /// <summary>
        /// Returns the 8 corners of the frustum, element0 is Near1 (near right down corner)
        /// , element1 is Near2 (near right top corner)
        /// , element2 is Near3 (near Left top corner)
        /// , element3 is Near4 (near Left down corner)
        /// , element4 is Far1 (far right down corner)
        /// , element5 is Far2 (far right top corner)
        /// , element6 is Far3 (far left top corner)
        /// , element7 is Far4 (far left down corner)
        /// </summary>
        /// <returns>The 8 corners of the frustum</returns>
        public Vector3[] GetCorners()
        {
            var corners = new Vector3[8];
            GetCorners(corners);
            return corners;
        }

        /// <summary>
        /// Returns the 8 corners of the frustum, element0 is Near1 (near right down corner)
        /// , element1 is Near2 (near right top corner)
        /// , element2 is Near3 (near Left top corner)
        /// , element3 is Near4 (near Left down corner)
        /// , element4 is Far1 (far right down corner)
        /// , element5 is Far2 (far right top corner)
        /// , element6 is Far3 (far left top corner)
        /// , element7 is Far4 (far left down corner)
        /// </summary>
        /// <returns>The 8 corners of the frustum</returns>
        public void GetCorners(Vector3[] corners)
        {
            corners[0] = Get3PlanesInterPoint(ref pNear, ref pBottom, ref pRight); //Near1
            corners[1] = Get3PlanesInterPoint(ref pNear, ref pTop, ref pRight); //Near2
            corners[2] = Get3PlanesInterPoint(ref pNear, ref pTop, ref pLeft); //Near3
            corners[3] = Get3PlanesInterPoint(ref pNear, ref pBottom, ref pLeft); //Near3
            corners[4] = Get3PlanesInterPoint(ref pFar, ref pBottom, ref pRight); //Far1
            corners[5] = Get3PlanesInterPoint(ref pFar, ref pTop, ref pRight); //Far2
            corners[6] = Get3PlanesInterPoint(ref pFar, ref pTop, ref pLeft); //Far3
            corners[7] = Get3PlanesInterPoint(ref pFar, ref pBottom, ref pLeft); //Far3
        }

        /// <summary>
        /// Checks whether a point lay inside, intersects or lay outside the frustum.
        /// </summary>
        /// <param name="point">The point.</param>
        /// <returns>Type of the containment</returns>
        public ContainmentType Contains(ref Vector3 point)
        {
            var result = PlaneIntersectionType.Front;
            var planeResult = PlaneIntersectionType.Front;
            for (var i = 0; i < 6; i++)
            {
                switch (i)
                {
                case 0:
                    planeResult = pNear.Intersects(ref point);
                    break;
                case 1:
                    planeResult = pFar.Intersects(ref point);
                    break;
                case 2:
                    planeResult = pLeft.Intersects(ref point);
                    break;
                case 3:
                    planeResult = pRight.Intersects(ref point);
                    break;
                case 4:
                    planeResult = pTop.Intersects(ref point);
                    break;
                case 5:
                    planeResult = pBottom.Intersects(ref point);
                    break;
                }
                switch (planeResult)
                {
                case PlaneIntersectionType.Back:
                    return ContainmentType.Disjoint;
                case PlaneIntersectionType.Intersecting:
                    result = PlaneIntersectionType.Intersecting;
                    break;
                }
            }
            switch (result)
            {
            case PlaneIntersectionType.Intersecting:
                return ContainmentType.Intersects;
            default:
                return ContainmentType.Contains;
            }
        }

        /// <summary>
        /// Checks whether a point lay inside, intersects or lay outside the frustum.
        /// </summary>
        /// <param name="point">The point.</param>
        /// <returns>Type of the containment</returns>
        public ContainmentType Contains(Vector3 point)
        {
            return Contains(ref point);
        }

        /// <summary>
        /// Checks whether a group of points lay totally inside the frustum (Contains), or lay partially inside the frustum
        /// (Intersects), or lay outside the frustum (Disjoint).
        /// </summary>
        /// <param name="points">The points.</param>
        /// <returns>Type of the containment</returns>
        public ContainmentType Contains(Vector3[] points)
        {
            throw new NotImplementedException();
            /* TODO: (PMin) This method is wrong, does not calculate case where only plane from points is intersected
            var containsAny = false;
            var containsAll = true;
            for (int i = 0; i < points.Length; i++)
            {
                switch (Contains(ref points[i]))
                {
                    case ContainmentType.Contains:
                    case ContainmentType.Intersects:
                        containsAny = true;
                        break;
                    case ContainmentType.Disjoint:
                        containsAll = false;
                        break;
                }
            }
            if (containsAny)
            {
                if (containsAll)
                    return ContainmentType.Contains;
                else
                    return ContainmentType.Intersects;
            }
            else
                return ContainmentType.Disjoint;  */
        }

        /// <summary>
        /// Checks whether a group of points lay totally inside the frustum (Contains), or lay partially inside the frustum
        /// (Intersects), or lay outside the frustum (Disjoint).
        /// </summary>
        /// <param name="points">The points.</param>
        /// <param name="result">Type of the containment.</param>
        public void Contains(Vector3[] points, out ContainmentType result)
        {
            result = Contains(points);
        }

        private void GetBoxToPlanePVertexNVertex(ref BoundingBox box, ref Vector3 planeNormal, out Vector3 p, out Vector3 n)
        {
            p = box.Minimum;
            if (planeNormal.X >= 0)
                p.X = box.Maximum.X;
            if (planeNormal.Y >= 0)
                p.Y = box.Maximum.Y;
            if (planeNormal.Z >= 0)
                p.Z = box.Maximum.Z;

            n = box.Maximum;
            if (planeNormal.X >= 0)
                n.X = box.Minimum.X;
            if (planeNormal.Y >= 0)
                n.Y = box.Minimum.Y;
            if (planeNormal.Z >= 0)
                n.Z = box.Minimum.Z;
        }

        /// <summary>
        /// Determines the intersection relationship between the frustum and a bounding box.
        /// </summary>
        /// <param name="box">The box.</param>
        /// <returns>Type of the containment</returns>
        public ContainmentType Contains(ref BoundingBox box)
        {
            var result = ContainmentType.Contains;
            for (var i = 0; i < 6; i++)
            {
                var plane = GetPlane(i);
                GetBoxToPlanePVertexNVertex(ref box, ref plane.Normal, out var p, out var n);
                if (CollisionsHelper.PlaneIntersectsPoint(ref plane, ref p) == PlaneIntersectionType.Back)
                    return ContainmentType.Disjoint;

                if (CollisionsHelper.PlaneIntersectsPoint(ref plane, ref n) == PlaneIntersectionType.Back)
                    result = ContainmentType.Intersects;
            }
            return result;
        }

        /// <summary>
        /// Determines the intersection relationship between the frustum and a bounding box.
        /// </summary>
        /// <param name="box">The box.</param>
        /// <returns>Type of the containment</returns>
        public ContainmentType Contains(BoundingBox box)
        {
            return Contains(ref box);
        }

        /// <summary>
        /// Determines the intersection relationship between the frustum and a bounding box.
        /// </summary>
        /// <param name="box">The box.</param>
        /// <param name="result">Type of the containment.</param>
        public void Contains(ref BoundingBox box, out ContainmentType result)
        {
            result = Contains(ref box);
        }

        /// <summary>
        /// Determines the intersection relationship between the frustum and a bounding sphere.
        /// </summary>
        /// <param name="sphere">The sphere.</param>
        /// <returns>Type of the containment</returns>
        public ContainmentType Contains(ref BoundingSphere sphere)
        {
            var result = PlaneIntersectionType.Front;
            var planeResult = PlaneIntersectionType.Front;
            for (var i = 0; i < 6; i++)
            {
                switch (i)
                {
                case 0:
                    planeResult = pNear.Intersects(ref sphere);
                    break;
                case 1:
                    planeResult = pFar.Intersects(ref sphere);
                    break;
                case 2:
                    planeResult = pLeft.Intersects(ref sphere);
                    break;
                case 3:
                    planeResult = pRight.Intersects(ref sphere);
                    break;
                case 4:
                    planeResult = pTop.Intersects(ref sphere);
                    break;
                case 5:
                    planeResult = pBottom.Intersects(ref sphere);
                    break;
                }
                switch (planeResult)
                {
                case PlaneIntersectionType.Back:
                    return ContainmentType.Disjoint;
                case PlaneIntersectionType.Intersecting:
                    result = PlaneIntersectionType.Intersecting;
                    break;
                }
            }
            switch (result)
            {
            case PlaneIntersectionType.Intersecting:
                return ContainmentType.Intersects;
            default:
                return ContainmentType.Contains;
            }
        }

        /// <summary>
        /// Determines the intersection relationship between the frustum and a bounding sphere.
        /// </summary>
        /// <param name="sphere">The sphere.</param>
        /// <returns>Type of the containment</returns>
        public ContainmentType Contains(BoundingSphere sphere)
        {
            return Contains(ref sphere);
        }

        /// <summary>
        /// Determines the intersection relationship between the frustum and a bounding sphere.
        /// </summary>
        /// <param name="sphere">The sphere.</param>
        /// <param name="result">Type of the containment.</param>
        public void Contains(ref BoundingSphere sphere, out ContainmentType result)
        {
            result = Contains(ref sphere);
        }

        /// <summary>
        /// Determines the intersection relationship between the frustum and another bounding frustum.
        /// </summary>
        /// <param name="frustum">The frustum.</param>
        /// <returns>Type of the containment</returns>
        public bool Contains(ref BoundingFrustum frustum)
        {
            return Contains(frustum.GetCorners()) != ContainmentType.Disjoint;
        }

        /// <summary>
        /// Determines the intersection relationship between the frustum and another bounding frustum.
        /// </summary>
        /// <param name="frustum">The frustum.</param>
        /// <returns>Type of the containment</returns>
        public bool Contains(BoundingFrustum frustum)
        {
            return Contains(ref frustum);
        }

        /// <summary>
        /// Determines the intersection relationship between the frustum and another bounding frustum.
        /// </summary>
        /// <param name="frustum">The frustum.</param>
        /// <param name="result">Type of the containment.</param>
        public void Contains(ref BoundingFrustum frustum, out bool result)
        {
            result = Contains(frustum.GetCorners()) != ContainmentType.Disjoint;
        }

        /// <summary>
        /// Checks whether the current BoundingFrustum intersects a BoundingSphere.
        /// </summary>
        /// <param name="sphere">The sphere.</param>
        /// <returns>Type of the containment</returns>
        public bool Intersects(ref BoundingSphere sphere)
        {
            return Contains(ref sphere) != ContainmentType.Disjoint;
        }

        /// <summary>
        /// Checks whether the current BoundingFrustum intersects a BoundingSphere.
        /// </summary>
        /// <param name="sphere">The sphere.</param>
        /// <param name="result">Set to <c>true</c> if the current BoundingFrustum intersects a BoundingSphere.</param>
        public void Intersects(ref BoundingSphere sphere, out bool result)
        {
            result = Contains(ref sphere) != ContainmentType.Disjoint;
        }

        /// <summary>
        /// Checks whether the current BoundingFrustum intersects a BoundingBox.
        /// </summary>
        /// <param name="box">The box.</param>
        /// <returns><c>true</c> if the current BoundingFrustum intersects a BoundingSphere.</returns>
        public bool Intersects(ref BoundingBox box)
        {
            return Contains(ref box) != ContainmentType.Disjoint;
        }

        /// <summary>
        /// Checks whether the current BoundingFrustum intersects a BoundingBox.
        /// </summary>
        /// <param name="box">The box.</param>
        /// <param name="result"><c>true</c> if the current BoundingFrustum intersects a BoundingSphere.</param>
        public void Intersects(ref BoundingBox box, out bool result)
        {
            result = Contains(ref box) != ContainmentType.Disjoint;
        }

        private PlaneIntersectionType PlaneIntersectsPoints(ref Plane plane, Vector3[] points)
        {
            PlaneIntersectionType result = CollisionsHelper.PlaneIntersectsPoint(ref plane, ref points[0]);
            for (var i = 1; i < points.Length; i++)
                if (CollisionsHelper.PlaneIntersectsPoint(ref plane, ref points[i]) != result)
                    return PlaneIntersectionType.Intersecting;
            return result;
        }

        /// <summary>
        /// Checks whether the current BoundingFrustum intersects the specified Plane.
        /// </summary>
        /// <param name="plane">The plane.</param>
        /// <returns>Plane intersection type.</returns>
        public PlaneIntersectionType Intersects(ref Plane plane)
        {
            return PlaneIntersectsPoints(ref plane, GetCorners());
        }

        /// <summary>
        /// Checks whether the current BoundingFrustum intersects the specified Plane.
        /// </summary>
        /// <param name="plane">The plane.</param>
        /// <param name="result">Plane intersection type.</param>
        public void Intersects(ref Plane plane, out PlaneIntersectionType result)
        {
            result = PlaneIntersectsPoints(ref plane, GetCorners());
        }

        /// <summary>
        /// Get the width of the frustum at specified depth.
        /// </summary>
        /// <param name="depth">the depth at which to calculate frustum width.</param>
        /// <returns>With of the frustum at the specified depth</returns>
        public float GetWidthAtDepth(float depth)
        {
            var hAngle = (float)(Math.PI / 2.0 - Math.Acos(Vector3.Dot(pNear.Normal, pLeft.Normal)));
            return (float)(Math.Tan(hAngle) * depth * 2);
        }

        /// <summary>
        /// Get the height of the frustum at specified depth.
        /// </summary>
        /// <param name="depth">the depth at which to calculate frustum height.</param>
        /// <returns>Height of the frustum at the specified depth</returns>
        public float GetHeightAtDepth(float depth)
        {
            var vAngle = (float)(Math.PI / 2.0 - Math.Acos(Vector3.Dot(pNear.Normal, pTop.Normal)));
            return (float)(Math.Tan(vAngle) * depth * 2);
        }

        private BoundingFrustum GetInsideOutClone()
        {
            BoundingFrustum frustum = this;
            frustum.pNear.Normal = -frustum.pNear.Normal;
            frustum.pFar.Normal = -frustum.pFar.Normal;
            frustum.pLeft.Normal = -frustum.pLeft.Normal;
            frustum.pRight.Normal = -frustum.pRight.Normal;
            frustum.pTop.Normal = -frustum.pTop.Normal;
            frustum.pBottom.Normal = -frustum.pBottom.Normal;
            return frustum;
        }

        /// <summary>
        /// Checks whether the current BoundingFrustum intersects the specified Ray.
        /// </summary>
        /// <param name="ray">The ray.</param>
        /// <returns><c>true</c> if the current BoundingFrustum intersects the specified Ray.</returns>
        public bool Intersects(ref Ray ray)
        {
            return Intersects(ref ray, out _, out _);
        }

        /// <summary>
        /// Checks whether the current BoundingFrustum intersects the specified Ray.
        /// </summary>
        /// <param name="ray">The Ray to check for intersection with.</param>
        /// <param name="inDistance">
        /// The distance at which the ray enters the frustum if there is an intersection and the ray
        /// starts outside the frustum.
        /// </param>
        /// <param name="outDistance">The distance at which the ray exits the frustum if there is an intersection.</param>
        /// <returns><c>true</c> if the current BoundingFrustum intersects the specified Ray.</returns>
        public bool Intersects(ref Ray ray, out float? inDistance, out float? outDistance)
        {
            if (Contains(ray.Position) != ContainmentType.Disjoint)
            {
                float nearstPlaneDistance = float.MaxValue;
                for (var i = 0; i < 6; i++)
                {
                    Plane plane = GetPlane(i);
                    if (CollisionsHelper.RayIntersectsPlane(ref ray, ref plane, out float distance) && (distance < nearstPlaneDistance))
                        nearstPlaneDistance = distance;
                }

                inDistance = nearstPlaneDistance;
                outDistance = null;
                return true;
            }
            //We will find the two points at which the ray enters and exists the frustum
            //These two points make a line which center inside the frustum if the ray intersects it
            //Or outside the frustum if the ray intersects frustum planes outside it.
            float minDist = float.MaxValue;
            float maxDist = float.MinValue;
            for (var i = 0; i < 6; i++)
            {
                Plane plane = GetPlane(i);
                if (CollisionsHelper.RayIntersectsPlane(ref ray, ref plane, out float distance))
                {
                    minDist = Math.Min(minDist, distance);
                    maxDist = Math.Max(maxDist, distance);
                }
            }

            Vector3 minPoint = ray.Position + ray.Direction * minDist;
            Vector3 maxPoint = ray.Position + ray.Direction * maxDist;
            Vector3 center = (minPoint + maxPoint) / 2f;
            if (Contains(ref center) != ContainmentType.Disjoint)
            {
                inDistance = minDist;
                outDistance = maxDist;
                return true;
            }
            inDistance = null;
            outDistance = null;
            return false;
        }

        /// <summary>
        /// Get the distance which when added to camera position along the lookat direction will do the effect of zoom to extents
        /// (zoom to fit) operation,
        /// so all the passed points will fit in the current view.
        /// if the returned value is positive, the camera will move toward the lookat direction (ZoomIn).
        /// if the returned value is negative, the camera will move in the reverse direction of the lookat direction (ZoomOut).
        /// </summary>
        /// <param name="points">The points.</param>
        /// <returns>The zoom to fit distance</returns>
        public float GetZoomToExtentsShiftDistance(Vector3[] points)
        {
            var vAngle = (float)(Math.PI / 2.0 - Math.Acos(Vector3.Dot(pNear.Normal, pTop.Normal)));
            var vSin = (float)Math.Sin(vAngle);
            var hAngle = (float)(Math.PI / 2.0 - Math.Acos(Vector3.Dot(pNear.Normal, pLeft.Normal)));
            var hSin = (float)Math.Sin(hAngle);
            float horizontalToVerticalMapping = vSin / hSin;

            BoundingFrustum ioFrustrum = GetInsideOutClone();

            float maxPointDist = float.MinValue;
            for (var i = 0; i < points.Length; i++)
            {
                float pointDist = CollisionsHelper.DistancePlanePoint(ref ioFrustrum.pTop, ref points[i]);
                pointDist = Math.Max(pointDist, CollisionsHelper.DistancePlanePoint(ref ioFrustrum.pBottom, ref points[i]));
                pointDist = Math.Max(pointDist, CollisionsHelper.DistancePlanePoint(ref ioFrustrum.pLeft, ref points[i]) * horizontalToVerticalMapping);
                pointDist = Math.Max(pointDist, CollisionsHelper.DistancePlanePoint(ref ioFrustrum.pRight, ref points[i]) * horizontalToVerticalMapping);

                maxPointDist = Math.Max(maxPointDist, pointDist);
            }
            return -maxPointDist / vSin;
        }

        /// <summary>
        /// Get the distance which when added to camera position along the lookat direction will do the effect of zoom to extents
        /// (zoom to fit) operation,
        /// so all the passed points will fit in the current view.
        /// if the returned value is positive, the camera will move toward the lookat direction (ZoomIn).
        /// if the returned value is negative, the camera will move in the reverse direction of the lookat direction (ZoomOut).
        /// </summary>
        /// <param name="boundingBox">The bounding box.</param>
        /// <returns>The zoom to fit distance</returns>
        public float GetZoomToExtentsShiftDistance(ref BoundingBox boundingBox)
        {
            return GetZoomToExtentsShiftDistance(boundingBox.GetCorners());
        }

        /// <summary>
        /// Get the vector shift which when added to camera position will do the effect of zoom to extents (zoom to fit)
        /// operation,
        /// so all the passed points will fit in the current view.
        /// </summary>
        /// <param name="points">The points.</param>
        /// <returns>The zoom to fit vector</returns>
        public Vector3 GetZoomToExtentsShiftVector(Vector3[] points)
        {
            return GetZoomToExtentsShiftDistance(points) * pNear.Normal;
        }

        /// <summary>
        /// Get the vector shift which when added to camera position will do the effect of zoom to extents (zoom to fit)
        /// operation,
        /// so all the passed points will fit in the current view.
        /// </summary>
        /// <param name="boundingBox">The bounding box.</param>
        /// <returns>The zoom to fit vector</returns>
        public Vector3 GetZoomToExtentsShiftVector(ref BoundingBox boundingBox)
        {
            return GetZoomToExtentsShiftDistance(boundingBox.GetCorners()) * pNear.Normal;
        }

        /// <summary>
        /// Indicate whether the current BoundingFrustum is Orthographic.
        /// </summary>
        /// <value>
        /// <c>true</c> if the current BoundingFrustum is Orthographic; otherwise, <c>false</c>.
        /// </value>
        public bool IsOrthographic => (pLeft.Normal == -pRight.Normal) && (pTop.Normal == -pBottom.Normal);
    }
}
