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
        private Random.State _currentState;
        /// <summary>
        /// Holds the initial seed.
        /// </summary>
        private int _initialSeed;

        /// <summary>
        /// Init
        /// </summary>
        public RandomStream()
        {
            _currentState = Random.Snapshot();
            _initialSeed = _currentState.Integer;
        }

        /// <summary>
        /// Creates and initializes a new random stream from the specified seed value.
        /// </summary>
        /// <param name="seed">The seed value.</param>
        public RandomStream(int seed)
        {
            _initialSeed = seed;
            _currentState = new Random.State(seed);
        }

        /// <summary>
        /// Gets initial seed value
        /// </summary>
        public int GetInitialSeed() => _initialSeed;


        /// <summary>
        /// Gets the current seed.
        /// </summary>
        public int GetCurrentSeed() => _currentState.Integer;

        /// <summary>
        /// Initializes this random stream with the specified seed value.
        /// </summary>
        /// <param name="seed">The seed value.</param>
        public void Initialize(int seed)
        {
            _initialSeed = seed;
            _currentState = new Random.State(seed);
        }

        /// <summary>
        /// Resets this random stream to the initial seed value.
        /// </summary>
        public void Reset() => _currentState = new Random.State(_initialSeed);

        /// <summary>
        /// Generates a new random seed.
        /// </summary>
        public void GenerateNewSeed() => Initialize(Random.Integer());

        /// <summary>
        /// Returns a random boolean.
        /// </summary>
        public bool GetBool()
        {
            MutateSeed();
            return _currentState.Condition();
        }

        /// <summary>
        /// Returns a random number between 0 and uint.MaxValue.
        /// </summary>
        public uint GetUnsignedInt()
        {
            MutateSeed();
            return unchecked((uint)_currentState.Integer);
        }

        /// <summary>
        /// Returns a random number between 0 and 1.
        /// </summary>
        public float GetFraction()
        {
            MutateSeed();
            return _currentState.Float;
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
            MutateSeed();
            return _currentState.UniformRange(min, max);
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
            MutateSeed();
            return _currentState.UniformRange(min, max);
        }

        /// <summary>
        /// Mutates the current seed into the next seed.
        /// </summary>
        protected void MutateSeed() => _currentState = _currentState.Next;
    }
}
