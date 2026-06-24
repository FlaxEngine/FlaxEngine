// Copyright (c) Wojciech Figat. All rights reserved.

using System.Runtime.CompilerServices;

namespace FlaxEngine
{
    /// <summary>
    /// Font reference that defines the font asset and font size to use.
    /// </summary>
    public class FontReference
    {
        [NoSerialize]
        private FontAsset _font;

        [NoSerialize]
        private float _size;

        [NoSerialize]
        private float _MSDFSize;

        [NoSerialize]
        private Font _cachedFont;

        /// <summary>
        /// Initializes a new instance of the <see cref="FontReference"/> struct.
        /// </summary>
        public FontReference()
        {
            _font = null;
            _size = 30;
            _MSDFSize = 32;
            _cachedFont = null;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="FontReference"/> struct.
        /// </summary>
        /// <param name="font">The font.</param>
        /// <param name="size">The font size.</param>
        /// <param name="MSDFSize">The font size for MSDF atlas generation.</param>
        public FontReference(FontAsset font, float size, float MSDFSize = 32.0f)
        {
            _font = font;
            _size = size;
            _MSDFSize = MSDFSize;
            _cachedFont = null;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="FontReference"/> struct.
        /// </summary>
        /// <param name="other">The other font reference.</param>
        public FontReference(FontReference other)
        {
            _font = other._font;
            _size = other._size;
            _MSDFSize = other._MSDFSize;
            _cachedFont = other._cachedFont;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="FontReference"/> struct.
        /// </summary>
        /// <param name="font">The font.</param>
        /// <param name="MSDFSize">The font size for MSDF atlas generation.</param>
        public FontReference(Font font, float MSDFSize = 32.0f)
        {
            if (font)
            {
                _font = font.Asset;
                _size = font.Size;
            }
            else
            {
                _font = null;
                _size = 30;
            }
            _MSDFSize = MSDFSize;
            _cachedFont = font;
        }

        /// <summary>
        /// The font asset.
        /// </summary>
        [EditorOrder(0), Tooltip("The font asset to use as characters source.")]
        public FontAsset Font
        {
            get => _font;
            set
            {
                if (_font != value)
                {
                    _font = value;
                    _cachedFont = null;
                }
            }
        }

        /// <summary>
        /// The size of the font characters.
        /// </summary>
        [EditorOrder(10), Limit(1, 500, 0.5f), Tooltip("The size of the font characters.")]
        public float Size
        {
            get => _size;
            set
            {
                if (_size != value)
                {
                    _size = value;
                    _cachedFont = null;
                }
            }
        }

        /// <summary>
        /// The font size for MSDF atlas generation.
        /// </summary>
        [EditorOrder(20), Limit(4, 256), Tooltip("The font size for MSDF atlas generation.")]
        public float MSDFSize
        {
            get => _MSDFSize;
            set
            {
                if (_MSDFSize != value)
                {
                    _MSDFSize = value;
                    _cachedFont = null;
                }
            }
        }

        /// <summary>
        /// Gets the font object described by the structure.
        /// </summary>
        /// <returns>The font or null if descriptor is invalid.</returns>
        public Font GetFont()
        {
            if (_cachedFont)
                return _cachedFont;
            if (_font)
                _cachedFont = _font.CreateFont(_size, _MSDFSize);
            return _cachedFont;
        }

        /// <summary>
        /// Gets the bold font object described by the structure.
        /// </summary>
        /// <returns>The bold font asset.</returns>
        public FontReference GetBold()
        {
            return new FontReference(_font?.GetBold(), _size);
        }

        /// <summary>
        /// Gets the italic font object described by the structure.
        /// </summary>
        /// <returns>The bold font asset.</returns>
        public FontReference GetItalic()
        {
            return new FontReference(_font?.GetItalic(), _size);
        }

        /// <summary>
        /// Determines whether the specified <see cref="FontReference" /> is equal to this instance.
        /// </summary>
        /// <param name="other">The <see cref="FontReference" /> to compare with this instance.</param>
        /// <returns><c>true</c> if the specified <see cref="FontReference" /> is equal to this instance; otherwise, <c>false</c>.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(FontReference other)
        {
            return !(other is null) && _font == other._font && _size == other._size && _MSDFSize == other._MSDFSize;
        }

        /// <summary>
        /// Compares two font references.
        /// </summary>
        /// <param name="lhs">The left.</param>
        /// <param name="rhs">The right.</param>
        /// <returns>True if font references are equal, otherwise false.</returns>
        public static bool operator ==(FontReference lhs, FontReference rhs)
        {
            if (lhs is null)
                return rhs is null;
            return lhs.Equals(rhs);
        }

        /// <summary>
        /// Compares two font references.
        /// </summary>
        /// <param name="lhs">The left.</param>
        /// <param name="rhs">The right.</param>
        /// <returns>True if font references are not equal, otherwise false.</returns>
        public static bool operator !=(FontReference lhs, FontReference rhs)
        {
            if (lhs is null)
                return !(rhs is null);
            return !lhs.Equals(rhs);
        }

        /// <inheritdoc />
        public override bool Equals(object other)
        {
            if (!(other is FontReference))
                return false;
            var fontReference = (FontReference)other;
            return Equals(fontReference);
        }

        /// <inheritdoc />
        public override int GetHashCode()
        {
            unchecked
            {
                int hashCode = _font ? _font.GetHashCode() : 0;
                hashCode = (hashCode * 397) ^ _size.GetHashCode();
                hashCode = (hashCode * 397) ^ _MSDFSize.GetHashCode();
                return hashCode;
            }
        }

        /// <inheritdoc />
        public override string ToString()
        {
            return string.Format("{0}, size {1}, MSDFSize {2}", _font ? _font.ToString() : string.Empty, _size, _MSDFSize);
        }
    }
}
