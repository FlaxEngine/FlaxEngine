// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Runtime.InteropServices;

namespace FlaxEngine
{
    partial class Spline
    {
        private BezierCurve<Transform>.Keyframe[] _keyframes;

        /// <summary>
        /// Gets or sets the spline keyframes collection.
        /// </summary>
        [Unmanaged]
        [Tooltip("Spline keyframes collection.")]
        [EditorOrder(10), EditorDisplay("Spline"), Collection(CanReorderItems = false)]
        public BezierCurve<Transform>.Keyframe[] SplineKeyframes
        {
            get
            {
                var count = SplinePointsCount;
                if (_keyframes == null || _keyframes.Length != count)
                    _keyframes = new BezierCurve<Transform>.Keyframe[count];
#if !BUILD_RELEASE
                if (System.Runtime.CompilerServices.Unsafe.SizeOf<BezierCurve<Transform>.Keyframe>() != Transform.SizeInBytes * 3 + sizeof(float))
                    throw new Exception("Invalid size of BezierCurve keyframe " + System.Runtime.CompilerServices.Unsafe.SizeOf<BezierCurve<Transform>.Keyframe>() + " bytes.");
#endif
                Internal_GetKeyframes(__unmanagedPtr, _keyframes);
                return _keyframes;
            }
            set
            {
                if (value == null)
                    value = Utils.GetEmptyArray<BezierCurve<Transform>.Keyframe>();
                _keyframes = null;
                Internal_SetKeyframes(__unmanagedPtr, value, System.Runtime.CompilerServices.Unsafe.SizeOf<BezierCurve<Transform>.Keyframe>());
            }
        }

        /// <summary>
        /// Gets the spline keyframe.
        /// </summary>
        /// <param name="index">The spline point index.</param>
        /// <returns>The keyframe.</returns>
        public BezierCurve<Transform>.Keyframe GetSplineKeyframe(int index)
        {
            return SplineKeyframes[index];
        }

        /// <summary>
        /// Sets the spline keyframe.
        /// </summary>
        /// <param name="index">The spline point index.</param>
        /// <param name="keyframe">The keyframe.</param>
        public void SetSplineKeyframe(int index, BezierCurve<Transform>.Keyframe keyframe)
        {
            SetSplineLocalTransform(index, keyframe.Value, false);
            SetSplineLocalTangent(index, keyframe.Value + keyframe.TangentIn, true, false);
            SetSplineLocalTangent(index, keyframe.Value + keyframe.TangentOut, false);
        }
    }
}
