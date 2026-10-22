using FlaxEngine.Utilities;
using System;
using System.Collections.Generic;
using System.Text;

#if USE_LARGE_WORLDS
using Real = System.Double;
using Mathr = FlaxEngine.Mathd;
#else
using Real = System.Single;
using Mathr = FlaxEngine.Mathf;
#endif

namespace FlaxEngine
{
    public partial struct Vector3LongFloat
    {

        /// <summary>
        /// Initializes a new instance of the <see cref="Vector3LongFloat"/> struct.
        /// </summary>
        /// <param name="X">The x.</param>
        /// <param name="Y">The y.</param>
        /// <param name="Z">The z.</param>
        public Vector3LongFloat(float X, float Y, float Z) : this()
        {
            this.X = new LongFloat(X); 
            this.Y = new LongFloat(Y);
            this.Z = new LongFloat(Z);
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Vector3LongFloat"/> struct.
        /// </summary>
        /// <param name="X">The x.</param>
        /// <param name="Y">The y.</param>
        /// <param name="Z">The z.</param>
        public Vector3LongFloat(double X, double Y, double Z) : this()
        {
            this.X = new LongFloat(X);
            this.Y = new LongFloat(Y);
            this.Z = new LongFloat(Z);
        }

        /// <summary>
        /// Quantizes Long flaot to the Real Vector.
        /// </summary>
        /// <returns>
        /// Quantized Real Vector
        /// </returns>
        public readonly Vector3 Quantize()
        {
            return new Vector3(
                LongFloatMath.QuantizeToDouble(X),
                LongFloatMath.QuantizeToDouble(Y),
                LongFloatMath.QuantizeToDouble(Z));
        }

        /// <summary>
        /// Quantizes Long flaot to the Float3.
        /// </summary>
        /// <returns>
        /// Quantized Float3 Vector
        /// </returns>
        public readonly Float3 QuantizeToFloat3()
        {
            return new Float3(
                (float)LongFloatMath.QuantizeToDouble(X),
                (float)LongFloatMath.QuantizeToDouble(Y),
                (float)LongFloatMath.QuantizeToDouble(Z));
        }

        /// <summary>
        /// Quantizes Long flaot to the Double3.
        /// </summary>
        /// <returns>
        /// Quantized Double3 Vector
        /// </returns>
        public readonly Double3 QuantizeToDouble3()
        {
            return new Float3(
                (float)LongFloatMath.QuantizeToDouble(X),
                (float)LongFloatMath.QuantizeToDouble(Y),
                (float)LongFloatMath.QuantizeToDouble(Z));
        }
    }
}
