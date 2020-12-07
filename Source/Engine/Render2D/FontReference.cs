// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using System.Runtime.CompilerServices;

namespace FlaxEngine
{
    /// <summary>
    /// Font reference that defines the font asset and font size to use.
    /// </summary>
    public struct FontReference
    {
        [NoSerialize]
        private FontAsset _font;

        [NoSerialize]
        private int _size;

        [NoSerialize]
        private Font _cachedFont;

        /// <summary>
        /// Initializes a new instance of the <see cref="FontReference"/> struct.
        /// </summary>
        /// <param name="font">The font.</param>
        /// <param name="size">The font size.</param>
        public FontReference(FontAsset font, int size)
        {
            _font = font;
            _size = size;
            _cachedFont = null;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="FontReference"/> struct.
        /// </summary>
        /// <param name="font">The font.</param>
        public FontReference(Font font)
        {
            _font = font?.Asset;
            _size = font?.Size ?? 30;
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

                    if (_cachedFont)
                        _cachedFont = null;
                }
            }
        }

        /// <summary>
        /// The size of the font characters.
        /// </summary>
        [EditorOrder(10), Limit(1, 500, 0.1f), Tooltip("The size of the font characters.")]
        public int Size
        {
            get => _size;
            set
            {
                if (_size != value)
                {
                    _size = value;

                    if (_cachedFont)
                        _cachedFont = null;
                }
            }
        }

        /// <summary>
        /// Gets the font object described by the structure.
        /// </summary>
        /// <returns>Th font or null if descriptor is invalid.</returns>
        public Font GetFont()
        {
            return _cachedFont ?? (_cachedFont = _font?.CreateFont(_size));
        }

        /// <summary>
        /// Determines whether the specified <see cref="FontReference" /> is equal to this instance.
        /// </summary>
        /// <param name="other">The <see cref="FontReference" /> to compare with this instance.</param>
        /// <returns>
        /// <c>true</c> if the specified <see cref="FontReference" /> is equal to this instance; otherwise, <c>false</c>.
        /// </returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(ref FontReference other)
        {
            return _font == other._font && _size == other._size;
        }

        /// <summary>
        /// Compares two font references.
        /// </summary>
        /// <param name="lhs">The left.</param>
        /// <param name="rhs">The right.</param>
        /// <returns>True if font references are equal, otherwise false.</returns>
        public static bool operator ==(FontReference lhs, FontReference rhs)
        {
            return lhs.Equals(ref rhs);
        }

        /// <summary>
        /// Compares two font references.
        /// </summary>
        /// <param name="lhs">The left.</param>
        /// <param name="rhs">The right.</param>
        /// <returns>True if font references are not equal, otherwise false.</returns>
        public static bool operator !=(FontReference lhs, FontReference rhs)
        {
            return !lhs.Equals(ref rhs);
        }

        /// <inheritdoc />
        public override bool Equals(object other)
        {
            if (!(other is FontReference))
                return false;
            var fontReference = (FontReference)other;
            return Equals(ref fontReference);
        }

        /// <inheritdoc />
        public override int GetHashCode()
        {
            unchecked
            {
                int hashCode = _font.GetHashCode();
                hashCode = (hashCode * 397) ^ _size;
                return hashCode;
            }
        }

        /// <inheritdoc />
        public override string ToString()
        {
            return string.Format("{0}, size {1}", _font ? _font.ToString() : string.Empty, _size);
        }
    }
}
