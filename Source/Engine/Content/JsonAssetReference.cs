// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Runtime.CompilerServices;

namespace FlaxEngine
{
    /// <summary>
    /// Json asset reference utility. References resource with a typed data type.
    /// </summary>
    /// <typeparam name="T">Type of the asset instance type.</typeparam>
#if FLAX_EDITOR
    [CustomEditor(typeof(FlaxEditor.CustomEditors.Editors.AssetRefEditor))]
#endif
    public struct JsonAssetReference<T> : IComparable, IComparable<JsonAssetReference<T>>, IEquatable<JsonAssetReference<T>>
    {
        /// <summary>
        /// Gets or sets the referenced asset.
        /// </summary>
        public JsonAsset Asset;

        /// <summary>
        /// Gets the instance of the serialized object from the json asset data. Cached internally.
        /// </summary>
        public T Instance => (T)Asset?.Instance;

        /// <summary>
        /// Initializes a new instance of the <see cref="JsonAssetReference{T}"/> structure.
        /// </summary>
        /// <param name="asset">The Json Asset.</param>
        public JsonAssetReference(JsonAsset asset)
        {
            Asset = asset;
        }

        /// <summary>
        /// Implicit cast operator.
        /// </summary>
        public static implicit operator JsonAsset(JsonAssetReference<T> value)
        {
            return value.Asset;
        }

        /// <summary>
        /// Implicit cast operator.
        /// </summary>
        public static implicit operator IntPtr(JsonAssetReference<T> value)
        {
            return Object.GetUnmanagedPtr(value.Asset);
        }

        /// <summary>
        /// Implicit cast operator.
        /// </summary>
        public static implicit operator JsonAssetReference<T>(JsonAsset value)
        {
            return new JsonAssetReference<T>(value);
        }

        /// <summary>
        /// Implicit cast operator.
        /// </summary>
        public static implicit operator JsonAssetReference<T>(IntPtr valuePtr)
        {
            return new JsonAssetReference<T>(Object.FromUnmanagedPtr(valuePtr) as JsonAsset);
        }

        /// <summary>
        /// Checks if the object exists (reference is not null and the unmanaged object pointer is valid).
        /// </summary>
        /// <param name="obj">The object to check.</param>
        /// <returns>True if object is valid, otherwise false.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static implicit operator bool(JsonAssetReference<T> obj)
        {
            return obj.Asset;
        }

        /// <summary>
        /// Checks whether the two objects are equal.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator ==(JsonAssetReference<T> left, JsonAssetReference<T> right)
        {
            return left.Asset == right.Asset;
        }

        /// <summary>
        /// Checks whether the two objects are not equal.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool operator !=(JsonAssetReference<T> left, JsonAssetReference<T> right)
        {
            return left.Asset != right.Asset;
        }

        /// <inheritdoc />
        public bool Equals(JsonAssetReference<T> other)
        {
            return Asset == other.Asset;
        }

        /// <inheritdoc />
        public int CompareTo(JsonAssetReference<T> other)
        {
            return Object.GetUnmanagedPtr(Asset).CompareTo(Object.GetUnmanagedPtr(other.Asset));
        }

        /// <inheritdoc />
        public override bool Equals(object obj)
        {
            return obj is JsonAssetReference<T> other && Asset == other.Asset;
        }

        /// <inheritdoc />
        public override string ToString()
        {
            return Asset?.ToString();
        }

        /// <inheritdoc />
        public int CompareTo(object obj)
        {
            return obj is JsonAssetReference<T> other ? CompareTo(other) : 1;
        }

        /// <inheritdoc />
        public override int GetHashCode()
        {
            return (Asset != null ? Asset.GetHashCode() : 0);
        }
    }
}
