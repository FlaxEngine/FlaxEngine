// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if !NET9_0_OR_GREATER

using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.Linq;

namespace FlaxEngine.Collections
{
    /// <summary>
    /// A dictionary object that allows rapid hash lookups using keys, but also maintains the key insertion order so that values can be retrieved by key index.
    /// </summary>
    [Serializable]
    public class OrderedDictionary<TKey, TValue> : IOrderedDictionary<TKey, TValue>
    {
        #region Fields/Properties

        private KeyedCollectionInternal<TKey, KeyValuePair<TKey, TValue>> _keyedCollection;

        /// <summary>
        /// Gets or sets the value associated with the specified key.
        /// </summary>
        /// <param name="key">The key associated with the value to get or set.</param>
        public TValue this[TKey key]
        {
            get => GetValue(key);
            set => SetValue(key, value);
        }

        /// <summary>
        /// Gets or sets the value at the specified index.
        /// </summary>
        /// <param name="index">The index of the value to get or set.</param>
        public TValue this[int index]
        {
            get => GetItem(index).Value;
            set => SetItem(index, value);
        }

        /// <summary>
        /// Gets the number of elements contained in the <see cref="OrderedDictionary{TKey,TValue}" />.
        /// </summary>
        /// <returns>The number of elements contained in the T:System.Collections.ICollection.</returns>
        public int Count => _keyedCollection.Count;

        /// <summary>
        /// Gets an <see cref="T:System.Collections.ICollection" /> object containing the keys in the
        /// <see cref="OrderedDictionary{TKey,TValue}" /> object.
        /// </summary>
        /// <returns>
        /// An <see cref="T:System.Collections.ICollection" /> object containing the keys in the
        /// <see cref="OrderedDictionary{TKey,TValue}" /> object.
        /// </returns>
        public ICollection<TKey> Keys
        {
            get { return _keyedCollection.Select(x => x.Key).ToList(); }
        }

        /// <summary>
        /// Gets an <see cref="T:System.Collections.ICollection" /> object containing the values in the
        /// <see cref="OrderedDictionary{TKey,TValue}" /> object.
        /// </summary>
        /// <returns>
        /// An <see cref="T:System.Collections.ICollection" /> object containing the values in the
        /// <see cref="OrderedDictionary{TKey,TValue}" /> object.
        /// </returns>
        public ICollection<TValue> Values
        {
            get { return _keyedCollection.Select(x => x.Value).ToList(); }
        }

        /// <summary>
        /// The keys equality comparer.
        /// </summary>
        public IEqualityComparer<TKey> Comparer { get; private set; }

        #endregion

        #region Constructors

        /// <summary>
        /// A dictionary object that allows rapid hash lookups using keys, but also
        /// maintains the key insertion order so that values can be retrieved by
        /// key index.
        /// </summary>
        public OrderedDictionary()
        {
            Initialize();
        }

        /// <summary>
        /// <inheritdoc cref="OrderedDictionary{TKey,TValue}" />
        /// </summary>
        /// <remarks>Allows custom comparer for items</remarks>
        public OrderedDictionary(IEqualityComparer<TKey> comparer)
        {
            Initialize(comparer);
        }

        /// <summary>
        /// <inheritdoc cref="OrderedDictionary{TKey,TValue}" />
        /// </summary>
        /// <remarks>Copy constructor</remarks>
        public OrderedDictionary(IOrderedDictionary<TKey, TValue> dictionary)
        {
            Initialize();
            foreach (KeyValuePair<TKey, TValue> pair in dictionary)
                _keyedCollection.Add(pair);
        }

        /// <summary>
        /// <inheritdoc cref="OrderedDictionary{TKey,TValue}" />
        /// </summary>
        /// <remarks>Copy constructor with custom items comparer</remarks>
        public OrderedDictionary(IOrderedDictionary<TKey, TValue> dictionary, IEqualityComparer<TKey> comparer)
        {
            Initialize(comparer);
            foreach (KeyValuePair<TKey, TValue> pair in dictionary)
                _keyedCollection.Add(pair);
        }

        #endregion

        #region Methods

