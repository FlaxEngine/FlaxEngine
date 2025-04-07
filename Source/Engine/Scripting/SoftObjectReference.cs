// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    /// <summary>
    /// The scripting object soft reference. Objects gets referenced on use (ID reference is resolving it).
    /// </summary>
    public struct SoftObjectReference : IComparable, IComparable<SoftObjectReference>
    {
        private Guid _id;
        private Object _object;

        /// <summary>
        /// Gets or sets the object identifier.
        /// </summary>
        public Guid ID
        {
            get => _id;
            set
            {
                if (_id == value)
                    return;
                _id = value;
                _object = null;
            }
        }

        /// <summary>
        /// Gets the object reference.
        /// </summary>
        /// <typeparam name="T">The object type.</typeparam>
        /// <returns>The resolved object or null.</returns>
        public T Get<T>() where T : Object
        {
            if (!_object)
                _object = Object.Find(ref _id, typeof(T));
            return _object as T;
        }

        /// <summary>
        /// Sets the object reference.
        /// </summary>
        /// <param name="obj">The object.</param>
        public void Set(Object obj)
        {
            _object = obj;
            _id = obj?.ID ?? Guid.Empty;
        }

        /// <inheritdoc />
        public override string ToString()
        {
            if (_object)
                return _object.ToString();
            return _id.ToString();
        }

        /// <inheritdoc />
        public override int GetHashCode()
        {
            return _id.GetHashCode();
        }

        /// <inheritdoc />
        public int CompareTo(object obj)
        {
            if (obj is SoftObjectReference other)
                return CompareTo(other);
            return 0;
        }

        /// <inheritdoc />
        public int CompareTo(SoftObjectReference other)
        {
            return _id.CompareTo(other._id);
        }
    }
}
