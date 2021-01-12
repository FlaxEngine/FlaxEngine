// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    /// <summary>
    /// A collection of common random functions which supports various types.
    /// </summary>
    public static class Randomf
    {
        // Single instance for random
        private static Random _rand = new Random(0);

        /// <summary>
        /// Sets the seed of the random number generator.
        /// Causes to construct a new <see cref="Random"/> instance.
        /// </summary>
        /// <param name="newSeed">The new seed</param>
        public static void SetSeed(int newSeed) => _rand = new Random(newSeed);

        // Bool

        public static bool RandomBool()
        {
            return RandomInt() == 1 ? true : false;
        }

        public static bool RandomBoolWithWeight(float weight = 0)
        {
            return weight >= RandomInt() ? true : false;
        }

        // Byte

        public static byte RandomByte(byte min = 0, byte max = 1)
        {
            return (byte)_rand.Next(min, max+1);
        }

        public static byte RandomUniformByte(byte value = 1)
        {
            int max = Math.Abs(value);
            int min = max * -1;

            return (byte)_rand.Next(min, max);
        }

        // Int

        public static int RandomInt(int min = 0, int max = 1)
        {
            return _rand.Next(min, max+1);
        }

        public static int RandomUniformInt(int value = 1)
        {
            int max = Math.Abs(value);
            int min = max * -1;

            return _rand.Next(min, max);
        }

        // Float

        public static float RandomFloat(float min = 0.0f, float max = 1.0f)
        {
            return (float)_rand.NextDouble() * (max - min) + min;
        }

        public static float RandomUniformFloat(float value = 1.0f)
        {
            // Ensure that in case of negative values we still return a value between + value and - value.
            float max = Math.Abs(value);
            float min = max * -1;

            return (float)_rand.NextDouble() * (max - min) + min;
        }

        // Quaternion

        public static Quaternion RandomQuaternion()
        {
            return Quaternion.Euler(
                RandomUniformFloat(180),
                RandomUniformFloat(180),
                RandomUniformFloat(180)
                );
        }

        // Vector

        public static Vector3 RandomVector3()
        {
            Vector3 output;

            do
            {
                // Create random float with a mean of 0 and deviation of ±1;
                output.X = RandomFloat() * 2.0f - 1.0f;
                output.Y = RandomFloat() * 2.0f - 1.0f;
                output.Z = RandomFloat() * 2.0f - 1.0f;

            } while (output.LengthSquared > 1 || output.LengthSquared < 1e-6f);

            output *= (1.0f / (float)Math.Sqrt(output.LengthSquared));

            return output;
        }

        public static Vector2 RandomVector2()
        {
            Vector2 output;

            do
            {
                // Create random float with a mean of 0 and deviation of +-1;
                output.X = RandomFloat() * 2.0f - 1.0f;
                output.Y = RandomFloat() * 2.0f - 1.0f;

            } while (output.LengthSquared > 1 || output.LengthSquared< 1e-6f);

            output *= (1.0f / (float)Math.Sqrt(output.LengthSquared));

            return output;
        }

        // Color

        public static Color RandomColor(bool trueRandom, bool randomAlpha)
        {
            float alpha = randomAlpha ? RandomFloat() : 1f;

            if (!trueRandom)
                return Color.FromHSV(RandomFloat(0f, 360f), 1f, 1f, alpha);

            return new Color(
                RandomFloat(0f, 255f),
                RandomFloat(0f, 255f),
                RandomFloat(0f, 255f), alpha);
        }
    }
}