        private void Initialize(IEqualityComparer<TKey> comparer = null)
        {
            Comparer = comparer;
            if (comparer != null)
                _keyedCollection = new KeyedCollectionInternal<TKey, KeyValuePair<TKey, TValue>>(x => x.Key, comparer);
            else
                _keyedCollection = new KeyedCollectionInternal<TKey, KeyValuePair<TKey, TValue>>(x => x.Key);
        }

        /// <summary>Adds an element with the specified key and value into the <see cref="OrderedDictionary{TKey,TValue}" />.</summary>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="value">The value of the element to add.</param>
        public void Add(TKey key, TValue value)
        {
            _keyedCollection.Add(new KeyValuePair<TKey, TValue>(key, value));
        }

        /// <summary>
        /// Clears the contents of the <see cref="OrderedDictionary{TKey,TValue}" /> instance.
        /// </summary>
        public void Clear()
        {
            _keyedCollection.Clear();
        }

        /// <summary>
        /// Performs additional custom processes before inserting a new element into the
        /// <see cref="OrderedDictionary{TKey,TValue}" /> instance.
        /// </summary>
        /// <param name="index">The index.</param>
        /// <param name="key">The key of the element to insert.</param>
        /// <param name="value">The value of the element to insert.</param>
        public void Insert(int index, TKey key, TValue value)
        {
            _keyedCollection.Insert(index, new KeyValuePair<TKey, TValue>(key, value));
        }

        /// <summary>
        /// Determines whether an element is in the <see cref="OrderedDictionary{TKey,TValue}" />.
        /// </summary>
        /// <param name="key">The object to locate in the current dictionary. The element to locate can be null for reference types.</param>
        /// <returns>The index of the item.</returns>
        public int IndexOf(TKey key)
        {
            if (_keyedCollection.Contains(key))
                return _keyedCollection.IndexOf(_keyedCollection[key]);
            return -1;
        }

        /// <summary>
        /// Determines whether the <see cref="OrderedDictionary{TKey,TValue}" /> contains a specific value.
        /// </summary>
        /// <param name="value">The value to locate in the <see cref="OrderedDictionary{TKey,TValue}" />.</param>
        /// <returns>True if the <see cref="OrderedDictionary{TKey,TValue}" /> contains an element with the specified value; otherwise, false.</returns>
        public bool ContainsValue(TValue value)
        {
            return Values.Contains(value);
        }

        /// <summary>
        /// Determines whether the <see cref="OrderedDictionary{TKey,TValue}" /> contains a specific value.
        /// </summary>
        /// <param name="value">The value to locate in the <see cref="OrderedDictionary{TKey,TValue}" />.</param>
        /// <param name="comparer">The custom <see cref="IEqualityComparer" /> for this search</param>
        /// <returns>True if the <see cref="OrderedDictionary{TKey,TValue}" /> contains an element with the specified value; otherwise, false.</returns>
        public bool ContainsValue(TValue value, IEqualityComparer<TValue> comparer)
        {
            return Values.Contains(value, comparer);
        }

        /// <summary>
        /// Determines whether the <see cref="OrderedDictionary{TKey,TValue}" /> contains a specific key.
        /// </summary>
        /// <param name="key">The key to locate in the <see cref="OrderedDictionary{TKey,TValue}" />.</param>
        /// <returns>True if the <see cref="OrderedDictionary{TKey,TValue}" /> contains an element with the specified key; otherwise, false.</returns>
        public bool ContainsKey(TKey key)
        {
            return _keyedCollection.Contains(key);
        }

        /// <summary>
        /// Gets item at given index.
        /// </summary>
        /// <param name="index">Requested key at index</param>
        /// <exception cref="ArgumentException">
        /// Thrown when the index specified does not refer to a KeyValuePair in this object
        /// </exception>
        public KeyValuePair<TKey, TValue> GetItem(int index)
        {
            if (index < 0 || index >= _keyedCollection.Count)
                throw new ArgumentException(string.Format("The index was outside the bounds of the dictionary: {0}", index));
            return _keyedCollection[index];
        }

