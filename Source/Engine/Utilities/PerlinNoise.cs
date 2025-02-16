// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine.Utilities
{
    /// <summary>
    /// Helper class for Perlin Noise generation.
    /// </summary>
    [HideInEditor]
    public class PerlinNoise
    {
        /// <summary>
        /// The base value.
        /// </summary>
        public float Base = 0.0f;

        /// <summary>
        /// The noise scale parameter.
        /// </summary>
        public float NoiseScale = 1.0f;

        /// <summary>
        /// The noise amount parameter.
        /// </summary>
        public float NoiseAmount = 1.0f;

        /// <summary>
        /// The noise octaves count.
        /// </summary>
        public int Octaves = 4;

        /// <summary>
        /// Initializes a new instance of the <see cref="PerlinNoise"/> class.
        /// </summary>
        public PerlinNoise()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="PerlinNoise"/> class.
        /// </summary>
        /// <param name="baseValue">The base value.</param>
        /// <param name="scale">The noise scale.</param>
        /// <param name="amount">The noise amount.</param>
        /// <param name="octaves">The noise octaves count.</param>
        public PerlinNoise(float baseValue, float scale, float amount, int octaves = 4)
        {
            Base = baseValue;
            NoiseScale = scale;
            NoiseAmount = amount;
            Octaves = octaves;
        }

        /// <summary>
        /// Samples the Perlin Noise at the given location (integer coordinates).
        /// </summary>
        /// <param name="x">The x coordinate.</param>
        /// <param name="y">The y coordinate.</param>
        /// <returns>The noise value.</returns>
        public float Sample(float x, float y)
        {
            float noise = 0.0f;
            if (NoiseScale > Mathf.Epsilon)
            {
                x = Mathf.Abs(x);
                y = Mathf.Abs(y);
                for (int octave = 0; octave < Octaves; octave++)
                {
                    float octaveShift = 1 << octave;
                    float octaveScale = octaveShift / NoiseScale;
                    noise += PerlinNoise2D(x * octaveScale, y * octaveScale) / octaveShift;
                }
            }
            return Base + noise * NoiseAmount;
        }

        private float Fade(float t)
        {
            return t * t * t * (t * (t * 6 - 15) + 10);
        }

        private float Grad(int hash, float x, float y)
        {
            int h = hash & 15;
            float u = h < 8 || h == 12 || h == 13 ? x : y,
                  v = h < 4 || h == 12 || h == 13 ? y : 0;
            return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
        }

        private float PerlinNoise2D(float x, float y)
        {
            int truncX = Mathf.FloorToInt(x),
                truncY = Mathf.FloorToInt(y),
                intX = truncX & 255,
                intY = truncY & 255;
            float fracX = x - truncX,
                  fracY = y - truncY;

            float u = Fade(fracX),
                  v = Fade(fracY);

            int a = Permutations[intX] + intY,
                aa = Permutations[a & 255],
                ab = Permutations[(a + 1) & 255],
                b = Permutations[(intX + 1) & 255] + intY,
                ba = Permutations[b & 255],
                bb = Permutations[(b + 1) & 255];

            return Mathf.Lerp(Mathf.Lerp(Grad(Permutations[aa], fracX, fracY),
                                         Grad(Permutations[ba], fracX - 1, fracY), u),
                              Mathf.Lerp(Grad(Permutations[ab], fracX, fracY - 1),
                                         Grad(Permutations[bb], fracX - 1, fracY - 1), u), v);
        }

        private static readonly int[] Permutations =
        {
            151, 160, 137, 91, 90, 15,
            131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23,
            190, 6, 148, 247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32, 57, 177, 33,
            88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175, 74, 165, 71, 134, 139, 48, 27, 166,
            77, 146, 158, 231, 83, 111, 229, 122, 60, 211, 133, 230, 220, 105, 92, 41, 55, 46, 245, 40, 244,
            102, 143, 54, 65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89, 18, 169, 200, 196,
            135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226, 250, 124, 123,
            5, 202, 38, 147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47, 16, 58, 17, 182, 189, 28, 42,
            223, 183, 170, 213, 119, 248, 152, 2, 44, 154, 163, 70, 221, 153, 101, 155, 167, 43, 172, 9,
            129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104, 218, 246, 97, 228,
            251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241, 81, 51, 145, 235, 249, 14, 239, 107,
            49, 192, 214, 31, 181, 199, 106, 157, 184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150, 254,
            138, 236, 205, 93, 222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180
        };
    }
}
