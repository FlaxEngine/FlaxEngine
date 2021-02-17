// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Globalization;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

// ReSharper disable CompareOfFloatsByEqualityOperator

namespace FlaxEngine
{
    [Serializable]
    partial struct Transform : IEquatable<Transform>, IFormattable
    {
        private static readonly string _formatString = "Translation:{0} Orientation:{1} Scale:{2}";

        /// <summary>
        /// The size of the <see cref="Transform" /> type, in bytes
        /// </summary>
        public static readonly int SizeInBytes = Marshal.SizeOf(typeof(Transform));

        /// <summary>
        /// A identity <see cref="Transform" /> with all default values
        /// </summary>
        public static readonly Transform Identity = new Transform(Vector3.Zero);

        /// <summary>
        /// Init
        /// </summary>
        /// <param name="position">Position in 3D space</param>
        public Transform(Vector3 position)
        {
            Translation = position;
            Orientation = Quaternion.Identity;
            Scale = Vector3.One;
        }

        /// <summary>
        /// Init
        /// </summary>
        /// <param name="position">Position in 3D space</param>
        /// <param name="rotation">Rotation in 3D space</param>
        public Transform(Vector3 position, Quaternion rotation)
        {
            Translation = position;
            Orientation = rotation;
            Scale = Vector3.One;
        }

        /// <summary>
        /// Init
        /// </summary>
        /// <param name="position">Position in 3D space</param>
        /// <param name="rotation">Rotation in 3D space</param>
        /// <param name="scale">Transform scale</param>
        public Transform(Vector3 position, Quaternion rotation, Vector3 scale)
        {
            Translation = position;
            Orientation = rotation;
            Scale = scale;
        }

        /// <summary>
        /// Creates a new Transform from a matrix
        /// </summary>
        /// <param name="transform">World matrix</param>
        public Transform(Matrix transform)
        {
            transform.Decompose(out Scale, out Orientation, out Translation);
        }

        /// <summary>
        /// Creates a new Transform from a matrix
        /// </summary>
        /// <param name="transform">World matrix</param>
        public Transform(ref Matrix transform)
        {
            transform.Decompose(out Scale, out Orientation, out Translation);
        }

        /// <summary>
        /// Gets a value indicting whether this transform is identity
        /// </summary>
        public bool IsIdentity => Equals(Identity);

        /// <summary>
        /// Gets the forward vector.
        /// </summary>
        public Vector3 Forward => Vector3.Transform(Vector3.Forward, Orientation);

        /// <summary>
        /// Gets the backward vector.
        /// </summary>
        public Vector3 Backward => Vector3.Transform(Vector3.Backward, Orientation);

        /// <summary>
        /// Gets the up vector.
        /// </summary>
        public Vector3 Up => Vector3.Transform(Vector3.Up, Orientation);

        /// <summary>
        /// Gets the down vector.
        /// </summary>
        public Vector3 Down => Vector3.Transform(Vector3.Down, Orientation);

        /// <summary>
        /// Gets the left vector.
        /// </summary>
        public Vector3 Left => Vector3.Transform(Vector3.Left, Orientation);

        /// <summary>
        /// Gets the right vector.
        /// </summary>
        public Vector3 Right => Vector3.Transform(Vector3.Right, Orientation);

        /// <summary>
        /// Gets rotation matrix (from Orientation).
        /// </summary>
        /// <returns>Rotation matrix</returns>
        public Matrix GetRotation()
        {
            Matrix.RotationQuaternion(ref Orientation, out var result);
            return result;
        }

        /// <summary>
        /// Gets rotation matrix (from Orientation).
        /// </summary>
        /// <param name="result">Matrix to set</param>
        public void GetRotation(out Matrix result)
        {
            Matrix.RotationQuaternion(ref Orientation, out result);
        }

        /// <summary>
        /// Sets rotation matrix (from Orientation).
        /// </summary>
        /// <param name="value">Rotation matrix</param>
        public void SetRotation(Matrix value)
        {
            Quaternion.RotationMatrix(ref value, out Orientation);
        }

        /// <summary>
        /// Sets rotation matrix (from Orientation).
        /// </summary>
        /// <param name="value">Rotation matrix</param>
        public void SetRotation(ref Matrix value)
        {
            Quaternion.RotationMatrix(ref value, out Orientation);
        }

        /// <summary>
        /// Gets world matrix that describes transformation as a 4 by 4 matrix.
        /// </summary>
        /// <returns>World matrix</returns>
        public Matrix GetWorld()
        {
            Matrix.Transformation(ref Scale, ref Orientation, ref Translation, out var result);
            return result;
        }

        /// <summary>
        /// Gets world matrix that describes transformation as a 4 by 4 matrix.
        /// </summary>
        /// <param name="result">World matrix</param>
        public void GetWorld(out Matrix result)
        {
            Matrix.Transformation(ref Scale, ref Orientation, ref Translation, out result);
        }

