// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.IO;
using System.Threading;
using System.Threading.Tasks;

namespace FlaxEngine.Json
{
    /// <summary>
    /// Implements a <see cref="T:System.IO.Stream" /> that reads from unmanaged buffer (provided as raw pointer and length).
    /// </summary>
    internal class UnmanagedMemoryStream : Stream
    {
        private unsafe byte* _ptr;
        private int _length;
        private int _pos;
        private FileAccess _access;
        internal bool _isOpen;
        private Task<int> _lastReadTask;

        internal UnmanagedMemoryStream()
        {
        }

        internal unsafe UnmanagedMemoryStream(byte* pointer, int length, FileAccess access = FileAccess.Read) => Initialize(pointer, length, access);

        internal unsafe void Initialize(byte* pointer, int length, FileAccess access = FileAccess.Read)
        {
            _ptr = pointer;
            _length = length;
            _pos = 0;
            _access = access;
            _isOpen = true;
            _lastReadTask = null;
        }

        public override bool CanRead => _isOpen && (uint)(_access & FileAccess.Read) > 0U;

        public override bool CanSeek => _isOpen;

        public override bool CanWrite => _isOpen && (uint)(_access & FileAccess.Write) > 0U;

        protected override unsafe void Dispose(bool disposing)
        {
            _isOpen = false;
            _ptr = null;
            base.Dispose(disposing);
        }

        public override void Flush()
        {
        }

        public override Task FlushAsync(CancellationToken cancellationToken)
        {
            Flush();
            return Task.FromResult(0);
        }

        public unsafe byte* Pointer => _ptr;

        public override long Length => _length;

        public long Capacity => _length;

        public override long Position
        {
            get => _pos;
            set => _pos = (int)value;
        }

        public unsafe byte* PositionPointer
        {
            get => _ptr + _pos;
            set => _pos = (int)(value - _ptr);
        }

        public override unsafe int Read(byte[] buffer, int offset, int count)
        {
            int toRead = _length - _pos;
            if (toRead > count)
                toRead = count;
            if (toRead <= 0)
                return 0;
            fixed (byte* bufferPtr = buffer)
                Utils.MemoryCopy(new IntPtr(bufferPtr), new IntPtr(_ptr + _pos), (ulong)toRead);
            _pos += toRead;
            return toRead;
        }

        public override Task<int> ReadAsync(byte[] buffer, int offset, int count, CancellationToken cancellationToken)
        {
            int result = Read(buffer, offset, count);
            return _lastReadTask == null || _lastReadTask.Result != result ? (_lastReadTask = Task.FromResult(result)) : _lastReadTask;
        }

        public override unsafe int ReadByte()
        {
            int index = _pos;
            if (index >= _length)
                return -1;
            _pos = index + 1;
            return _ptr[index];
        }

        public override long Seek(long offset, SeekOrigin loc)
        {
            switch (loc)
            {
            case SeekOrigin.Begin:
                _pos = (int)offset;
                break;
            case SeekOrigin.Current:
                _pos += (int)offset;
                break;
            case SeekOrigin.End:
                _pos = _length + (int)offset;
                break;
            default: throw new ArgumentOutOfRangeException();
            }
            return _pos;
        }

        public override void SetLength(long value)
        {
            _length = (int)value;
            if (_pos > value)
                _pos = _length;
        }

        public override unsafe void Write(byte[] buffer, int offset, int count)
        {
            int newPos = _pos + count;
            if (newPos > _length)
                _length = newPos;
            fixed (byte* bufferPtr = buffer)
                Utils.MemoryCopy(new IntPtr(bufferPtr), new IntPtr(_pos + _pos), (ulong)count);
            _pos = newPos;
        }

        public override Task WriteAsync(byte[] buffer, int offset, int count, CancellationToken cancellationToken)
        {
            Write(buffer, offset, count);
            return Task.FromResult(0);
        }

        public override unsafe void WriteByte(byte value)
        {
            long newPos = _pos + 1;
            if (_pos >= _length)
                _length = (int)newPos;
            _ptr[_pos] = value;
            _pos = (int)newPos;
        }
    }
}
