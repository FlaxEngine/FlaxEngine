// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Globalization;
using System.Runtime.InteropServices;

namespace FlaxEngine
{
    /// <summary>
    /// Represents a color in the form of Hue, Saturation, Value, Alpha.
    /// </summary>
    [Serializable]
    [StructLayout(LayoutKind.Sequential, Pack = 4)]
    public struct ColorHSV : IEquatable<ColorHSV>, IFormattable
    {
        private const string ToStringFormat = "Hue:{0} Saturation:{1} Value:{2} Alpha:{3}";

        /// <summary>
        /// The Hue of the color.
        /// </summary>
        public float H;

        /// <summary>
        /// The Saturation of the color.
        /// </summary>
        public float S;

        /// <summary>
        /// The Value of the color.
        /// </summary>
        public float V;

        /// <summary>
        /// The alpha component of the color.
        /// </summary>
        public float A;

        /// <summary>
        /// Initializes a new instance of the <see cref="ColorHSV"/> struct.
        /// </summary>
        /// <param name="h">The hue.</param>
        /// <param name="s">The saturation.</param>
        /// <param name="v">The value.</param>
        /// <param name="a">The alpha.</param>
        public ColorHSV(float h, float s, float v, float a)
        {
            H = h;
            S = s;
            V = v;
            A = a;
        }

        /// <summary>
        /// Converts the color into a three component vector.
        /// </summary>
        /// <returns>A three component vector containing the red, green, and blue components of the color.</returns>
        public Color ToColor()
        {
            float hdiv = H / 60;
            int hi = (int)hdiv;
            float f = hdiv - hi;

            float p = V * (1 - S);
            float q = V * (1 - (S * f));
            float t = V * (1 - (S * (1 - f)));

            switch (hi)
            {
            case 0:
                return new Color(V, t, p, A);
            case 1:
                return new Color(q, V, p, A);
            case 2:
                return new Color(p, V, t, A);
            case 3:
                return new Color(p, q, V, A);
            case 4:
                return new Color(t, p, V, A);
            default:
                return new Color(V, p, q, A);
            }
        }

        /// <summary>
        /// Converts the color into a HSV color.
        /// </summary>
        /// <param name="color">The color.</param>
        /// <returns>A HSV color</returns>
        public static ColorHSV FromColor(Color color)
        {
            float max = Math.Max(color.R, Math.Max(color.G, color.B));
            float min = Math.Min(color.R, Math.Min(color.G, color.B));

            float delta = max - min;
            float h = 0.0f;

            if (delta > 0.0f)
            {
                if (color.R >= max)
                    h = (color.G - color.B) / delta;
                else if (color.G >= max)
                    h = (color.B - color.R) / delta + 2.0f;
                else
                    h = (color.R - color.G) / delta + 4.0f;
                h *= 60.0f;

                if (h < 0)
                    h += 360f;
            }

            float s = Mathf.IsZero(max) ? 0.0f : delta / max;

            return new ColorHSV(h, s, max, color.A);
        }

        /// <inheritdoc/>
        public bool Equals(ColorHSV other)
        {
            return other.H.Equals(H) && other.S.Equals(S) && other.V.Equals(V) && other.A.Equals(A);
        }

        /// <inheritdoc/>
        public override bool Equals(object obj)
        {
            if (ReferenceEquals(null, obj)) return false;
            return obj is ColorHSV && Equals((ColorHSV)obj);
        }

        /// <inheritdoc/>
        public override int GetHashCode()
        {
            unchecked
            {
                int result = H.GetHashCode();
                result = (result * 397) ^ S.GetHashCode();
                result = (result * 397) ^ V.GetHashCode();
                result = (result * 397) ^ A.GetHashCode();
                return result;
            }
        }

        /// <summary>
        /// Returns a <see cref="string"/> that represents this instance.
        /// </summary>
        /// <returns>A <see cref="string"/> that represents this instance.</returns>
        public override string ToString()
        {
            return ToString(CultureInfo.CurrentCulture);
        }

        /// <summary>
        /// Returns a <see cref="string"/> that represents this instance.
        /// </summary>
        /// <param name="format">The format.</param>
        /// <returns>A <see cref="string"/> that represents this instance.</returns>
        public string ToString(string format)
        {
            return ToString(format, CultureInfo.CurrentCulture);
        }

        /// <summary>
        /// Returns a <see cref="string"/> that represents this instance.
        /// </summary>
        /// <param name="formatProvider">The format provider.</param>
        /// <returns>A <see cref="string"/> that represents this instance.</returns>
        public string ToString(IFormatProvider formatProvider)
        {
            return string.Format(formatProvider, ToStringFormat, H, S, V, A);
        }

        /// <summary>
        /// Returns a <see cref="string"/> that represents this instance.
        /// </summary>
        /// <param name="format">The format.</param>
        /// <param name="formatProvider">The format provider.</param>
        /// <returns>A <see cref="string"/> that represents this instance.</returns>
        public string ToString(string format, IFormatProvider formatProvider)
        {
            if (format == null)
                return ToString(formatProvider);

            return string.Format(formatProvider, ToStringFormat,
                                 H.ToString(format, formatProvider),
                                 S.ToString(format, formatProvider),
                                 V.ToString(format, formatProvider),
                                 A.ToString(format, formatProvider));
        }
    }
}
