// Copyright (c) Wojciech Figat. All rights reserved.

namespace FlaxEditor.Windows.Profiler
{
    /// <summary>
    /// Profiler samples storage buffer. Support recording new frame samples.
    /// </summary>
    /// <typeparam name="T">Single sample data type.</typeparam>
    public class SamplesBuffer<T>
    {
        private T[] _data;
        private int _count;

        /// <summary>
        /// Gets the amount of samples in the buffer.
        /// </summary>
        public int Count => _count;

        /// <summary>
        /// Gets the last sample value. Check buffer <see cref="Count"/> before calling this property.
        /// </summary>
        public T Last => _data[_count - 1];

        /// <summary>
        /// Gets or sets the sample value at the specified index.
        /// </summary>
        /// <param name="index">The index.</param>
        /// <returns>The sample value.</returns>
        public T this[int index]
        {
            get => _data[index];
            set => _data[index] = value;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="SamplesBuffer{T}"/> class.
        /// </summary>
        /// <param name="capacity">The maximum buffer capacity.</param>
#if USE_PROFILER
        public SamplesBuffer(int capacity = ProfilerMode.MaxSamples)
#else
        public SamplesBuffer(int capacity = 600)
#endif
        {
            _data = new T[capacity];
            _count = 0;
        }

        /// <summary>
        /// Gets the sample at the specified index or the last sample if index is equal to -1.
        /// </summary>
        /// <param name="index">The index.</param>
        /// <returns>The sample value</returns>
        public T Get(int index)
        {
            if (_count == 0 || index >= _data.Length || _data.Length == 0)
                return default;
            return index == -1 ? _data[_count - 1] : _data[index];
        }

        /// <summary>
        /// Clears this buffer.
        /// </summary>
        public void Clear()
        {
            _count = 0;
        }

        /// <summary>
        /// Adds the specified sample to the buffer.
        /// </summary>
        /// <param name="sample">The sample.</param>
        public void Add(T sample)
        {
            // Remove first sample if no space
            if (_count == _data.Length)
            {
                for (int i = 1; i < _count; i++)
                {
                    _data[i - 1] = _data[i];
                }

                _count--;
            }

            _data[_count++] = sample;
        }

        /// <summary>
        /// Adds the specified sample to the buffer.
        /// </summary>
        /// <param name="sample">The sample.</param>
        public void Add(ref T sample)
        {
            // Remove first sample if no space
            if (_count == _data.Length)
            {
                for (int i = 1; i < _count; i++)
                {
                    _data[i - 1] = _data[i];
                }

                _count--;
            }

            _data[_count++] = sample;
        }
    }
}