        /// <summary>
        /// Adds two transforms.
        /// </summary>
        /// <param name="left">The first transform to add.</param>
        /// <param name="right">The second transform to add.</param>
        /// <returns>The sum of the two transforms.</returns>
        public static Transform Add(Transform left, Transform right)
        {
            Transform result;
            Quaternion.Multiply(ref left.Orientation, ref right.Orientation, out result.Orientation);
            Vector3.Multiply(ref left.Scale, ref right.Scale, out result.Scale);
            Vector3.Add(ref left.Translation, ref right.Translation, out result.Translation);
            return result;
        }

        /// <summary>
        /// Subtracts two transforms.
        /// </summary>
        /// <param name="left">The first transform to subtract from.</param>
        /// <param name="right">The second transform to subtract.</param>
        /// <returns>The difference of the two transforms.</returns>
        public static Transform Subtract(Transform left, Transform right)
        {
            Transform result;
            Vector3.Subtract(ref left.Translation, ref right.Translation, out result.Translation);
            Quaternion invRotation = right.Orientation.Conjugated();
            Quaternion.Multiply(ref left.Orientation, ref invRotation, out result.Orientation);
            Vector3.Divide(ref left.Scale, ref right.Scale, out result.Scale);
            return result;
        }

        /// <summary>
        /// Perform transformation of the given transform in local space
        /// </summary>
        /// <param name="other">Local space transform</param>
        /// <returns>World space transform</returns>
        public Transform LocalToWorld(Transform other)
        {
            Transform result;
            Quaternion.Multiply(ref Orientation, ref other.Orientation, out result.Orientation);
            Vector3.Multiply(ref Scale, ref other.Scale, out result.Scale);
            result.Translation = LocalToWorld(other.Translation);
            return result;
        }

        /// <summary>
        /// Perform transformation of the given point in local space
        /// </summary>
        /// <param name="point">Local space point</param>
        /// <returns>World space point</returns>
        public Vector3 LocalToWorld(Vector3 point)
        {
            point *= Scale;
            Vector3.Transform(ref point, ref Orientation, out point);
            return point + Translation;
        }

        /// <summary>
        /// Performs transformation of the given vector in local space to the world space of this transform.
        /// </summary>
        /// <param name="vector">The local space vector.</param>
        /// <returns>The world space vector.</returns>
        public Vector3 LocalToWorldVector(Vector3 vector)
        {
            vector *= Scale;
            Vector3.Transform(ref vector, ref Orientation, out vector);
            return vector;
        }

        /// <summary>
        /// Perform transformation of the given points in local space
        /// </summary>
        /// <param name="points">Local space points</param>
        /// <param name="result">World space points</param>
        public void LocalToWorld(Vector3[] points, Vector3[] result)
        {
            for (int i = 0; i < points.Length; i++)
            {
                result[i] = Vector3.Transform(points[i] * Scale, Orientation) + Translation;
            }
        }

        /// <summary>
        /// Perform transformation of the given transform in world space
        /// </summary>
        /// <param name="other">World space transform</param>
        /// <returns>Local space transform</returns>
        public Transform WorldToLocal(Transform other)
        {
            Vector3 invScale = Scale;
            if (invScale.X != 0.0f)
                invScale.X = 1.0f / invScale.X;
            if (invScale.Y != 0.0f)
                invScale.Y = 1.0f / invScale.Y;
            if (invScale.Z != 0.0f)
                invScale.Z = 1.0f / invScale.Z;

            Transform result;
            result.Orientation = Orientation;
            result.Orientation.Invert();
            Quaternion.Multiply(ref result.Orientation, ref other.Orientation, out result.Orientation);
            Vector3.Multiply(ref other.Scale, ref invScale, out result.Scale);
            result.Translation = WorldToLocal(other.Translation);

            return result;
        }

        /// <summary>
        /// Perform transformation of the given point in world space
        /// </summary>
        /// <param name="point">World space point</param>
        /// <returns>Local space point</returns>
        public Vector3 WorldToLocal(Vector3 point)
        {
            Vector3 invScale = Scale;
            if (invScale.X != 0.0f)
                invScale.X = 1.0f / invScale.X;
            if (invScale.Y != 0.0f)
                invScale.Y = 1.0f / invScale.Y;
            if (invScale.Z != 0.0f)
                invScale.Z = 1.0f / invScale.Z;

            Quaternion invRotation = Orientation;
            invRotation.Invert();

            Vector3 result = point - Translation;
            Vector3.Transform(ref result, ref invRotation, out result);

            return result * invScale;
        }

