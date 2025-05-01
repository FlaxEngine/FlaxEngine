// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.IO;

namespace FlaxEngine.Json
{
    /// <summary>
    /// Implements a <see cref="T:System.IO.TextReader" /> that reads from unmanaged UTF8 string buffer (provided as raw pointer and length).
    /// </summary>
    internal class UnmanagedStringReader : TextReader
    {
        private unsafe byte* _ptr;
        private int _pos;
        private int _length;

        /// <summary>
        /// Initializes a new instance of the <see cref="UnmanagedStringReader"/> class.
        /// </summary>
        public unsafe UnmanagedStringReader()
        {
            _ptr = null;
            _pos = 0;
            _length = 0;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="UnmanagedStringReader"/> class.
        /// </summary>
        /// <param name="buffer">The text buffer pointer (raw, fixed memory).</param>
        /// <param name="length">The text length (characters count).</param>
        public unsafe UnmanagedStringReader(IntPtr buffer, int length)
        {
            if (buffer == IntPtr.Zero)
                throw new ArgumentNullException(nameof(buffer));

            _ptr = (byte*)buffer.ToPointer();
            _pos = 0;
            _length = length;
        }

        /// <summary>
        /// Initializes the reader with the specified text buffer.
        /// </summary>
        /// <param name="buffer">The text buffer pointer (raw, fixed memory).</param>
        /// <param name="length">The text length (characters count).</param>
        public unsafe void Initialize(void* buffer, int length)
        {
            if (buffer == null)
                throw new ArgumentNullException(nameof(buffer));

            _ptr = (byte*)buffer;
            _pos = 0;
            _length = length;
        }

        /// <summary>
        /// Initializes the reader with the specified text buffer.
        /// </summary>
        /// <param name="buffer">The text buffer pointer (raw, fixed memory).</param>
        /// <param name="length">The text length (characters count).</param>
        public unsafe void Initialize(IntPtr buffer, int length)
        {
            if (buffer == IntPtr.Zero)
                throw new ArgumentNullException(nameof(buffer));

            _ptr = (byte*)buffer.ToPointer();
            _pos = 0;
            _length = length;
        }

        /// <inheritdoc />
        protected override unsafe void Dispose(bool disposing)
        {
            _ptr = null;
            _pos = 0;
            _length = 0;

            base.Dispose(disposing);
        }

        /// <inheritdoc />
        public override unsafe int Peek()
        {
            if (_pos == _length)
                return -1;

            return _ptr[_pos];
        }

        /// <inheritdoc />
        public override unsafe int Read()
        {
            if (_pos == _length)
                return -1;

            return _ptr[_pos++];
        }
    }
}