        /// <summary>
        /// Sets the value at the index specified.
        /// </summary>
        /// <param name="index">The index of the value desired</param>
        /// <param name="value">The value to set</param>
        /// <exception cref="ArgumentOutOfRangeException">
        /// Thrown when the index specified does not refer to a KeyValuePair in this object
        /// </exception>
        public void SetItem(int index, TValue value)
        {
            if (index < 0 || index >= _keyedCollection.Count)
                throw new ArgumentException($"The index is outside the bounds of the dictionary: {index}");
            var kvp = new KeyValuePair<TKey, TValue>(_keyedCollection[index].Key, value);
            _keyedCollection[index] = kvp;
        }

        /// <summary>
        /// Returns an <see cref="T:System.Collections.IEnumerator" /> that iterates through the
        /// <see cref="OrderedDictionary{TKey,TValue}" />.
        /// </summary>
        /// <returns>An <see cref="T:System.Collections.IEnumerator" /> for the <see cref="OrderedDictionary{TKey,TValue}" />.</returns>
        public IEnumerator<KeyValuePair<TKey, TValue>> GetEnumerator()
        {
            return _keyedCollection.GetEnumerator();
        }

        /// <summary>
        /// Performs additional custom processes before removing an element from the
        /// <see cref="OrderedDictionary{TKey,TValue}" /> instance.
        /// </summary>
        /// <param name="key">The key of the element to remove.</param>
        public bool Remove(TKey key)
        {
            return _keyedCollection.Remove(key);
        }

        /// <summary>
        /// Removes the <see cref="OrderedDictionary{TKey,TValue}" /> item at the specified index.
        /// </summary>
        /// <param name="index">The zero-based index of the item to remove.</param>
        public void RemoveAt(int index)
        {
            if (index < 0 || index >= _keyedCollection.Count)
                throw new ArgumentException($"The index was outside the bounds of the dictionary: {index}");
            _keyedCollection.RemoveAt(index);
        }

        /// <summary>
        /// Gets the value associated with the specified key.
        /// </summary>
        /// <param name="key">The key associated with the value to get.</param>
        public TValue GetValue(TKey key)
        {
            if (_keyedCollection.Contains(key) == false)
                throw new ArgumentException($"The given key is not present in the dictionary: {key}");
            var kvp = _keyedCollection[key];
            return kvp.Value;
        }

        /// <summary>
        /// Sets the value associated with the specified key.
        /// </summary>
        /// <param name="key">The key associated with the value to set.</param>
        /// <param name="value">The the value to set.</param>
        public void SetValue(TKey key, TValue value)
        {
            var kvp = new KeyValuePair<TKey, TValue>(key, value);
            var idx = IndexOf(key);
            if (idx > -1)
                _keyedCollection[idx] = kvp;
            else
                _keyedCollection.Add(kvp);
        }

        /// <summary>
        /// Tries to get value at specified key.
        /// </summary>
        /// <param name="key">The key associated with the value to find.</param>
        /// <param name="value">Found value.</param>
        /// <returns>true if value existed, false if not</returns>
        public bool TryGetValue(TKey key, out TValue value)
        {
            if (_keyedCollection.Contains(key))
            {
                value = _keyedCollection[key].Value;
                return true;
            }

            value = default;
            return false;
        }

        #endregion

        #region Sorting

        /// <summary>
        /// Sorts the keys.
        /// </summary>
        public void SortKeys()
        {
            _keyedCollection.SortByKeys();
        }

        /// <summary>
        /// Sorts the keys.
        /// </summary>
        /// <param name="comparer">The comparer.</param>
        public void SortKeys(IComparer<TKey> comparer)
        {
            _keyedCollection.SortByKeys(comparer);
        }

        /// <summary>
        /// Sorts the keys.
        /// </summary>
        /// <param name="comparison">The comparison.</param>
        public void SortKeys(Comparison<TKey> comparison)
        {
            _keyedCollection.SortByKeys(comparison);
        }

        /// <summary>
        /// Sorts the values.
        /// </summary>
        public void SortValues()
        {
            var comparer = Comparer<TValue>.Default;
            SortValues(comparer);
        }

        /// <summary>
        /// Sorts the values.
        /// </summary>
        /// <param name="comparer">The comparer.</param>
        public void SortValues(IComparer<TValue> comparer)
        {
            _keyedCollection.Sort((x, y) => comparer.Compare(x.Value, y.Value));
        }

