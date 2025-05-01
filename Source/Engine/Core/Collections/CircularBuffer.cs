// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using Newtonsoft.Json;

namespace FlaxEngine.Collections
{
    /// <summary>
    /// Creates new structure array like, with fast front and back insertion.
    /// <para>Every overflow of this buffer removes last item form other side of insertion</para>
    /// </summary>
    /// <remarks>This collection is NOT thread-safe.</remarks>
    /// <typeparam name="T">Type of items inserted into buffer</typeparam>
    [Serializable]
    [JsonObject(MemberSerialization.OptIn)]
    public class CircularBuffer<T> : IEnumerable<T>
    {
        /// <summary>
        /// Arguments for new item added event
        /// </summary>
        public class ItemAddedEventArgs : EventArgs
        {
            /// <summary>
            /// Gets Index of new element in buffer
            /// </summary>
            public int Index { get; }

            /// <summary>
            /// Gets added item
            /// </summary>
            public T Item { get; }

            /// <summary>
            /// Initializes a new instance of the <see cref="ItemAddedEventArgs"/> class.
            /// </summary>
            /// <param name="index">The index.</param>
            /// <param name="item">The item.</param>
            public ItemAddedEventArgs(int index, T item)
            {
                Index = index;
                Item = item;
            }
        }

        /// <summary>
        /// Arguments for item removed event
        /// </summary>
        public class ItemRemovedEventArgs : EventArgs
        {
            /// <summary>
            /// Gets if item removed was item from front of the buffer
            /// </summary>
            public bool WasFrontItem { get; }

            /// <summary>
            /// Gets removed item
            /// </summary>
            public T Item { get; }

            /// <summary>
            /// Initializes a new instance of the <see cref="ItemRemovedEventArgs"/> class.
            /// </summary>
            /// <param name="wasFrontItem">if set to <c>true</c> [was front item].</param>
            /// <param name="item">The item.</param>
            public ItemRemovedEventArgs(bool wasFrontItem, T item)
            {
                WasFrontItem = wasFrontItem;
                Item = item;
            }
        }

        /// <summary>
        /// Arguments for item being replaced because of buffer was overflown with data
        /// </summary>
        public class ItemOverflownEventArgs : EventArgs
        {
            /// <summary>
            /// Gets if item removed was item from front of the buffer
            /// </summary>
            public bool WasFrontItem { get; }

            /// <summary>
            /// Gets overflown item
            /// </summary>
            public T Item { get; }

            /// <summary>
            /// Initializes a new instance of the <see cref="ItemOverflownEventArgs"/> class.
            /// </summary>
            /// <param name="wasFrontItem">if set to <c>true</c> [was front item].</param>
            /// <param name="item">The item.</param>
            public ItemOverflownEventArgs(bool wasFrontItem, T item)
            {
                WasFrontItem = wasFrontItem;
                Item = item;
            }
        }

        [JsonProperty("buffer"), Serialize]
        private T[] _buffer;

        [JsonProperty("frontItem"), Serialize]
        private int _frontItem;

        [JsonProperty("backItem"), Serialize]
        private int _backItem;

        /// <summary>
        /// Executes an action when item is removed
        /// </summary>
        public event ItemRemovedEventHandler OnItemRemoved;

        /// <see cref="ItemRemovedEventHandler" />
        public delegate void ItemRemovedEventHandler(object sender, ItemRemovedEventArgs e);

        /// <summary>
        /// Executes an action when item is added
        /// </summary>
        public event ItemAddedEventHandler OnItemAdded;

        /// <see cref="ItemAddedEventHandler" />
        public delegate void ItemAddedEventHandler(object sender, ItemAddedEventArgs e);

        /// <summary>
        /// Executes an action when item is removed because of overflow in buffer
        /// </summary>
        public event ItemOverflownEventHandler OnItemOverflown;

        /// <see cref="ItemOverflownEventHandler" />
        public delegate void ItemOverflownEventHandler(object sender, ItemOverflownEventArgs e);

        /// <summary>
        /// Amount of items currently in buffer
        /// </summary>
        public int Count { get; private set; }

        /// <summary>
        /// Current capacity of internal buffer
        /// </summary>
        public int Capacity
        {
            get => _buffer.Length;
            set
            {
                if (value <= 0)
                    throw new ArgumentOutOfRangeException();
                if (value == _buffer.Length)
                    return;
                if (Count > 0)
                    throw new InvalidOperationException("Cannot change capacity for non-empty buffer.");

                _buffer = new T[value];
            }
        }

        /// <summary>
        /// Returns true if there are no items in structure, or false if there are
        /// </summary>
        public bool IsEmpty => Count == 0;

        /// <summary>
        /// Returns true if buffer is filled with whole of its capacity with items
        /// </summary>
        public bool IsFull => Count == Capacity;

