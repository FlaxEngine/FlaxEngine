// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;

namespace Flax.Build
{
    /// <summary>
    /// The two-way enumerator interface that can move forward or backwards.
    /// </summary>
    /// <typeparam name="T">The element type.</typeparam>
    /// <seealso cref="System.Collections.Generic.IEnumerator{T}" />
    public interface ITwoWayEnumerator<T> : IEnumerator<T>
    {
        /// <summary>
        /// Advances the enumerator to the previous element of the collection.
        /// </summary>
        /// <returns>
        /// <see langword="true" /> if the enumerator was successfully advanced to the previous element; <see langword="false" /> if the enumerator has passed the beginning of the collection.</returns>
        /// <exception cref="T:System.InvalidOperationException">The collection was modified after the enumerator was created. </exception>
        bool MovePrevious();
    }

    /// <summary>
    /// The implementation of the <see cref="ITwoWayEnumerator{T}"/> that uses a list.
    /// </summary>
    /// <typeparam name="T">The element type.</typeparam>
    /// <seealso cref="Flax.Build.ITwoWayEnumerator{T}" />
    public class TwoWayEnumerator<T> : ITwoWayEnumerator<T>
    {
        private IEnumerator<T> _enumerator;
        private List<T> _buffer;
        private int _index;

        /// <summary>
        /// Initializes a new instance of the <see cref="TwoWayEnumerator{T}"/> class.
        /// </summary>
        /// <param name="enumerator">The enumerator.</param>
        public TwoWayEnumerator(IEnumerator<T> enumerator)
        {
            _enumerator = enumerator ?? throw new ArgumentNullException("enumerator");
            _buffer = new List<T>();
            _index = -1;
        }

        /// <summary>
        /// Advances the enumerator to the previous element of the collection.
        /// </summary>
        /// <returns><see langword="true" /> if the enumerator was successfully advanced to the previous element; <see langword="false" /> if the enumerator has passed the beginning of the collection.</returns>
        public bool MovePrevious()
        {
            if (_index <= 0)
            {
                return false;
            }

            --_index;
            return true;
        }

        /// <summary>
        /// Advances the enumerator to the next element of the collection.
        /// </summary>
        /// <returns><see langword="true" /> if the enumerator was successfully advanced to the next element; <see langword="false" /> if the enumerator has passed the end of the collection.</returns>
        public bool MoveNext()
        {
            if (_index < _buffer.Count - 1)
            {
                ++_index;
                return true;
            }

            if (_enumerator.MoveNext())
            {
                _buffer.Add(_enumerator.Current);
                ++_index;
                return true;
            }

            return false;
        }

        /// <summary>
        /// Gets the element in the collection at the current position of the enumerator.
        /// </summary>
        public T Current
        {
            get
            {
                if (_index < 0 || _index >= _buffer.Count)
                    throw new InvalidOperationException();
                return _buffer[_index];
            }
        }

        /// <summary>
        /// Sets the enumerator to its initial position, which is before the first element in the collection.
        /// </summary>
        public void Reset()
        {
            _enumerator.Reset();
            _buffer.Clear();
            _index = -1;
        }

        /// <summary>
        /// Performs application-defined tasks associated with freeing, releasing, or resetting unmanaged resources.
        /// </summary>
        public void Dispose()
        {
            _enumerator.Dispose();
        }

        /// <summary>
        /// Gets the element in the collection at the current position of the enumerator.
        /// </summary>
        object System.Collections.IEnumerator.Current => Current;
    }
}