        /// <summary>
        /// Perform transformation of the given vector in world space
        /// </summary>
        /// <param name="vector">World space vector</param>
        /// <returns>Local space vector</returns>
        public Vector3 WorldToLocalVector(Vector3 vector)
        {
            Vector3 invScale = Scale;
            if (invScale.X != 0.0f)
                invScale.X = 1.0f / invScale.X;
            if (invScale.Y != 0.0f)
                invScale.Y = 1.0f / invScale.Y;
            if (invScale.Z != 0.0f)
                invScale.Z = 1.0f / invScale.Z;

            Quaternion invRotation = Orientation;
            invRotation.Invert();

            Vector3.Transform(ref vector, ref invRotation, out var result);

            return result * invScale;
        }

        /// <summary>
        /// Perform transformation of the given points in world space
        /// </summary>
        /// <param name="points">World space points</param>
        /// <param name="result">Local space points</param>
        public void WorldToLocal(Vector3[] points, Vector3[] result)
        {
            Vector3 invScale = Scale;
            if (invScale.X != 0.0f)
                invScale.X = 1.0f / invScale.X;
            if (invScale.Y != 0.0f)
                invScale.Y = 1.0f / invScale.Y;
            if (invScale.Z != 0.0f)
                invScale.Z = 1.0f / invScale.Z;

            Quaternion invRotation = Orientation;
            invRotation.Invert();

            for (int i = 0; i < points.Length; i++)
            {
                result[i] = points[i] - Translation;
                Vector3.Transform(ref result[i], ref invRotation, out result[i]);
                result[i] *= invScale;
            }
        }

        /// <summary>
        /// Transforms the direction vector from the local space to the world space.
        /// </summary>
        /// <remarks>
        /// This operation is not affected by scale or position of the transform. The returned vector has the same length as direction.
        /// Use <see cref="TransformPoint"/> for the conversion if the vector represents a position rather than a direction.
        /// </remarks>
        /// <param name="direction">The direction.</param>
        /// <returns>The transformed direction vector.</returns>
        public Vector3 TransformDirection(Vector3 direction)
        {
            Vector3.Transform(ref direction, ref Orientation, out var result);
            return result;
        }

        /// <summary>
        /// Transforms the position from the local space to the world space.
        /// </summary>
        /// <remarks>
        /// Use <see cref="TransformDirection"/> for the conversion if the vector represents a direction rather than a position.
        /// </remarks>
        /// <param name="position">The position.</param>
        /// <returns>The transformed position.</returns>
        public Vector3 TransformPoint(Vector3 position)
        {
            return LocalToWorld(position);
        }

        /// <summary>
        /// Performs a linear interpolation between two transformations.
        /// </summary>
        /// <param name="start">Start transformation.</param>
        /// <param name="end">End transformation.</param>
        /// <param name="amount">Value between 0 and 1 indicating the weight of <paramref name="end" />.</param>
        /// <returns>The linear interpolation of the two transformations.</returns>
        /// <remarks>Passing <paramref name="amount" /> a value of 0 will cause <paramref name="start" /> to be returned; a value of 1 will cause <paramref name="end" /> to be returned.</remarks>
        public static Transform Lerp(Transform start, Transform end, float amount)
        {
            Transform result;
            Vector3.Lerp(ref start.Translation, ref end.Translation, amount, out result.Translation);
            Quaternion.Slerp(ref start.Orientation, ref end.Orientation, amount, out result.Orientation);
            Vector3.Lerp(ref start.Scale, ref end.Scale, amount, out result.Scale);
            return result;
        }

        /// <summary>
        /// Performs a linear interpolation between two transformations.
        /// </summary>
        /// <param name="start">Start transformation.</param>
        /// <param name="end">End transformation.</param>
        /// <param name="amount">Value between 0 and 1 indicating the weight of <paramref name="end" />.</param>
        /// <param name="result">When the method completes, contains the linear interpolation of the two transformations.</param>
        /// <remarks>Passing <paramref name="amount" /> a value of 0 will cause <paramref name="start" /> to be returned; a value of 1 will cause <paramref name="end" /> to be returned.</remarks>
        public static void Lerp(ref Transform start, ref Transform end, float amount, out Transform result)
        {
            Vector3.Lerp(ref start.Translation, ref end.Translation, amount, out result.Translation);
            Quaternion.Slerp(ref start.Orientation, ref end.Orientation, amount, out result.Orientation);
            Vector3.Lerp(ref start.Scale, ref end.Scale, amount, out result.Scale);
        }

        /// <summary>
        /// Tests for equality between two objects.
        /// </summary>
        /// <param name="left">The first value to compare.</param>
        /// <param name="right">The second value to compare.</param>
        /// <returns><c>true</c> if <paramref name="left" /> has the same value as <paramref name="right" />; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator ==(Transform left, Transform right)
        {
            return left.Equals(ref right);
        }