        /// <summary>
        /// Creates new instance of object with given capacity, copies given array as a framework
        /// </summary>
        /// <param name="buffer">Buffer to insert into</param>
        /// <param name="frontItem">First index of an item in provided buffer</param>
        /// <param name="backItem">Last index on an item in provided buffer</param>
        [JsonConstructor]
        public CircularBuffer(IEnumerable<T> buffer, int frontItem = 0, int backItem = 0)
        {
            var insertionArray = buffer.ToArray();
            if (insertionArray.Length < frontItem)
                throw new ArgumentOutOfRangeException(nameof(frontItem),
                                                      "argument cannot be larger then requested capacity");
            if (-1 > frontItem)
                throw new ArgumentOutOfRangeException(nameof(frontItem),
                                                      "argument cannot be smaller then -1");
            if (insertionArray.Length < backItem)
                throw new ArgumentOutOfRangeException(nameof(frontItem),
                                                      "argument cannot be larger then requested capacity");
            if (-1 > backItem)
                throw new ArgumentOutOfRangeException(nameof(frontItem),
                                                      "argument cannot be smaller then -1");
            _buffer = insertionArray;
            _backItem = backItem;
            _frontItem = frontItem;
            Count = insertionArray.Length;
        }

        /// <summary>
        /// Creates new instance of object with given capacity
        /// </summary>
        /// <param name="capacity">Capacity of internal structure</param>
        public CircularBuffer(int capacity)
        {
            if (capacity <= 0)
                throw new ArgumentOutOfRangeException(nameof(capacity), "argument cannot be lower or equal zero");
            _buffer = new T[capacity];
            _backItem = 0;
            _frontItem = 0;
            Count = 0;
        }

        /// <summary>
        /// Creates new instance of object with given capacity and adds array of items to internal buffer
        /// </summary>
        /// <param name="capacity">Capacity of internal structure</param>
        /// <param name="items">Items to input</param>
        /// <param name="arrayIndex">Index of items to input at in internal buffer</param>
        public CircularBuffer(int capacity, T[] items, int arrayIndex = 0)
        {
            if (capacity <= 0)
                throw new ArgumentOutOfRangeException(nameof(capacity), "argument cannot be lower or equal zero");
            if (items.Length + arrayIndex > capacity)
                throw new ArgumentOutOfRangeException(nameof(items), "argument cannot be larger then requested capacity with moved arrayIndex");
            _buffer = new T[capacity];
            items.CopyTo(_buffer, arrayIndex);
            _backItem = arrayIndex;
            _frontItem = items.Length + arrayIndex;
            if (items.Length > 0)
                _frontItem -= 1;
            Count = items.Length;
        }

        /// <summary>
        /// Gets or sets item from list at given index.
        /// <remarks>All items are in order of input regardless of overflow that may occur</remarks>
        /// </summary>
        /// <param name="index">Index to item required</param>
        public T this[int index]
        {
            get
            {
                if (index < 0)
                    throw new ArgumentOutOfRangeException(nameof(index), "argument cannot be lower then zero");
                if (index >= Count)
                    throw new IndexOutOfRangeException("argument cannot be bigger then amount of elements");
                var currentIndex = (index + _backItem) % Capacity;
                return _buffer[currentIndex];
            }
            set
            {
                if (index < 0)
                    throw new ArgumentOutOfRangeException(nameof(index), "argument cannot be lower then zero");
                if (index >= Count)
                    throw new IndexOutOfRangeException("argument cannot be bigger then amount of elements");
                var currentIndex = (index + _backItem) % Capacity;
                _buffer[currentIndex] = value;
            }
        }

        /// <summary>
        /// Adds item to the front of the buffer
        /// </summary>
        /// <param name="item">Item to add</param>
        public void PushFront(T item)
        {
            if (!IsEmpty)
                IncreaseFrontIndex();
            OnItemAdded?.Invoke(this, new ItemAddedEventArgs(Count - 1, item));
            if (Count == Capacity)
                OnItemOverflown?.Invoke(this, new ItemOverflownEventArgs(false, _buffer[_frontItem]));

            _buffer[_frontItem] = item;
            if (Count < Capacity)
                Count++;
        }

        /// <summary>
        /// Adds item to the back of the buffer
        /// </summary>
        /// <param name="item">Item to add</param>
        public void PushBack(T item)
        {
            if (!IsEmpty)
                DecreaseBackIndex();
            OnItemAdded?.Invoke(this, new ItemAddedEventArgs(0, item));
            if (Count == Capacity)
                OnItemOverflown?.Invoke(this, new ItemOverflownEventArgs(true, _buffer[_backItem]));

            _buffer[_backItem] = item;
            if (Count < Capacity)
                Count++;
        }

        /// <summary>
        /// Gets top first element form collection
        /// </summary>
        public T Front()
        {
            if (Count == 0)
                throw new IndexOutOfRangeException("Collection cannot be empty");
            return _buffer[_frontItem];
        }

        /// <summary>
        /// Gets bottom first element form collection
        /// </summary>
        public T Back()
        {
            if (Count == 0)
                throw new IndexOutOfRangeException("Collection cannot be empty");
            return _buffer[_backItem];
        }

