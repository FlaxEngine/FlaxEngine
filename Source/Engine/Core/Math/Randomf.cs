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

        //TODO: Implement some form of generic method for numerics?

        /// <summary>
        /// Sets the seed of the random number generator.
        /// Constructs a new <see cref="Random"/> instance.
        /// </summary>
        /// <param name="newSeed">The new seed.</param>
        public static void SetSeed(int newSeed) => _rand = new Random(newSeed);

        /// <summary>
        /// Generates a random <see cref="bool"/>.
        /// </summary>
        /// <returns>A <see cref="bool"/> thats either true or false.</returns>
        public static bool RandomBool() => RandomInt() == 1;

        /// <summary>
        /// Generates a random <see cref="bool"/> with a weight value to adjust preference.
        /// </summary>
        /// <param name="weight">Normalized value that determines the chance to return true.</param>
        /// <returns>A <see cref="bool"/> thats either true or false.</returns>
        public static bool RandomBoolWithWeight(float weight = 0) => weight >= RandomFloat();

        /// <summary>
        /// Generates a random <see cref="byte"/> value between min and max.
        /// </summary>
        /// <param name="min">The lower boundary.</param>
        /// <param name="max">The upper boundary.</param>
        /// <returns>A <see cref="byte"/> between min and max.</returns>
        public static byte RandomByte(byte min = 0, byte max = 1)
        {
            return (byte)_rand.Next(min, max+1);
        }

        /// <summary>
        /// Generates a random <see cref="byte"/> by using a single value as both upper and lower boundary.
        /// </summary>
        /// <param name="value">Defines both upper and lower boundary.</param>
        /// <returns>A that is <see cref="byte"/> between +value and -value.</returns>
        public static byte RandomUniformByte(byte value = 1)
        {
            int max = Math.Abs(value);
            int min = max * -1;

            return (byte)_rand.Next(min, max);
        }

        /// <summary>
        /// Generates a random <see cref="int"/> value between min and max.
        /// </summary>
        /// <param name="min">The lower boundary.</param>
        /// <param name="max">The upper boundary.</param>
        /// <returns>A random <see cref="int"/> between min and max.</returns>
        public static int RandomInt(int min = 0, int max = 1)
        {
            return _rand.Next(min, max+1);
        }

        /// <summary>
        /// Generates a random <see cref="int"/> by using a single value as both upper and lower boundary.
        /// </summary>
        /// <param name="value">Defines both upper and lower boundary.</param>
        /// <returns>A random <see cref="int"/> between +value and -value.</returns>
        public static int RandomUniformInt(int value = 1)
        {
            int max = Math.Abs(value);
            int min = max * -1;

            return _rand.Next(min, max);
        }

        /// <summary>
        /// Generates a random <see cref="float"/>  value between min and max.
        /// </summary>
        /// <param name="min">The lower boundary.</param>
        /// <param name="max">The upper boundary.</param>
        /// <returns>A random <see cref="float"/> between min and max.</returns>
        public static float RandomFloat(float min = 0.0f, float max = 1.0f)
        {
            return (float)_rand.NextDouble() * (max - min) + min;
        }

        /// <summary>
        /// Generates a random <see cref="float"/>  by using a single value as both upper and lower boundary.
        /// </summary>
        /// <param name="value">Defines both upper and lower boundary.</param>
        /// <returns>A random <see cref="float"/>  between +value and -value.</returns>
        public static float RandomUniformFloat(float value = 1.0f)
        {
            // Ensure that in case of negative values we still return a value between + value and - value.
            float max = Math.Abs(value);
            float min = max * -1;

            return (float)_rand.NextDouble() * (max - min) + min;
        }

        /// <summary>
        /// Generates a random <see cref="Quaternion"/>.
        /// </summary>
        /// <returns>A random <see cref="Quaternion"/>.</returns>
        public static Quaternion RandomQuaternion()
        {
            return Quaternion.Euler(
                RandomUniformFloat(180),
                RandomUniformFloat(180),
                RandomUniformFloat(180));
        }

        /// <summary>
        /// Generates a uniformly distributed random unit length vector point on a unit sphere.
        /// </summary>
        /// <returns>A random <see cref="Vector3"/>.</returns>
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

        /// <summary>
        /// Generates a uniformly distributed random unit length vector point on a unit sphere in 2D.
        /// </summary>
        /// <returns>A random <see cref="Vector2"./></returns>
        public static Vector2 RandomVector2()
        {
            Vector2 output;

            do
            {
                output.X = RandomFloat() * 2.0f - 1.0f;
                output.Y = RandomFloat() * 2.0f - 1.0f;

            } while (output.LengthSquared > 1 || output.LengthSquared< 1e-6f);

            output *= (1.0f / (float)Math.Sqrt(output.LengthSquared));

            return output;
        }

        /// <summary>
        /// Generates a random color.
        /// </summary>
        /// <param name="trueRandom">Should the color be generated from a single random Hue value or separate values for each channel.</param>
        /// <param name="randomAlpha">Randomize the alpha value.</param>
        /// <returns>A nice random color.</returns>
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
