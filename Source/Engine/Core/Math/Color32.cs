// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Runtime.InteropServices;

namespace FlaxEngine
{
    /// <summary>
    /// Representation of RGBA colors in 32 bit format.
    /// </summary>
    [Serializable]
    [StructLayout(LayoutKind.Sequential)]
    public struct Color32
    {
        /// <summary>
        /// The size of the <see cref="Color32" /> type, in bytes.
        /// </summary>
        public static readonly int SizeInBytes = Marshal.SizeOf(typeof(Color32));

        /// <summary>
        /// The transparent color.
        /// </summary>
        public static readonly Color32 Transparent = new Color32(0, 0, 0, 0);

        /// <summary>
        /// The black color.
        /// </summary>
        public static readonly Color32 Black = new Color32(0, 0, 0, 255);

        /// <summary>
        /// The white color.
        /// </summary>
        public static readonly Color32 White = new Color32(255, 255, 255, 255);

        /// <summary>
        /// Red component of the color.
        /// </summary>
        public byte R;

        /// <summary>
        /// Green component of the color.
        /// </summary>
        public byte G;

        /// <summary>
        /// Blue component of the color.
        /// </summary>
        public byte B;

        /// <summary>
        /// Alpha component of the color.
        /// </summary>
        public byte A;

        /// <summary>
        /// Gets or sets the component at the specified index.
        /// </summary>
        /// <value>The value of the red, green, blue, and alpha components, depending on the index.</value>
        /// <param name="index">The index of the component to access. Use 0 for the alpha component, 1 for the red component, 2 for the green component, and 3 for the blue component.</param>
        /// <returns>The value of the component at the specified index.</returns>
        /// <exception cref="System.IndexOutOfRangeException">Thrown when the <paramref name="index"/> is out of the range [0, 3].</exception>
        public byte this[int index]
        {
            get
            {
                switch (index)
                {
                case 0:
                {
                    return R;
                }
                case 1:
                {
                    return G;
                }
                case 2:
                {
                    return B;
                }
                case 3:
                {
                    return A;
                }
                }
                throw new IndexOutOfRangeException("Invalid Color32 index!");
            }
            set
            {
                switch (index)
                {
                case 0:
                {
                    R = value;
                    break;
                }
                case 1:
                {
                    G = value;
                    break;
                }
                case 2:
                {
                    B = value;
                    break;
                }
                case 3:
                {
                    A = value;
                    break;
                }
                default:
                {
                    throw new IndexOutOfRangeException("Invalid Color32 index!");
                }
                }
            }
        }

        /// <summary>
        /// Constructs a new Color32 with given r, g, b, a components.
        /// </summary>
        /// <param name="r">The red component value.</param>
        /// <param name="g">The green component value.</param>
        /// <param name="b">The blue component value.</param>
        /// <param name="a">The alpha component value.</param>
        public Color32(byte r, byte g, byte b, byte a)
        {
            R = r;
            G = g;
            B = b;
            A = a;
        }

        /// <summary>
        /// Linearly interpolates between colors a and b by t.
        /// </summary>
        /// <param name="a">Color a</param>
        /// <param name="b">Color b</param>
        /// <param name="t">Float for combining a and b</param>
        public static Color32 Lerp(Color32 a, Color32 b, float t)
        {
            return new Color32((byte)(a.R + (b.R - a.R) * t), (byte)(a.G + (b.G - a.G) * t), (byte)(a.B + (b.B - a.B) * t), (byte)(a.A + (b.A - a.A) * t));
        }

        /// <summary>
        /// Linearly interpolates between colors a and b by t.
        /// </summary>
        /// <param name="a">Color a</param>
        /// <param name="b">Color b</param>
        /// <param name="t">Float for combining a and b</param>
        /// <param name="result">Result</param>
        public static void Lerp(ref Color32 a, ref Color32 b, float t, out Color32 result)
        {
            result = new Color32((byte)(a.R + (b.R - a.R) * t), (byte)(a.G + (b.G - a.G) * t), (byte)(a.B + (b.B - a.B) * t), (byte)(a.A + (b.A - a.A) * t));
        }

        /// <summary>
        /// Adds two colors.
        /// </summary>
        /// <param name="a">The first color.</param>
        /// <param name="b">The second color.</param>
        /// <returns>The result of the operator.</returns>
        public static Color32 operator +(Color32 a, Color32 b)
        {
            return new Color32((byte)(a.R + b.R), (byte)(a.G + b.G), (byte)(a.B + b.B), (byte)(a.A + b.A));
        }

