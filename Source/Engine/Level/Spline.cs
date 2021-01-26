// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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
                if (Marshal.SizeOf(typeof(BezierCurve<Transform>.Keyframe)) != Transform.SizeInBytes * 3 + sizeof(float))
                    throw new Exception("Invalid size of BezierCurve keyframe " + Marshal.SizeOf(typeof(BezierCurve<Transform>.Keyframe)) + " bytes.");
#endif
                Internal_GetKeyframes(__unmanagedPtr, _keyframes);
                return _keyframes;
            }
            set
            {
                if (value == null)
                    value = Utils.GetEmptyArray<BezierCurve<Transform>.Keyframe>();
                _keyframes = null;
                Internal_SetKeyframes(__unmanagedPtr, value);
            }
        }
    }
}