        /// <summary>
        /// Tests for inequality between two objects.
        /// </summary>
        /// <param name="left">The first value to compare.</param>
        /// <param name="right">The second value to compare.</param>
        /// <returns><c>true</c> if <paramref name="left" /> has a different value than <paramref name="right" />; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator !=(Transform left, Transform right)
        {
            return !left.Equals(ref right);
        }

        /// <summary>
        /// Adds two transformations.
        /// </summary>
        /// <param name="left">The first transform to add.</param>
        /// <param name="right">The second transform to add.</param>
        /// <returns>The sum of the two transformations.</returns>
        public static Transform operator +(Transform left, Transform right)
        {
            return Add(left, right);
        }

        /// <summary>
        /// Subtracts two transformations.
        /// </summary>
        /// <param name="left">The first transform to subtract from.</param>
        /// <param name="right">The second transform to subtract.</param>
        /// <returns>The difference of the two transformations.</returns>
        public static Transform operator -(Transform left, Transform right)
        {
            return Subtract(left, right);
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public override string ToString()
        {
            return string.Format(CultureInfo.CurrentCulture, _formatString, Translation, Orientation, Scale);
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <param name="format">The format.</param>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public string ToString(string format)
        {
            if (format == null)
                return ToString();

            return string.Format(CultureInfo.CurrentCulture, _formatString, Translation.ToString(format, CultureInfo.CurrentCulture), Orientation.ToString(format, CultureInfo.CurrentCulture), Scale.ToString(format, CultureInfo.CurrentCulture));
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <param name="formatProvider">The format provider.</param>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public string ToString(IFormatProvider formatProvider)
        {
            return string.Format(formatProvider, _formatString, Translation, Orientation, Scale);
        }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <param name="format">The format.</param>
        /// <param name="formatProvider">The format provider.</param>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public string ToString(string format, IFormatProvider formatProvider)
        {
            if (format == null)
                return ToString(formatProvider);

            return string.Format(formatProvider, _formatString,
                                 Translation.ToString(format, formatProvider),
                                 Orientation.ToString(format, formatProvider),
                                 Scale.ToString(format, formatProvider));
        }

        /// <summary>
        /// Returns a hash code for this instance.
        /// </summary>
        /// <returns>A hash code for this instance, suitable for use in hashing algorithms and data structures like a hash table.</returns>
        public override int GetHashCode()
        {
            unchecked
            {
                int hashCode = Translation.GetHashCode();
                hashCode = (hashCode * 397) ^ Orientation.GetHashCode();
                hashCode = (hashCode * 397) ^ Scale.GetHashCode();
                return hashCode;
            }
        }

        /// <summary>
        /// Tests whether one transform is near another transform.
        /// </summary>
        /// <param name="left">The left transform.</param>
        /// <param name="right">The right transform.</param>
        /// <param name="epsilon">The epsilon.</param>
        /// <returns><c>true</c> if left and right are near another, <c>false</c> otherwise</returns>
        public static bool NearEqual(Transform left, Transform right, float epsilon = Mathf.Epsilon)
        {
            return NearEqual(ref left, ref right, epsilon);
        }

        /// <summary>
        /// Tests whether one transform is near another transform.
        /// </summary>
        /// <param name="left">The left transform.</param>
        /// <param name="right">The right transform.</param>
        /// <param name="epsilon">The epsilon.</param>
        /// <returns><c>true</c> if left and right are near another, <c>false</c> otherwise</returns>
        public static bool NearEqual(ref Transform left, ref Transform right, float epsilon = Mathf.Epsilon)
        {
            return Vector3.NearEqual(ref left.Translation, ref right.Translation, epsilon) && Quaternion.NearEqual(ref left.Orientation, ref right.Orientation, epsilon) && Vector3.NearEqual(ref left.Scale, ref right.Scale, epsilon);
        }

        /// <summary>
        /// Determines whether the specified <see cref="Transform" /> is equal to this instance.
        /// </summary>
        /// <param name="other">The <see cref="Transform" /> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="Transform" /> is equal to this instance; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(ref Transform other)
        {
            return Translation == other.Translation && Orientation == other.Orientation && Scale == other.Scale;
        }

        /// <summary>
        /// Determines whether the specified <see cref="Transform" /> is equal to this instance.
        /// </summary>
        /// <param name="other">The <see cref="Transform" /> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="Transform" /> is equal to this instance; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(Transform other)
        {
            return Equals(ref other);
        }

        /// <summary>
        /// Determines whether the specified <see cref="System.Object" /> is equal to this instance.
        /// </summary>
        /// <param name="value">The <see cref="System.Object" /> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="System.Object" /> is equal to this instance; otherwise, <c>false</c>.</returns>
        public override bool Equals(object value)
        {
            if (!(value is Transform))
                return false;
            var strongValue = (Transform)value;
            return Equals(ref strongValue);
        }
    }
}
