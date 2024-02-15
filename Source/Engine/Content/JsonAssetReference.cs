// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    /// <summary>
    /// Json asset reference utility. References resource with a typed data type.
    /// </summary>
    /// <typeparam name="T">Type of the asset instance type.</typeparam>
#if FLAX_EDITOR
    [CustomEditor(typeof(FlaxEditor.CustomEditors.Editors.AssetRefEditor))]
#endif
    public struct JsonAssetReference<T>
    {
        /// <summary>
        /// Gets or sets the referenced asset.
        /// </summary>
        public JsonAsset Asset;

        /// <summary>
        /// Gets the instance of the Json Asset. Null if unset.
        /// </summary>
        /// <returns>instance of the Json Asset or null if unset.</returns>
        public T Get()
        {
            return (T)Asset?.Instance;
        }

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

        /// <inheritdoc />
        public override string ToString()
        {
            return Asset?.ToString();
        }

        /// <inheritdoc />
        public override int GetHashCode()
        {
            return Asset?.GetHashCode() ?? 0;
        }
    }
}
