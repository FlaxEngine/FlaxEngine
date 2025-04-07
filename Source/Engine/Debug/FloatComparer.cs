// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;

namespace FlaxEngine.Assertions
{
    /// <summary>
    /// A float comparer used by Assertions.Assert performing approximate comparison.
    /// </summary>
    [HideInEditor]
    public class FloatComparer : IEqualityComparer<float>
    {
        /// <summary>
        /// Default epsilon used by the comparer.
        /// </summary>
        public const float Epsilon = 1E-05f;

        /// <summary>
        /// Default instance of a comparer class with default error epsilon and absolute error check.
        /// </summary>
        public static readonly FloatComparer ComparerWithDefaultTolerance;

        private readonly float _error;
        private readonly bool _relative;

        static FloatComparer()
        {
            ComparerWithDefaultTolerance = new FloatComparer(1E-05f);
        }

        /// <summary>
        /// Creates an instance of the comparer.
        /// </summary>
        public FloatComparer()
        : this(Epsilon, false)
        {
        }

        /// <summary>
        /// Creates an instance of the comparer.
        /// </summary>
        /// <param name="relative">
        /// Should a relative check be used when comparing values? By default, an absolute check will be
        /// used.
        /// </param>
        public FloatComparer(bool relative)
        : this(Epsilon, relative)
        {
        }

        /// <summary>
        /// Creates an instance of the comparer.
        /// </summary>
        /// <param name="error">Allowed comparison error. By default, the FloatComparer.Epsilon is used.</param>
        public FloatComparer(float error)
        : this(error, false)
        {
        }

        /// <summary>
        /// Creates an instance of the comparer.
        /// </summary>
        /// <param name="relative">
        /// Should a relative check be used when comparing values? By default, an absolute check will be
        /// used.
        /// </param>
        /// <param name="error">Allowed comparison error. By default, the FloatComparer.Epsilon is used.</param>
        public FloatComparer(float error, bool relative)
        {
            _error = error;
            _relative = relative;
        }

        /// <summary>
        /// Performs equality check with absolute error check.
        /// </summary>
        /// <param name="expected">Expected value.</param>
        /// <param name="actual">Actual value.</param>
        /// <param name="error">Comparison error.</param>
        /// <returns>
        /// Result of the comparison.
        /// </returns>
        public static bool AreEqual(float expected, float actual, float error)
        {
            return Math.Abs(actual - expected) <= error;
        }

        /// <summary>
        /// Performs equality check with relative error check.
        /// </summary>
        /// <param name="expected">Expected value.</param>
        /// <param name="actual">Actual value.</param>
        /// <param name="error">Comparison error.</param>
        /// <returns>Result of the comparison.</returns>
        public static bool AreEqualRelative(float expected, float actual, float error)
        {
            if (expected == actual)
                return true;
            float single = Math.Abs(expected);
            float single1 = Math.Abs(actual);
            return Math.Abs((actual - expected) / (single <= single1 ? single1 : single)) <= error;
        }

        /// <inheritdoc />
        public bool Equals(float a, float b)
        {
            return !_relative ? AreEqual(a, b, _error) : AreEqualRelative(a, b, _error);
        }

        /// <inheritdoc />
        public int GetHashCode(float obj)
        {
            return GetHashCode();
        }
    }
}
