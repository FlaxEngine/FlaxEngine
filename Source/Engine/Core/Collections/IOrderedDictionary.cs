// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.Collections.Specialized;

namespace FlaxEngine.Collections
{
    /// <summary>
    /// Interface for a dictionary object that allows rapid hash lookups using keys, but also maintains the key insertion order so that values can be retrieved by key index.
    /// </summary>
    /// <typeparam name="TKey">The type of keys in the dictionary.</typeparam>
    /// <typeparam name="TValue">The type of values in the dictionary.</typeparam>
    public interface IOrderedDictionary<TKey, TValue> : IDictionary<TKey, TValue>, IOrderedDictionary
    {
        /// <summary>
        /// Gets or sets the element at the specified index.
        /// </summary>
        /// <param name="index">The index of the element to get or set.</param>
        /// <returns>The element at the specified index.</returns>
        new TValue this[int index] { get; set; }

        /// <summary>
        /// Gets or sets the element with the specified key.
        /// </summary>
        /// <param name="key">The key of the element to get or set.</param>
        /// <returns>The element with the specified key.</returns>
        new TValue this[TKey key] { get; set; }

        /// <summary>
        /// Gets the number of elements contained in the collection.
        /// </summary>
        new int Count { get; }

        /// <summary>
        /// Gets the collection of the keys.
        /// </summary>
        new ICollection<TKey> Keys { get; }

        /// <summary>
        /// Gets the collection of the values.
        /// </summary>
        new ICollection<TValue> Values { get; }

        /// <summary>
        /// Adds an element with the provided key and value to the collection.
        /// </summary>
        /// <param name="key">The object to use as the key of the element to add.</param>
        /// <param name="value">The object to use as the value of the element to add.</param>
        new void Add(TKey key, TValue value);

        /// <summary>
        /// Removes all items from the collection.
        /// </summary>
        new void Clear();

        /// <summary>
        /// Inserts the item at the specified index.
        /// </summary>
        /// <param name="index">The index.</param>
        /// <param name="key">The object to use as the key of the element to add.</param>
        /// <param name="value">The object to use as the value of the element to add.</param>
        void Insert(int index, TKey key, TValue value);

        /// <summary>
        /// Determines whether an element is in the collection.
        /// </summary>
        /// <param name="key">The object to locate in the current dictionary. The element to locate can be null for reference types.</param>
        /// <returns>The index of the item.</returns>
        int IndexOf(TKey key);

        /// <summary>
        /// Determines whether the dictionary contains the specified value.
        /// </summary>
        /// <param name="value">The value to check.</param>
        /// <returns><c>true</c> if the dictionary contains the specified value; otherwise, <c>false</c>.</returns>
        bool ContainsValue(TValue value);

        /// <summary>
        /// Determines whether the dictionary contains the specified value.
        /// </summary>
        /// <param name="value">The value to check.</param>
        /// <param name="comparer">The equality comparer.</param>
        /// <returns><c>true</c> if the dictionary contains the specified value; otherwise, <c>false</c>.</returns>
        bool ContainsValue(TValue value, IEqualityComparer<TValue> comparer);

        /// <summary>
        /// Determines whether the dictionary contains the specified key.
        /// </summary>
        /// <param name="key">The key to check.</param>
        /// <returns><c>true</c> if the dictionary contains the specified key; otherwise, <c>false</c>.</returns>
        new bool ContainsKey(TKey key);

        /// <summary>
        /// Returns an enumerator that iterates through the collection.
        /// </summary>
        /// <returns>An enumerator that can be used to iterate through the collection.</returns>
        new IEnumerator<KeyValuePair<TKey, TValue>> GetEnumerator();

        /// <summary>
        /// Removes the element with the specified key from the collection.
        /// </summary>
        /// <param name="key">The key of the element to remove.</param>
        /// <returns><see langword="true" /> if the element is successfully removed; otherwise, <see langword="false" />. This method also returns <see langword="false" /> if <paramref name="key" /> was not found in the original collection.</returns>
        new bool Remove(TKey key);

        /// <summary>
        /// Removes the element at the specified index.
        /// </summary>
        /// <param name="index">The zero-based index of the element to remove.</param>
        new void RemoveAt(int index);

        /// <summary>
        /// Gets the value associated with the specified key.
        /// </summary>
        /// <param name="key">The key whose value to get.</param>
        /// <param name="value">When this method returns, the value associated with the specified key, if the key is found; otherwise, the default value for the type of the <paramref name="value" /> parameter. This parameter is passed uninitialized.</param>
        /// <returns><see langword="true" /> if the object that implements collection contains an element with the specified key; otherwise, <see langword="false" />.</returns>
        new bool TryGetValue(TKey key, out TValue value);

        /// <summary>
        /// Gets the value by the key.
        /// </summary>
        /// <param name="key">The key.</param>
        /// <returns>The value.</returns>
        TValue GetValue(TKey key);

        /// <summary>
        /// Sets the 0 by the key.
        /// </summary>
        /// <param name="key">The key.</param>
        /// <param name="value">The value.</param>
        void SetValue(TKey key, TValue value);

        /// <summary>
        /// Gets the item at the specified index.
        /// </summary>
        /// <param name="index">The index.</param>
        /// <returns>The key-value pair.</returns>
        KeyValuePair<TKey, TValue> GetItem(int index);

        /// <summary>
        /// Sets the value.
        /// </summary>
        /// <param name="index">The index.</param>
        /// <param name="value">The value.</param>
        void SetItem(int index, TValue value);
    }
}