        /// <summary>
        /// Sorts the values.
        /// </summary>
        /// <param name="comparison">The comparison.</param>
        public void SortValues(Comparison<TValue> comparison)
        {
            _keyedCollection.Sort((x, y) => comparison(x.Value, y.Value));
        }

        #endregion

        #region IDictionary<TKey, TValue>

        void IDictionary<TKey, TValue>.Add(TKey key, TValue value)
        {
            Add(key, value);
        }

        bool IDictionary<TKey, TValue>.ContainsKey(TKey key)
        {
            return ContainsKey(key);
        }

        ICollection<TKey> IDictionary<TKey, TValue>.Keys => Keys;

        bool IDictionary<TKey, TValue>.Remove(TKey key)
        {
            return Remove(key);
        }

        bool IDictionary<TKey, TValue>.TryGetValue(TKey key, out TValue value)
        {
            return TryGetValue(key, out value);
        }

        ICollection<TValue> IDictionary<TKey, TValue>.Values => Values;

        TValue IDictionary<TKey, TValue>.this[TKey key]
        {
            get => this[key];
            set => this[key] = value;
        }

        #endregion

        #region ICollection<KeyValuePair<TKey, TValue>>

        void ICollection<KeyValuePair<TKey, TValue>>.Add(KeyValuePair<TKey, TValue> item)
        {
            _keyedCollection.Add(item);
        }

        void ICollection<KeyValuePair<TKey, TValue>>.Clear()
        {
            _keyedCollection.Clear();
        }

        bool ICollection<KeyValuePair<TKey, TValue>>.Contains(KeyValuePair<TKey, TValue> item)
        {
            return _keyedCollection.Contains(item);
        }

        void ICollection<KeyValuePair<TKey, TValue>>.CopyTo(KeyValuePair<TKey, TValue>[] array, int arrayIndex)
        {
            _keyedCollection.CopyTo(array, arrayIndex);
        }

        int ICollection<KeyValuePair<TKey, TValue>>.Count => _keyedCollection.Count;

        bool ICollection<KeyValuePair<TKey, TValue>>.IsReadOnly => false;

        bool ICollection<KeyValuePair<TKey, TValue>>.Remove(KeyValuePair<TKey, TValue> item)
        {
            return _keyedCollection.Remove(item);
        }

        #endregion

        #region IEnumerable<KeyValuePair<TKey, TValue>>

        IEnumerator<KeyValuePair<TKey, TValue>> IEnumerable<KeyValuePair<TKey, TValue>>.GetEnumerator()
        {
            return GetEnumerator();
        }

        #endregion

        #region IEnumerable

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }

        #endregion

        #region IOrderedDictionary

        IDictionaryEnumerator IOrderedDictionary.GetEnumerator()
        {
            return new DictionaryEnumerator<TKey, TValue>(this);
        }

        void IOrderedDictionary.Insert(int index, object key, object value)
        {
            Insert(index, (TKey)key, (TValue)value);
        }

        void IOrderedDictionary.RemoveAt(int index)
        {
            RemoveAt(index);
        }

        object IOrderedDictionary.this[int index]
        {
            get => this[index];
            set => this[index] = (TValue)value;
        }

        #endregion

        #region IDictionary

        void IDictionary.Add(object key, object value)
        {
            Add((TKey)key, (TValue)value);
        }

        void IDictionary.Clear()
        {
            Clear();
        }

        bool IDictionary.Contains(object key)
        {
            return _keyedCollection.Contains((TKey)key);
        }

        IDictionaryEnumerator IDictionary.GetEnumerator()
        {
            return new DictionaryEnumerator<TKey, TValue>(this);
        }

        bool IDictionary.IsFixedSize => false;

        bool IDictionary.IsReadOnly => false;

        ICollection IDictionary.Keys => (ICollection)Keys;

        void IDictionary.Remove(object key)
        {
            Remove((TKey)key);
        }

        ICollection IDictionary.Values => (ICollection)Values;

        object IDictionary.this[object key]
        {
            get => this[(TKey)key];
            set => this[(TKey)key] = (TValue)value;
        }

        #endregion

        #region ICollection

        void ICollection.CopyTo(Array array, int index)
        {
            ((ICollection)_keyedCollection).CopyTo(array, index);
        }

        int ICollection.Count => ((ICollection)_keyedCollection).Count;