        /// <summary>
        /// Removes first item from the front of the buffer
        /// </summary>
        /// <returns></returns>
        public T PopFront()
        {
            if (IsEmpty)
                throw new IndexOutOfRangeException("You cannot remove item from empty collection");
            var result = Front();
            Count--;
            if (!IsEmpty)
            {
                DecreaseFrontIndex();
            }
            else
            {
                _frontItem = 0;
                _backItem = 0;
            }
            OnItemRemoved?.Invoke(this, new ItemRemovedEventArgs(true, result));
            return result;
        }

        /// <summary>
        /// Removes first item from the back of the buffer
        /// </summary>
        /// <returns></returns>
        public T PopBack()
        {
            if (IsEmpty)
                throw new IndexOutOfRangeException("You cannot remove item from empty collection");
            var result = Back();
            Count--;
            if (!IsEmpty)
            {
                IncreaseBackIndex();
            }
            else
            {
                _frontItem = 0;
                _backItem = 0;
            }
            OnItemRemoved?.Invoke(this, new ItemRemovedEventArgs(false, result));
            return result;
        }

        /// <summary>
        /// Copies the buffer contents to an array, according to the logical
        /// contents of the buffer (i.e. independent of the internal
        /// order/contents)
        /// </summary>
        /// <returns>A new array with a copy of the buffer contents.</returns>
        public T[] ToArray()
        {
            if (Count == 0)
                return Utils.GetEmptyArray<T>();

            var result = new T[Count];
            if (_backItem > _frontItem)
            {
                Array.Copy(_buffer, _backItem, result, 0, Capacity - _backItem);
                Array.Copy(_buffer, 0, result, Capacity - _backItem, _frontItem + 1);
            }
            else
            {
                Array.Copy(_buffer, _backItem, result, 0, _frontItem - _backItem + 1);
            }

            return result;
        }

        /// <summary>
        /// CopyTo copies a collection into an Array, starting at a particular index into the array.
        /// </summary>
        /// <returns>A new array with a copy of the buffer contents.</returns>
        public void CopyTo(T[] array, int arrayIndex)
        {
            ToArray().CopyTo(array, arrayIndex);
        }

        /// <summary>
        /// Clears buffer and remains capacity
        /// </summary>
        public void Clear()
        {
            if (Count > 0)
            {
                _buffer = new T[Capacity];
                _frontItem = 0;
                _backItem = 0;
                Count = 0;
            }
        }

        /// <summary>
        /// Clears buffer and changes its capacity.
        /// </summary>
        /// <param name="newCapacity">The new capacity of the buffer.</param>
        public void Clear(int newCapacity)
        {
            if (newCapacity <= 0)
                throw new ArgumentOutOfRangeException();

            _buffer = new T[newCapacity];
            _frontItem = 0;
            _backItem = 0;
            Count = 0;
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }

        /// <summary>
        /// Returns an enumerator that iterates through the collection.
        /// </summary>
        /// <returns>
        /// A <see cref="T:System.Collections.Generic.IEnumerator`1" /> that can be used to iterate through the collection.
        /// </returns>
        public IEnumerator<T> GetEnumerator()
        {
            if (IsEmpty)
                yield break;
            var array = ToArray();
            for (int i = 0; i < array.Length; i++)
                yield return array[i];
        }

        /// <summary>
        /// Decrease index of _backItem and warp it round if fall below 0
        /// <para>Move _frontItem back index if they've met</para>
        /// </summary>
        private void DecreaseBackIndex()
        {
            var currentIndex = --_backItem % Capacity;
            if (currentIndex < 0)
                currentIndex = Capacity + currentIndex;
            _backItem = currentIndex;
            if (_backItem == _frontItem && IsFull)
                DecreaseFrontIndex();
        }

        /// <summary>
        /// Decrease index of _frontItem and warp it round if fall below 0
        /// <para>Move _backItem back index if they've met</para>
        /// </summary>
        private void DecreaseFrontIndex()
        {
            var currentIndex = --_frontItem % Capacity;
            if (currentIndex < 0)
                currentIndex = Capacity + currentIndex;
            _frontItem = currentIndex;
            if (_backItem == _frontItem && IsFull)
                DecreaseBackIndex();
        }

        /// <summary>
        /// Increases index of _backItem and warp it round if exceded capacity
        /// <para>Move _frontItem forward index if they've met</para>
        /// </summary>
        private void IncreaseBackIndex()
        {
            _backItem = ++_backItem % Capacity;
            if (_backItem == _frontItem)
                IncreaseFrontIndex();
        }

        /// <summary>
        /// Increases index of _frontItem and warp it round if exceded capacity
        /// <para>Move _backItem forward index if they've met</para>
        /// </summary>
        private void IncreaseFrontIndex()
        {
            _frontItem = ++_frontItem % Capacity;
            if (_backItem == _frontItem)
                IncreaseBackIndex();
        }
    }
}
