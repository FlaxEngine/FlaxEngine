// Copyright (c) Wojciech Figat. All rights reserved.

using System.Runtime.CompilerServices;

#pragma warning disable 675

namespace FlaxEngine
{
    /// <summary>
    /// Very basic pseudo numbers generator.
    /// </summary>
    [HideInEditor]
    public class RandomStream
    {
        /// <summary>
        /// Holds the initial seed.
        /// </summary>
        private int _initialSeed;

        /// <summary>
        /// Holds the current seed.
        /// </summary>
        private int _seed;

        /// <summary>
        /// Init
        /// </summary>
        public RandomStream()
        {
            _initialSeed = 0;
            _seed = 0;
        }

        /// <summary>
        /// Creates and initializes a new random stream from the specified seed value.
        /// </summary>
        /// <param name="seed">The seed value.</param>
        public RandomStream(int seed)
        {
            _initialSeed = seed;
            _seed = seed;
        }

        /// <summary>
        /// Gets initial seed value
        /// </summary>
        public int GetInitialSeed()
        {
            return _initialSeed;
        }


        /// <summary>
        /// Gets the current seed.
        /// </summary>
        public int GetCurrentSeed()
        {
            return _seed;
        }

        /// <summary>
        /// Initializes this random stream with the specified seed value.
        /// </summary>
        /// <param name="seed">The seed value.</param>
        public void Initialize(int seed)
        {
            _initialSeed = seed;
            _seed = seed;
        }

        /// <summary>
        /// Resets this random stream to the initial seed value.
        /// </summary>
        public void Reset()
        {
            _seed = _initialSeed;
        }

        /// <summary>
        /// Generates a new random seed.
        /// </summary>
        public void GenerateNewSeed()
        {
            Initialize(new System.Random().Next());
        }

        /// <summary>
        /// Returns a random boolean.
        /// </summary>
        public unsafe bool GetBool()
        {
            MutateSeed();
            fixed (int* seedPtr = &_seed)
                return *seedPtr < (uint.MaxValue / 2);
        }

        /// <summary>
        /// Returns a random number between 0 and uint.MaxValue.
        /// </summary>
        public unsafe uint GetUnsignedInt()
        {
            MutateSeed();
            fixed (int* seedPtr = &_seed)
                return (uint)*seedPtr;
        }

        /// <summary>
        /// Returns a random number between 0 and 1.
        /// </summary>
        public unsafe float GetFraction()
        {
            MutateSeed();
            float temp = 1.0f;
            float result = 0.0f;
            *(int*)&result = (int)((*(int*)&temp & 0xff800000) | (_seed & 0x007fffff));
            return Mathf.Frac(result);
        }

        /// <summary>
        /// Returns a random number between 0 and 1.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public float Rand()
        {
            return GetFraction();
        }

        /// <summary>
        /// Returns a random vector of unit size.
        /// </summary>
        public Float3 GetUnitVector()
        {
            Float3 result;
            float l;
            do
            {
                result.X = GetFraction() * 2.0f - 1.0f;
                result.Y = GetFraction() * 2.0f - 1.0f;
                result.Z = GetFraction() * 2.0f - 1.0f;
                l = result.LengthSquared;
            } while (l > 1.0f || l < Mathf.Epsilon);
            return Float3.Normalize(result);
        }

        /// <summary>
        /// Gets a random <see cref="Vector3"/> with components in a range between [0;1].
        /// </summary>
        public Vector3 GetVector3()
        {
            return new Vector3(GetFraction(), GetFraction(), GetFraction());
        }

        /// <summary>
        /// Helper function for rand implementations.
        /// </summary>
        /// <param name="a">Top border</param>
        /// <returns>A random number in [0..A)</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public int RandHelper(int a)
        {
            return a > 0 ? Mathf.FloorToInt(GetFraction() * ((float)a - Mathf.Epsilon)) : 0;
        }

        /// <summary>
        /// Helper function for rand implementations
        /// </summary>
        /// <param name="min">Minimum value</param>
        /// <param name="max">Maximum value</param>
        /// <returns>A random number in [min; max] range</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public int RandRange(int min, int max)
        {
            int range = max - min + 1;
            return min + RandHelper(range);
        }

        /// <summary>
        /// Helper function for rand implementations
        /// </summary>
        /// <param name="min">Minimum value</param>
        /// <param name="max">Maximum value</param>
        /// <returns>A random number in [min; max] range</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public float RandRange(float min, float max)
        {
            return min + (max - min) * Rand();
        }

        /// <summary>
        /// Mutates the current seed into the next seed.
        /// </summary>
        protected void MutateSeed()
        {
            // This can be modified to provide better randomization
            _seed = _seed * 196314165 + 907633515;
        }
    }
}
