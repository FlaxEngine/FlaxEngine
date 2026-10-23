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
        /// <param name="v">The v.</param>
        public Vector3LongFloat(int v) : this(v, v, v) { }

        /// <summary>
        /// Initializes a new instance of the <see cref="Vector3LongFloat"/> struct.
        /// </summary>
        /// <param name="v">The v.</param>
        public Vector3LongFloat(uint v) : this(v, v, v) { }

        /// <summary>
        /// Initializes a new instance of the <see cref="Vector3LongFloat"/> struct.
        /// </summary>
        /// <param name="v">The v.</param>
        public Vector3LongFloat(long v) : this(v, v, v) { }

        /// <summary>
        /// Initializes a new instance of the <see cref="Vector3LongFloat"/> struct.
        /// </summary>
        /// <param name="v">The v.</param>
        public Vector3LongFloat(ulong v) : this(v, v, v) { }

        /// <summary>
        /// Initializes a new instance of the <see cref="Vector3LongFloat"/> struct.
        /// </summary>
        /// <param name="v">The v.</param>
        public Vector3LongFloat(float v) : this(v, v, v) { }

        /// <summary>
        /// Initializes a new instance of the <see cref="Vector3LongFloat"/> struct.
        /// </summary>
        /// <param name="v">The v.</param>
        public Vector3LongFloat(double v) : this(v, v, v) { }

        /// <summary>
        /// Initializes a new instance of the <see cref="Vector3LongFloat"/> struct.
        /// </summary>
        /// <param name="X">The x.</param>
        /// <param name="Y">The y.</param>
        /// <param name="Z">The z.</param>
        public Vector3LongFloat(int X, int Y, int Z) : this()
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
        public Vector3LongFloat(uint X, uint Y, uint Z) : this()
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
        public Vector3LongFloat(long X, long Y, long Z) : this()
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
        public Vector3LongFloat(ulong X, ulong Y, ulong Z) : this()
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
        /// Gets the length.
        /// </summary>
        /// <value>
        /// The length.
        /// </value>
        public readonly LongFloat Length => LongFloatMath.SquareRoot(LengthSquared);

        /// <summary>
        /// Gets the length squared.
        /// </summary>
        /// <value>
        /// The length squared.
        /// </value>
        public readonly LongFloat LengthSquared => X * X + Y * Y + Z * Z;


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

        /// <summary>
        /// Distances the squared.
        /// </summary>
        /// <param name="rhs">The RHS.</param>
        /// <param name="lhs">The LHS.</param>
        /// <returns></returns>
        public static LongFloat DistanceSquared(Vector3LongFloat rhs, Vector3LongFloat lhs)
        {
            LongFloat x = rhs.X - lhs.X;
            LongFloat y = rhs.Y - lhs.Y;
            LongFloat z = rhs.Z - lhs.Z;
            LongFloat squared = x * x + y * y + z * z;
            if (LongFloatMath.IsNAN(squared))
            {
                //set it to max overflow triggered probably
                //todo: for now it will return max value but if needed it can crash,remove this to do if decision was made
                return LongFloat.Max;
            }
            return squared;
        }
    }
}