        bool ICollection.IsSynchronized => ((ICollection)_keyedCollection).IsSynchronized;

        object ICollection.SyncRoot => ((ICollection)_keyedCollection).SyncRoot;

        #endregion
    }

    [Serializable]
    internal class KeyedCollectionInternal<TKey, TItem> : KeyedCollection<TKey, TItem>
    {
        private const string DelegateNullExceptionMessage = "Delegate passed cannot be null";
        private Func<TItem, TKey> _getKeyForItemDelegate;

        public KeyedCollectionInternal(Func<TItem, TKey> getKeyForItemDelegate)
        {
            _getKeyForItemDelegate = getKeyForItemDelegate ?? throw new ArgumentNullException(DelegateNullExceptionMessage);
        }

        public KeyedCollectionInternal(Func<TItem, TKey> getKeyForItemDelegate, IEqualityComparer<TKey> comparer)
        : base(comparer)
        {
            _getKeyForItemDelegate = getKeyForItemDelegate ?? throw new ArgumentNullException(DelegateNullExceptionMessage);
        }

        protected override TKey GetKeyForItem(TItem item)
        {
            return _getKeyForItemDelegate(item);
        }

        public void SortByKeys()
        {
            var comparer = Comparer<TKey>.Default;
            SortByKeys(comparer);
        }

        public void SortByKeys(IComparer<TKey> keyComparer)
        {
            var comparer = new ComparerInternal<TItem>((x, y) => keyComparer.Compare(GetKeyForItem(x), GetKeyForItem(y)));
            Sort(comparer);
        }

        public void SortByKeys(Comparison<TKey> keyComparison)
        {
            var comparer = new ComparerInternal<TItem>((x, y) => keyComparison(GetKeyForItem(x), GetKeyForItem(y)));
            Sort(comparer);
        }

        public void Sort()
        {
            var comparer = Comparer<TItem>.Default;
            Sort(comparer);
        }

        public void Sort(Comparison<TItem> comparison)
        {
            var newComparer = new ComparerInternal<TItem>(comparison);
            Sort(newComparer);
        }

        public void Sort(IComparer<TItem> comparer)
        {
            if (Items is List<TItem> list)
                list.Sort(comparer);
        }
    }

    internal class ComparerInternal<T> : Comparer<T>
    {
        private readonly Comparison<T> _compareFunction;

        /// <inheritdoc />
        public ComparerInternal(Comparison<T> comparison)
        {
            _compareFunction = comparison ?? throw new ArgumentNullException(nameof(comparison));
        }

        /// <inheritdoc />
        public override int Compare(T arg1, T arg2)
        {
            return _compareFunction(arg1, arg2);
        }
    }

    /// <summary>
    /// The enumerator implementation for dictionary
    /// </summary>
    /// <typeparam name="TKey">The type of the key.</typeparam>
    /// <typeparam name="TValue">The type of the value.</typeparam>
    /// <seealso cref="System.Collections.IDictionaryEnumerator" />
    /// <seealso cref="System.IDisposable" />
    [Serializable]
    public class DictionaryEnumerator<TKey, TValue> : IDictionaryEnumerator, IDisposable
    {
        private readonly IEnumerator<KeyValuePair<TKey, TValue>> _impl;

        /// <inheritdoc />
        public void Dispose()
        {
            _impl.Dispose();
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="DictionaryEnumerator{TKey, TValue}"/> class.
        /// </summary>
        /// <param name="value">The value.</param>
        public DictionaryEnumerator(IDictionary<TKey, TValue> value)
        {
            _impl = value.GetEnumerator();
        }

        /// <inheritdoc />
        public void Reset()
        {
            _impl.Reset();
        }

        /// <inheritdoc />
        public bool MoveNext()
        {
            return _impl.MoveNext();
        }

        /// <inheritdoc />
        public DictionaryEntry Entry
        {
            get
            {
                var pair = _impl.Current;
                return new DictionaryEntry(pair.Key, pair.Value);
            }
        }

        /// <inheritdoc />
        public object Key => _impl.Current.Key;

        /// <inheritdoc />
        public object Value => _impl.Current.Value;

        /// <inheritdoc />
        public object Current => Entry;
    }
}

#endif
