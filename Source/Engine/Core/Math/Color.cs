// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace FlaxEngine
{
    [Serializable]
#if FLAX_EDITOR
    [System.ComponentModel.TypeConverter(typeof(TypeConverters.ColorConverter))]
#endif
    partial struct Color
    {
        /// <summary>
        /// The size of the <see cref="Color" /> type, in bytes.
        /// </summary>
        public static readonly int SizeInBytes = Marshal.SizeOf(typeof(Color));

        /// <summary>
        /// Gets or sets the component at the specified index.
        /// </summary>
        /// <value>The value of the red, green, blue, and alpha components, depending on the index.</value>
        /// <param name="index">The index of the component to access. Use 0 for the alpha component, 1 for the red component, 2 for the green component, and 3 for the blue component.</param>
        /// <returns>The value of the component at the specified index.</returns>
        /// <exception cref="System.IndexOutOfRangeException">Thrown when the <paramref name="index"/> is out of the range [0, 3].</exception>
        public float this[int index]
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
                throw new IndexOutOfRangeException("Invalid Color index!");
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
                    throw new IndexOutOfRangeException("Invalid Color index!");
                }
                }
            }
        }

        /// <summary>
        /// Returns the minimum color component value: Min(r,g,b).
        /// </summary>
        public float MinColorComponent => Mathf.Min(Mathf.Min(R, G), B);

        /// <summary>
        /// Returns the maximum color component value: Max(r,g,b).
        /// </summary>
        public float MaxColorComponent => Mathf.Max(Mathf.Max(R, G), B);

        /// <summary>
        /// Gets a minimum component value (max of r,g,b,a).
        /// </summary>
        public float MinValue => Math.Min(R, Math.Min(G, Math.Min(B, A)));

        /// <summary>
        /// Gets a maximum component value (min of r,g,b,a).
        /// </summary>
        public float MaxValue => Math.Max(R, Math.Max(G, Math.Max(B, A)));

        /// <summary>
        /// Gets a sum of the component values.
        /// </summary>
        public float ValuesSum => R + G + B + A;

        /// <summary>
        /// Constructs a new Color with given r,g,b,a component.
        /// </summary>
        /// <param name="rgba">RGBA component.</param>
        public Color(float rgba)
        {
            R = rgba;
            G = rgba;
            B = rgba;
            A = rgba;
        }

        /// <summary>
        /// Constructs a new Color with given r,g,b,a components (values in range [0;1]).
        /// </summary>
        /// <param name="r">Red component.</param>
        /// <param name="g">Green component.</param>
        /// <param name="b">Blue component.</param>
        /// <param name="a">Alpha component.</param>
        public Color(float r, float g, float b, float a)
        {
            R = r;
            G = g;
            B = b;
            A = a;
        }

        /// <summary>
        /// Constructs a new Color with given r,g,b,a components (values in range [0;255]).
        /// </summary>
        /// <param name="r">Red component.</param>
        /// <param name="g">Green component.</param>
        /// <param name="b">Blue component.</param>
        /// <param name="a">Alpha component.</param>
        public Color(int r, int g, int b, int a = 255)
        {
            R = r / 255.0f;
            G = g / 255.0f;
            B = b / 255.0f;
            A = a / 255.0f;
        }

        /// <summary>
        /// Constructs a new Color with given r,g,b,a components (values in range [0;255]).
        /// </summary>
        /// <param name="r">Red component.</param>
        /// <param name="g">Green component.</param>
        /// <param name="b">Blue component.</param>
        /// <param name="a">Alpha component.</param>
        public Color(byte r, byte g, byte b, byte a = 255)
        {
            R = r / 255.0f;
            G = g / 255.0f;
            B = b / 255.0f;
            A = a / 255.0f;
        }

        /// <summary>
        /// Constructs a new Color with given r,g,b components and sets alpha to 1.
        /// </summary>
        /// <param name="r">Red component.</param>
        /// <param name="g">Green component.</param>
        /// <param name="b">Blue component.</param>
        public Color(float r, float g, float b)
        {
            R = r;
            G = g;
            B = b;
            A = 1f;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Color"/> struct.
        /// </summary>
        /// <param name="values">The values to assign to the red, green, blue, and alpha components of the color. This must be an array with four elements.</param>
        /// <exception cref="ArgumentNullException">Thrown when <paramref name="values"/> is <c>null</c>.</exception>
        /// <exception cref="ArgumentOutOfRangeException">Thrown when <paramref name="values"/> contains more or less than four elements.</exception>
        public Color(float[] values)
        {
            if (values == null)
                throw new ArgumentNullException(nameof(values));
            if (values.Length != 4)
                throw new ArgumentOutOfRangeException(nameof(values), "There must be four and only four input values for Color.");
            R = values[0];
            G = values[1];
            B = values[2];
            A = values[3];
        }

        /// <inheritdoc />
        public override bool Equals(object value)
        {
            return value is Color other && Equals(ref other);
        }

        /// <summary>
        /// Determines whether the specified <see cref="Color" /> is equal to this instance.
        /// </summary>
        /// <param name="other">The <see cref="Color" /> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="Color" /> is equal to this instance; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(ref Color other)
        {
            return R == other.R && G == other.G && B == other.B && A == other.A;
        }

        /// <inheritdoc />
        public override int GetHashCode()
        {
            unchecked
            {
                int hashCode = R.GetHashCode();
                hashCode = (hashCode * 397) ^ G.GetHashCode();
                hashCode = (hashCode * 397) ^ B.GetHashCode();
                hashCode = (hashCode * 397) ^ A.GetHashCode();
                return hashCode;
            }
        }

        /// <summary>
        /// Creates <see cref="Color"/> from the RGB value (bottom bits contain Blue) and separate alpha channel.
        /// </summary>
        /// <param name="rgb">The packed RGB value (bottom bits contain Blue).</param>
        /// <param name="a">The alpha channel value.</param>
        /// <returns>The color.</returns>
        public static Color FromRGB(uint rgb, float a = 1.0f)
        {
            return new Color(((rgb >> 16) & 0xff) / 255.0f, ((rgb >> 8) & 0xff) / 255.0f, (rgb & 0xff) / 255.0f, a);
        }

        /// <summary>
        /// Creates <see cref="Color"/> from the ARGB value (bottom bits contain Blue).
        /// </summary>
        /// <param name="argb">The packed ARGB value (bottom bits contain Blue).</param>
        /// <returns>The color.</returns>
        public static Color FromARGB(uint argb)
        {
            return new Color(((argb >> 16) & 0xff) / 255.0f, ((argb >> 8) & 0xff) / 255.0f, ((argb >> 0) & 0xff) / 255.0f, ((argb >> 24) & 0xff) / 255.0f);
        }

        /// <summary>
        /// Creates <see cref="Color"/> from the RGBA value (bottom bits contain Alpha).
        /// </summary>
        /// <param name="rgba">The packed RGBA value (bottom bits Alpha Red).</param>
        /// <returns>The color.</returns>
        public static Color FromRGBA(uint rgba)
        {
            return new Color(((rgba >> 24) & 0xff) / 255.0f, ((rgba >> 16) & 0xff) / 255.0f, ((rgba >> 8) & 0xff) / 255.0f, ((rgba >> 0) & 0xff) / 255.0f);
        }

        /// <summary>
        /// Creates <see cref="Color"/> from the Hex string.
        /// </summary>
        /// <param name="hex">The hexadecimal color string.</param>
        /// <returns>The output color value.</returns>
        public static Color FromHex(string hex)
        {
            FromHex(hex, out var color);
            return color;
        }

        /// <summary>
        /// Creates <see cref="Color"/> from the Hex string.
        /// </summary>
        /// <param name="hex">The hexadecimal color string.</param>
        /// <param name="color">The output color value. Valid if method returns true.</param>
        /// <returns>True if method was able to convert color, otherwise false.</returns>
        public static bool FromHex(string hex, out Color color)
        {
            int r, g, b, a = 255;
            bool isValid = true;

            int startIndex = hex.Length != 0 && hex[0] == '#' ? 1 : 0;
            if (hex.Length == 3 + startIndex)
            {
                r = StringUtils.HexDigit(hex[startIndex++]);
                g = StringUtils.HexDigit(hex[startIndex++]);
                b = StringUtils.HexDigit(hex[startIndex]);
                r = (r << 4) + r;
                g = (g << 4) + g;
                b = (b << 4) + b;
            }
            else if (hex.Length == 6 + startIndex)
            {
                r = (StringUtils.HexDigit(hex[startIndex + 0]) << 4) + StringUtils.HexDigit(hex[startIndex + 1]);
                g = (StringUtils.HexDigit(hex[startIndex + 2]) << 4) + StringUtils.HexDigit(hex[startIndex + 3]);
                b = (StringUtils.HexDigit(hex[startIndex + 4]) << 4) + StringUtils.HexDigit(hex[startIndex + 5]);
            }
            else if (hex.Length == 8 + startIndex)
            {
                r = (StringUtils.HexDigit(hex[startIndex + 0]) << 4) + StringUtils.HexDigit(hex[startIndex + 1]);
                g = (StringUtils.HexDigit(hex[startIndex + 2]) << 4) + StringUtils.HexDigit(hex[startIndex + 3]);
                b = (StringUtils.HexDigit(hex[startIndex + 4]) << 4) + StringUtils.HexDigit(hex[startIndex + 5]);
                a = (StringUtils.HexDigit(hex[startIndex + 6]) << 4) + StringUtils.HexDigit(hex[startIndex + 7]);
            }
            else
            {
                r = g = b = 0;
                isValid = false;
            }

            color = new Color(r, g, b, a);
            return isValid;
        }

        /// <summary>
        /// Gets the color value as the hexadecimal string (in RGBA order).
        /// </summary>
        /// <returns>Hex string.</returns>
        public string ToHexString()
        {
            const string digits = "0123456789ABCDEF";

            byte r = (byte)(R * 255);
            byte g = (byte)(G * 255);
            byte b = (byte)(B * 255);
            byte a = (byte)(A * 255);

            var result = new char[8];

            result[0] = digits[(r >> 4) & 0x0f];
            result[1] = digits[(r) & 0x0f];

            result[2] = digits[(g >> 4) & 0x0f];
            result[3] = digits[(g) & 0x0f];

            result[4] = digits[(b >> 4) & 0x0f];
            result[5] = digits[(b) & 0x0f];

            result[6] = digits[(a >> 4) & 0x0f];
            result[7] = digits[(a) & 0x0f];

            return new string(result);
        }

        /// <summary>
        /// Creates <see cref="Color"/> from the text string (hex or color name).
        /// </summary>
        /// <param name="text">The color string (hex or color name).</param>
        /// <returns>The color.</returns>
        public static Color Parse(string text)
        {
            if (TryParse(text, out Color value))
                return value;
            throw new FormatException();
        }

        /// <summary>
        /// Creates <see cref="Color"/> from the string (hex or color name).
        /// </summary>
        /// <param name="text">The color string (hex or color name).</param>
        /// <param name="value">Output value.</param>
        /// <returns>True if value has been parsed, otherwise false.</returns>
        public static bool TryParse(string text, out Color value)
        {
            // Try hexadecimal
            if (text.StartsWith("#") && TryParseHex(text, out value))
                return true;

            // Try named color
            if (text.Length > 2)
            {
                var fieldName = char.ToUpperInvariant(text[0]) + text.Substring(1).ToLowerInvariant();
                var field = typeof(Color).GetField(fieldName, BindingFlags.Public | BindingFlags.Static);
                if (field != null && fieldName != "Zero")
                {
                    value = (Color)field.GetValue(null);
                    return true;
                }
            }

            value = Black;
            return false;
        }

        /// <summary>
        /// Creates <see cref="Color"/> from the hexadecimal string.
        /// </summary>
        /// <param name="hexString">The hexadecimal string.</param>
        /// <returns>The color.</returns>
        public static Color ParseHex(string hexString)
        {
            if (TryParseHex(hexString, out Color value))
            {
                return value;
            }
            throw new FormatException();
        }

        /// <summary>
        /// Creates <see cref="Color"/> from the hexadecimal string.
        /// </summary>
        /// <param name="hexString">The hexadecimal string.</param>
        /// <param name="value">Output value.</param>
        /// <returns>True if value has been parsed, otherwise false.</returns>
        public static bool TryParseHex(string hexString, out Color value)
        {
            value = Black;
            if (string.IsNullOrEmpty(hexString))
                return false;

            int r, g, b, a = 255;
            int startIndex = hexString[0] == '#' ? 1 : 0;
            if (hexString.Length == 6 + startIndex)
            {
                r = (StringUtils.HexDigit(hexString[startIndex + 0]) << 4) + StringUtils.HexDigit(hexString[startIndex + 1]);
                g = (StringUtils.HexDigit(hexString[startIndex + 2]) << 4) + StringUtils.HexDigit(hexString[startIndex + 3]);
                b = (StringUtils.HexDigit(hexString[startIndex + 4]) << 4) + StringUtils.HexDigit(hexString[startIndex + 5]);
            }
            else if (hexString.Length == 8 + startIndex)
            {
                r = (StringUtils.HexDigit(hexString[startIndex + 0]) << 4) + StringUtils.HexDigit(hexString[startIndex + 1]);
                g = (StringUtils.HexDigit(hexString[startIndex + 2]) << 4) + StringUtils.HexDigit(hexString[startIndex + 3]);
                b = (StringUtils.HexDigit(hexString[startIndex + 4]) << 4) + StringUtils.HexDigit(hexString[startIndex + 5]);
                a = (StringUtils.HexDigit(hexString[startIndex + 6]) << 4) + StringUtils.HexDigit(hexString[startIndex + 7]);
            }
            else
            {
                return false;
            }

            value = new Color(r, g, b, a);
            return true;
        }

        /// <summary>
        /// Converts the color from a packed BGRA integer.
        /// </summary>
        /// <param name="color">A packed integer containing all four color components in BGRA order</param>
        /// <returns>A color.</returns>
        public static Color FromBgra(int color)
        {
            return new Color((byte)((color >> 16) & 255), (byte)((color >> 8) & 255), (byte)(color & 255), (byte)((color >> 24) & 255));
        }

        /// <summary>
        /// Converts the color from a packed BGRA integer.
        /// </summary>
        /// <param name="color">A packed integer containing all four color components in BGRA order</param>
        /// <returns>A color.</returns>
        public static Color FromBgra(uint color)
        {
            return FromBgra(unchecked((int)color));
        }

        /// <summary>
        /// Converts the color into a packed integer.
        /// </summary>
        /// <returns>A packed integer containing all four color components.</returns>
        public int ToBgra()
        {
            uint a = (uint)(A * 255.0f) & 255;
            uint r = (uint)(R * 255.0f) & 255;
            uint g = (uint)(G * 255.0f) & 255;
            uint b = (uint)(B * 255.0f) & 255;

            uint value = b;
            value |= g << 8;
            value |= r << 16;
            value |= a << 24;

            return (int)value;
        }

        /// <summary>
        /// Converts the color into a packed integer.
        /// </summary>
        /// <returns>A packed integer containing all four color components.</returns>
        public void ToBgra(out byte r, out byte g, out byte b, out byte a)
        {
            a = (byte)(A * 255.0f);
            r = (byte)(R * 255.0f);
            g = (byte)(G * 255.0f);
            b = (byte)(B * 255.0f);
        }

        /// <summary>
        /// Converts the color into a packed integer.
        /// </summary>
        /// <returns>A packed integer containing all four color components.</returns>
        public int ToRgba()
        {
            uint a = (uint)(A * 255.0f) & 255;
            uint r = (uint)(R * 255.0f) & 255;
            uint g = (uint)(G * 255.0f) & 255;
            uint b = (uint)(B * 255.0f) & 255;

            uint value = r;
            value |= g << 8;
            value |= b << 16;
            value |= a << 24;

            return (int)value;
        }

        /// <summary>
        /// Creates an array containing the elements of the color.
        /// </summary>
        /// <returns>A four-element array containing the components of the color.</returns>
        public float[] ToArray()
        {
            return new[] { R, G, B, A };
        }

        /// <summary>
        /// Converts this color from linear space to sRGB space.
        /// </summary>
        /// <returns>A color3 in sRGB space.</returns>
        public Color ToSRgb()
        {
            return new Color(Mathf.LinearToSRgb(R), Mathf.LinearToSRgb(G), Mathf.LinearToSRgb(B), A);
        }

        /// <summary>
        /// Converts this color from sRGB space to linear space.
        /// </summary>
        /// <returns>A Color in linear space.</returns>
        public Color ToLinear()
        {
            return new Color(Mathf.SRgbToLinear(R), Mathf.SRgbToLinear(G), Mathf.SRgbToLinear(B), A);
        }

        static readonly int[][] RGBSwizzle =
        {
            new[]
            {
                0,
                3,
                1
            },
            new[]
            {
                2,
                0,
                1
            },
            new[]
            {
                1,
                0,
                3
            },
            new[]
            {
                1,
                2,
                0
            },
            new[]
            {
                3,
                1,
                0
            },
            new[]
            {
                0,
                1,
                2
            },
        };

        /// <summary>
        /// Creates RGB color from Hue[0-360], Saturation[0-1] and Value[0-1].
        /// </summary>
        /// <param name="hue">The hue angle in degrees [0-360].</param>
        /// <param name="saturation">The saturation normalized [0-1].</param>
        /// <param name="value">The value normalized [0-1].</param>
        /// <param name="alpha">The alpha value. Default is 1.</param>
        /// <returns>The RGB color.</returns>
        public static Color FromHSV(float hue, float saturation, float value, float alpha = 1.0f)
        {
            float hDiv60 = hue / 60.0f;
            float hDiv60Floor = (float)Math.Floor(hDiv60);
            float hDiv60Fraction = hDiv60 - hDiv60Floor;

            float[] rgbValues =
            {
                value,
                value * (1.0f - saturation),
                value * (1.0f - (hDiv60Fraction * saturation)),
                value * (1.0f - ((1.0f - hDiv60Fraction) * saturation)),
            };

            int swizzleIndex = ((int)hDiv60Floor) % 6;

            return new Color(rgbValues[RGBSwizzle[swizzleIndex][0]],
                             rgbValues[RGBSwizzle[swizzleIndex][1]],
                             rgbValues[RGBSwizzle[swizzleIndex][2]],
                             alpha);
        }

        /// <summary>
        /// Creates RGB color from Hue[0-360], Saturation[0-1] and Value[0-1] packed to XYZ vector.
        /// </summary>
        /// <param name="hsv">The HSV color.</param>
        /// <param name="alpha">The alpha value. Default is 1.</param>
        /// <returns>The RGB color.</returns>
        public static Color FromHSV(Float3 hsv, float alpha = 1.0f)
        {
            return FromHSV(hsv.X, hsv.Y, hsv.Z, alpha);
        }

        /// <summary>
        /// Linearly interpolates between colors a and b by t.
        /// </summary>
        /// <param name="a">Color a</param>
        /// <param name="b">Color b</param>
        /// <param name="t">Float for combining a and b</param>
        public static Color Lerp(Color a, Color b, float t)
        {
            return new Color(a.R + (b.R - a.R) * t, a.G + (b.G - a.G) * t, a.B + (b.B - a.B) * t, a.A + (b.A - a.A) * t);
        }

        /// <summary>
        /// Linearly interpolates between colors a and b by t.
        /// </summary>
        /// <param name="a">Color a</param>
        /// <param name="b">Color b</param>
        /// <param name="t">Float for combining a and b</param>
        /// <param name="result">The result.</param>
        public static void Lerp(ref Color a, ref Color b, float t, out Color result)
        {
            result = new Color(a.R + (b.R - a.R) * t, a.G + (b.G - a.G) * t, a.B + (b.B - a.B) * t, a.A + (b.A - a.A) * t);
        }

        /// <summary>
        /// Adds two colors.
        /// </summary>
        /// <param name="a">The first color.</param>
        /// <param name="b">The second color.</param>
        /// <returns>The result of the operator.</returns>
        public static Color operator +(Color a, Color b)
        {
            return new Color(a.R + b.R, a.G + b.G, a.B + b.B, a.A + b.A);
        }

        /// <summary>
        /// Divides color by the scale factor.
        /// </summary>
        /// <param name="a">The first color.</param>
        /// <param name="b">The division factor.</param>
        /// <returns>The result of the operator.</returns>
        public static Color operator /(Color a, float b)
        {
            return new Color(a.R / b, a.G / b, a.B / b, a.A / b);
        }

        /// <summary>
        /// Compares two colors.
        /// </summary>
        /// <param name="left">The left.</param>
        /// <param name="right">The right.</param>
        /// <returns>True if colors are equal, otherwise false.</returns>
        public static bool operator ==(Color left, Color right)
        {
            return left.Equals(ref right);
        }

        /// <summary>
        /// Compares two colors.
        /// </summary>
        /// <param name="left">The left.</param>
        /// <param name="right">The right.</param>
        /// <returns>True if colors are not equal, otherwise false.</returns>
        public static bool operator !=(Color left, Color right)
        {
            return !left.Equals(ref right);
        }

        /// <summary>
        /// Performs an implicit conversion from <see cref="Color"/> to <see cref="Float3"/>.
        /// </summary>
        /// <param name="c">The color.</param>
        /// <returns>The result of the conversion.</returns>
        public static implicit operator Float3(Color c)
        {
            return new Vector3(c.R, c.G, c.B);
        }

        /// <summary>
        /// Performs an implicit conversion from <see cref="Color"/> to <see cref="Float4"/>.
        /// </summary>
        /// <param name="c">The color.</param>
        /// <returns>The result of the conversion.</returns>
        public static implicit operator Float4(Color c)
        {
            return new Float4(c.R, c.G, c.B, c.A);
        }

        /// <summary>
        /// Performs an implicit conversion from <see cref="Float4"/> to <see cref="Color"/>.
        /// </summary>
        /// <param name="v">The vector.</param>
        /// <returns>The result of the conversion.</returns>
        public static implicit operator Color(Float4 v)
        {
            return new Color(v.X, v.Y, v.Z, v.W);
        }

        /// <summary>
        /// Performs an implicit conversion from <see cref="Float3"/> to <see cref="Color"/>.
        /// </summary>
        /// <param name="v">The vector.</param>
        /// <returns>The result of the conversion.</returns>
        public static implicit operator Color(Float3 v)
        {
            return new Color(v.X, v.Y, v.Z);
        }

        /// <summary>
        /// Performs an implicit conversion from <see cref="Color"/> to <see cref="Vector3"/>.
        /// </summary>
        /// <param name="c">The color.</param>
        /// <returns>The result of the conversion.</returns>
        public static implicit operator Vector3(Color c)
        {
            return new Vector3(c.R, c.G, c.B);
        }

        /// <summary>
        /// Performs an implicit conversion from <see cref="Color"/> to <see cref="Vector4"/>.
        /// </summary>
        /// <param name="c">The color.</param>
        /// <returns>The result of the conversion.</returns>
        public static implicit operator Vector4(Color c)
        {
            return new Vector4(c.R, c.G, c.B, c.A);
        }

        /// <summary>
        /// Performs an implicit conversion from <see cref="Vector4"/> to <see cref="Color"/>.
        /// </summary>
        /// <param name="v">The vector.</param>
        /// <returns>The result of the conversion.</returns>
        public static implicit operator Color(Vector4 v)
        {
            return new Color((float)v.X, (float)v.Y, (float)v.Z, (float)v.W);
        }

        /// <summary>
        /// Performs an implicit conversion from <see cref="Vector3"/> to <see cref="Color"/>.
        /// </summary>
        /// <param name="v">The vector.</param>
        /// <returns>The result of the conversion.</returns>
        public static implicit operator Color(Vector3 v)
        {
            return new Color((float)v.X, (float)v.Y, (float)v.Z);
        }

        /// <summary>
        /// Multiplies color components by the other color components.
        /// </summary>
        /// <param name="a">The first color.</param>
        /// <param name="b">The second color.</param>
        /// <returns>The result of the operator.</returns>
        public static Color operator *(Color a, Color b)
        {
            return new Color(a.R * b.R, a.G * b.G, a.B * b.B, a.A * b.A);
        }

        /// <summary>
        /// Multiplies color components by the scale factor.
        /// </summary>
        /// <param name="a">The color.</param>
        /// <param name="b">The scale.</param>
        /// <returns>The result of the operator.</returns>
        public static Color operator *(Color a, float b)
        {
            return new Color(a.R * b, a.G * b, a.B * b, a.A * b);
        }

        /// <summary>
        /// Multiplies color components by the scale factor.
        /// </summary>
        /// <param name="b">The scale.</param>
        /// <param name="a">The color.</param>
        /// <returns>The result of the operator.</returns>
        public static Color operator *(float b, Color a)
        {
            return new Color(a.R * b, a.G * b, a.B * b, a.A * b);
        }

        /// <summary>
        /// Subtracts one color from the another.
        /// </summary>
        /// <param name="a">The first color.</param>
        /// <param name="b">The second color.</param>
        /// <returns>The result of the operator.</returns>
        public static Color operator -(Color a, Color b)
        {
            return new Color(a.R - b.R, a.G - b.G, a.B - b.B, a.A - b.A);
        }

        /// <summary>
        /// Returns the color with RGB channels multiplied by the given scale factor. The alpha channel remains the same.
        /// </summary>
        /// <param name="multiplier">The multiplier.</param>
        /// <returns>The modified color.</returns>
        public Color RGBMultiplied(float multiplier)
        {
            return new Color(R * multiplier, G * multiplier, B * multiplier, A);
        }

        /// <summary>
        /// Returns the color with RGB channels multiplied by the given color. The alpha channel remains the same.
        /// </summary>
        /// <param name="multiplier">The multiplier.</param>
        /// <returns>The modified color.</returns>
        public Color RGBMultiplied(Color multiplier)
        {
            return new Color(R * multiplier.R, G * multiplier.G, B * multiplier.B, A);
        }

        /// <summary>
        /// Returns the color with alpha channel multiplied by the given color. The RGB channels remain the same.
        /// </summary>
        /// <param name="multiplier">The multiplier.</param>
        /// <returns>The modified color.</returns>
        public Color AlphaMultiplied(float multiplier)
        {
            return new Color(R, G, B, A * multiplier);
        }

        /// <summary>
        /// Gets Hue[0-360], Saturation[0-1] and Value[0-1] from RGB color.
        /// </summary>
        /// <returns>The HSV color.</returns>
        public Float3 ToHSV()
        {
            float rgbMin = Mathf.Min(R, G, B);
            float rgbMax = Mathf.Max(R, G, B);
            float rgbRange = rgbMax - rgbMin;

            float hue;
            if (Mathf.NearEqual(rgbMax, rgbMin))
                hue = 0.0f;
            else if (Mathf.NearEqual(rgbMax, R))
                hue = ((((G - B) / rgbRange) * 60.0f) + 360.0f) % 360.0f;
            else if (Mathf.NearEqual(rgbMax, G))
                hue = (((B - R) / rgbRange) * 60.0f) + 120.0f;
            else if (Mathf.NearEqual(rgbMax, B))
                hue = (((R - G) / rgbRange) * 60.0f) + 240.0f;
            else
                hue = 0.0f;

            float saturation = Mathf.IsZero(rgbMax) ? 0.0f : rgbRange / rgbMax;
            float value = rgbMax;

            return new Float3(hue, saturation, value);
        }

        /// <summary>
        /// Convert color from the RGB color space to HSV color space.
        /// </summary>
        /// <param name="rgbColor">Color of the RGB.</param>
        /// <param name="h">The output Hue.</param>
        /// <param name="s">The output Saturation.</param>
        /// <param name="v">The output Value.</param>
        public static void RGBToHSV(Color rgbColor, out float h, out float s, out float v)
        {
            if ((rgbColor.B > rgbColor.G) && (rgbColor.B > rgbColor.R))
                RGBToHSVHelper(4f, rgbColor.B, rgbColor.R, rgbColor.G, out h, out s, out v);
            else if (rgbColor.G <= rgbColor.R)
                RGBToHSVHelper(0f, rgbColor.R, rgbColor.G, rgbColor.B, out h, out s, out v);
            else
                RGBToHSVHelper(2f, rgbColor.G, rgbColor.B, rgbColor.R, out h, out s, out v);
        }

        private static void RGBToHSVHelper(float offset, float dominantcolor, float colorone, float colortwo, out float h, out float s, out float v)
        {
            v = dominantcolor;
            if (Mathf.IsZero(v))
            {
                s = 0f;
                h = 0f;
            }
            else
            {
                var single = colorone <= colortwo ? colorone : colortwo;
                float vv = v - single;
                if (Mathf.IsZero(vv))
                {
                    s = 0f;
                    h = offset + (colorone - colortwo);
                }
                else
                {
                    s = vv / v;
                    h = offset + (colorone - colortwo) / vv;
                }
                h /= 6f;
                if (h < 0f)
                    h++;
            }
        }

        /// <summary>1
        /// Adjusts the contrast of a color.
        /// </summary>
        /// <param name="value">The color whose contrast is to be adjusted.</param>
        /// <param name="contrast">The amount by which to adjust the contrast.</param>
        /// <param name="result">When the method completes, contains the adjusted color.</param>
        public static void AdjustContrast(ref Color value, float contrast, out Color result)
        {
            result.A = value.A;
            result.R = 0.5f + contrast * (value.R - 0.5f);
            result.G = 0.5f + contrast * (value.G - 0.5f);
            result.B = 0.5f + contrast * (value.B - 0.5f);
        }

        /// <summary>
        /// Adjusts the contrast of a color.
        /// </summary>
        /// <param name="value">The color whose contrast is to be adjusted.</param>
        /// <param name="contrast">The amount by which to adjust the contrast.</param>
        /// <returns>The adjusted color.</returns>
        public static Color AdjustContrast(Color value, float contrast)
        {
            return new Color(
                             0.5f + contrast * (value.R - 0.5f),
                             0.5f + contrast * (value.G - 0.5f),
                             0.5f + contrast * (value.B - 0.5f),
                             value.A);
        }

        /// <summary>
        /// Adjusts the saturation of a color.
        /// </summary>
        /// <param name="value">The color whose saturation is to be adjusted.</param>
        /// <param name="saturation">The amount by which to adjust the saturation.</param>
        /// <param name="result">When the method completes, contains the adjusted color.</param>
        public static void AdjustSaturation(ref Color value, float saturation, out Color result)
        {
            float grey = value.R * 0.2125f + value.G * 0.7154f + value.B * 0.0721f;

            result.A = value.A;
            result.R = grey + saturation * (value.R - grey);
            result.G = grey + saturation * (value.G - grey);
            result.B = grey + saturation * (value.B - grey);
        }

        /// <summary>
        /// Adjusts the saturation of a color.
        /// </summary>
        /// <param name="value">The color whose saturation is to be adjusted.</param>
        /// <param name="saturation">The amount by which to adjust the saturation.</param>
        /// <returns>The adjusted color.</returns>
        public static Color AdjustSaturation(Color value, float saturation)
        {
            float grey = value.R * 0.2125f + value.G * 0.7154f + value.B * 0.0721f;

            return new Color(
                             grey + saturation * (value.R - grey),
                             grey + saturation * (value.G - grey),
                             grey + saturation * (value.B - grey),
                             value.A);
        }

        /// <summary>
        /// Premultiplies the color components by the alpha value.
        /// </summary>
        /// <param name="value">The color to premultiply.</param>
        /// <returns>A color with premultiplied alpha.</returns>
        public static Color PremultiplyAlpha(Color value)
        {
            return new Color(value.R * value.A, value.G * value.A, value.B * value.A, value.A);
        }

        /// <summary>
        /// Returns a color containing the largest components of the specified colors.
        /// </summary>
        /// <param name="left">The first source color.</param>
        /// <param name="right">The second source color.</param>
        /// <param name="result">When the method completes, contains an new color composed of the largest components of the source colors.</param>
        public static void Max(ref Color left, ref Color right, out Color result)
        {
            result = new Color(
                               Mathf.Max(left.R, right.R),
                               Mathf.Max(left.G, right.G),
                               Mathf.Max(left.B, right.B),
                               Mathf.Max(left.A, right.A)
                              );
        }

        /// <summary>
        /// Returns a color containing the largest components of the specified colors.
        /// </summary>
        /// <param name="left">The first source color.</param>
        /// <param name="right">The second source color.</param>
        /// <returns>A color containing the largest components of the source colors.</returns>
        public static Color Max(Color left, Color right)
        {
            Max(ref left, ref right, out var result);
            return result;
        }

        /// <summary>
        /// Returns a color containing the smallest components of the specified colors.
        /// </summary>
        /// <param name="left">The first source color.</param>
        /// <param name="right">The second source color.</param>
        /// <param name="result">When the method completes, contains an new color composed of the smallest components of the source colors.</param>
        public static void Min(ref Color left, ref Color right, out Color result)
        {
            result = new Color(
                               Mathf.Min(left.R, right.R),
                               Mathf.Min(left.G, right.G),
                               Mathf.Min(left.B, right.B),
                               Mathf.Min(left.A, right.A)
                              );
        }

        /// <summary>
        /// Returns a color containing the smallest components of the specified colors.
        /// </summary>
        /// <param name="left">The first source color.</param>
        /// <param name="right">The second source color.</param>
        /// <returns>A color containing the smallest components of the source colors.</returns>
        public static Color Min(Color left, Color right)
        {
            Min(ref left, ref right, out var result);
            return result;
        }

        /// <summary>
        /// Restricts a value to be within a specified range.
        /// </summary>
        /// <param name="value">The value to clamp.</param>
        /// <param name="min">The minimum value.</param>
        /// <param name="max">The maximum value.</param>
        /// <param name="result">When the method completes, contains the clamped value.</param>
        public static void Clamp(ref Color value, ref Color min, ref Color max, out Color result)
        {
            result = new Color(
                               Mathf.Clamp(value.R, min.R, max.R),
                               Mathf.Clamp(value.G, min.G, max.G),
                               Mathf.Clamp(value.B, min.B, max.B),
                               Mathf.Clamp(value.A, min.A, max.A)
                              );
        }

        /// <summary>
        /// Restricts a value to be within a specified range.
        /// </summary>
        /// <param name="value">The value to clamp.</param>
        /// <param name="min">The minimum value.</param>
        /// <param name="max">The maximum value.</param>
        /// <returns>The clamped value.</returns>
        public static Color Clamp(Color value, Color min, Color max)
        {
            Clamp(ref value, ref min, ref max, out Color result);
            return result;
        }

        /// <summary>
        /// Returns a nicely formatted string of this color.
        /// </summary>
        public override string ToString()
        {
            return $"RGBA({R:F3}, {G:F3}, {B:F3}, {A:F3})";
        }

        /// <summary>
        /// Returns a nicely formatted string of this color.
        /// </summary>
        /// <param name="format"></param>
        public string ToString(string format)
        {
            return $"RGBA({R.ToString(format)}, {G.ToString(format)}, {B.ToString(format)}, {A.ToString(format)})";
        }
    }
}