        /// <summary>
        /// Divides color by the scale factor.
        /// </summary>
        /// <param name="a">The first color.</param>
        /// <param name="b">The division factor.</param>
        /// <returns>The result of the operator.</returns>
        public static Color32 operator /(Color32 a, float b)
        {
            return new Color32((byte)(a.R / b), (byte)(a.G / b), (byte)(a.B / b), (byte)(a.A / b));
        }

        /// <summary>
        /// Multiplies color components by the other color components.
        /// </summary>
        /// <param name="a">The first color.</param>
        /// <param name="b">The second color.</param>
        /// <returns>The result of the operator.</returns>
        public static Color32 operator *(Color32 a, Color32 b)
        {
            return new Color32((byte)(a.R * b.R), (byte)(a.G * b.G), (byte)(a.B * b.B), (byte)(a.A * b.A));
        }

        /// <summary>
        /// Multiplies color components by the scale factor.
        /// </summary>
        /// <param name="a">The color.</param>
        /// <param name="b">The scale.</param>
        /// <returns>The result of the operator.</returns>
        public static Color32 operator *(Color32 a, float b)
        {
            return new Color32((byte)(a.R * b), (byte)(a.G * b), (byte)(a.B * b), (byte)(a.A * b));
        }

        /// <summary>
        /// Multiplies color components by the scale factor.
        /// </summary>
        /// <param name="b">The scale.</param>
        /// <param name="a">The color.</param>
        /// <returns>The result of the operator.</returns>
        public static Color32 operator *(float b, Color32 a)
        {
            return new Color32((byte)(a.R * b), (byte)(a.G * b), (byte)(a.B * b), (byte)(a.A * b));
        }

        /// <summary>
        /// Subtracts one color from the another.
        /// </summary>
        /// <param name="a">The first color.</param>
        /// <param name="b">The second color.</param>
        /// <returns>The result of the operator.</returns>
        public static Color32 operator -(Color32 a, Color32 b)
        {
            return new Color32((byte)(a.R - b.R), (byte)(a.G - b.G), (byte)(a.B - b.B), (byte)(a.A - b.A));
        }

        /// <summary>
        /// Performs an implicit conversion from <see cref="Color"/> to <see cref="Color32"/>.
        /// </summary>
        /// <param name="c">The color.</param>
        /// <returns>
        /// The result of the conversion.
        /// </returns>
        public static implicit operator Color32(Color c)
        {
            return new Color32((byte)(Mathf.Saturate(c.R) * 255f), (byte)(Mathf.Saturate(c.G) * 255f), (byte)(Mathf.Saturate(c.B) * 255f), (byte)(Mathf.Saturate(c.A) * 255f));
        }

        /// <summary>
        /// Performs an implicit conversion from <see cref="Color32"/> to <see cref="Color"/>.
        /// </summary>
        /// <param name="c">The color.</param>
        /// <returns>
        /// The result of the conversion.
        /// </returns>
        public static implicit operator Color(Color32 c)
        {
            return new Color(c.R / 255f, c.G / 255f, c.B / 255f, c.A / 255f);
        }

        /// <summary>
        /// Performs an explicit conversion from <see cref="Color32" /> to <see cref="Int4" />.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the conversion.</returns>
        public static explicit operator Int4(Color32 value)
        {
            return new Int4(value.R, value.G, value.B, value.A);
        }

        /// <summary>
        /// Performs an explicit conversion from <see cref="Color32"/> to <see cref="Vector4"/>.
        /// </summary>
        /// <param name="c">The color.</param>
        /// <returns>The result of the conversion.</returns>
        public static explicit operator Vector4(Color32 c)
        {
            return new Vector4(c.R / 255f, c.G / 255f, c.B / 255f, c.A / 255f);
        }

        /// <summary>
        /// Returns a nicely formatted string of this color.
        /// </summary>
        public override string ToString()
        {
            return string.Format("RGBA({0}, {1}, {2}, {3})", R, G, B, A);
        }

        /// <summary>
        /// Returns a nicely formatted string of this color.
        /// </summary>
        /// <param name="format"></param>
        public string ToString(string format)
        {
            return string.Format("RGBA({0}, {1}, {2}, {3})", R.ToString(format), G.ToString(format), B.ToString(format), A.ToString(format));
        }
    }
}
