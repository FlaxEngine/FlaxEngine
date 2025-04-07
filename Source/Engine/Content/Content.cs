// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Runtime.CompilerServices;

namespace FlaxEngine
{
    partial class Content
    {
        /// <summary>
        /// Loads asset to the Content Pool and holds it until it won't be referenced by any object. Returns null if asset is missing. Actual asset data loading is performed on a other thread in async.
        /// </summary>
        /// <param name="id">Asset unique ID.</param>
        /// <typeparam name="T">Type of the asset to load. Includes any asset types derived from the type.</typeparam>
        /// <returns>Asset instance if loaded, null otherwise.</returns>
        public static T LoadAsync<T>(Guid id) where T : Asset
        {
            return (T)LoadAsync(id, typeof(T));
        }

        /// <summary>
        /// Loads asset to the Content Pool and holds it until it won't be referenced by any object. Returns null if asset is missing. Actual asset data loading is performed on a other thread in async.
        /// </summary>
        /// <param name="id">Asset unique ID.</param>
        /// <returns>Asset instance if loaded, null otherwise</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Asset LoadAsync(Guid id)
        {
            return LoadAsync<Asset>(id);
        }

        /// <summary>
        /// Loads asset to the Content Pool and holds it until it won't be referenced by any object. Returns null if asset is missing. Actual asset data loading is performed on a other thread in async.
        /// </summary>
        /// <param name="path">Path to the asset.</param>
        /// <typeparam name="T">Type of the asset to load. Includes any asset types derived from the type.</typeparam>
        /// <returns>Asset instance if loaded, null otherwise.</returns>
        public static T LoadAsync<T>(string path) where T : Asset
        {
            return (T)LoadAsync(path, typeof(T));
        }

        /// <summary>
        /// Loads asset to the Content Pool and holds it until it won't be referenced by any object. Returns null if asset is missing. Actual asset data loading is performed on a other thread in async.
        /// </summary>
        /// <param name="path">Path to the asset.</param>
        /// <returns>Asset instance if loaded, null otherwise</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Asset LoadAsync(string path)
        {
            return LoadAsync(path, typeof(Asset));
        }

        /// <summary>
        /// Loads asset to the Content Pool and holds it until it won't be referenced by any object. Returns null if asset is missing. Actual asset data loading is performed on a other thread in async.
        /// </summary>
        /// <param name="internalPath">Internal path to the asset. Relative to the Engine startup folder.</param>
        /// <typeparam name="T">Type of the asset to load. Includes any asset types derived from the type.</typeparam>
        /// <returns>Asset instance if loaded, null otherwise.</returns>
        [HideInEditor]
        public static T LoadAsyncInternal<T>(string internalPath) where T : Asset
        {
            return (T)LoadAsyncInternal(internalPath, typeof(T));
        }

        /// <summary>
        /// Loads asset to the Content Pool and holds it until it won't be referenced by any object. Returns null if asset is missing. Actual asset data loading is performed on a other thread in async.
        /// </summary>
        /// <param name="internalPath">Internal path to the asset. Relative to the Engine startup folder.</param>
        /// <returns>Asset instance if loaded, null otherwise</returns>
        [HideInEditor]
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Asset LoadAsyncInternal(string internalPath)
        {
            return LoadAsyncInternal(internalPath, typeof(Asset));
        }

        /// <summary>
        /// Loads asset to the Content Pool and holds it until it won't be referenced by any object. Returns null if asset is missing. Actual asset data loading is performed on a other thread in async. The current thread execution is blocked until asset is loaded.
        /// Waits until asset will be loaded. It's equivalent to LoadAsync + WaitForLoaded.
        /// </summary>
        /// <param name="id">Asset unique ID.</param>
        /// <param name="timeoutInMilliseconds">Custom timeout value in milliseconds.</param>
        /// <returns>Asset instance if loaded, null otherwise</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Asset Load(Guid id, double timeoutInMilliseconds = 30000.0)
        {
            return Load<Asset>(id, timeoutInMilliseconds);
        }

        /// <summary>
        /// Loads asset to the Content Pool and holds it until it won't be referenced by any object. Returns null if asset is missing. Actual asset data loading is performed on a other thread in async. The current thread execution is blocked until asset is loaded.
        /// Waits until asset will be loaded. It's equivalent to LoadAsync + WaitForLoaded.
        /// </summary>
        /// <param name="path">Path to the asset.</param>
        /// <param name="timeoutInMilliseconds">Custom timeout value in milliseconds.</param>
        /// <returns>Asset instance if loaded, null otherwise</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Asset Load(string path, double timeoutInMilliseconds = 30000.0)
        {
            return Load<Asset>(path, timeoutInMilliseconds);
        }

        /// <summary>
        /// Loads asset to the Content Pool and holds it until it won't be referenced by any object. Returns null if asset is missing. Actual asset data loading is performed on a other thread in async.
        /// Waits until asset will be loaded. It's equivalent to LoadAsync + WaitForLoaded.
        /// </summary>
        /// <param name="internalPath">Internal path to the asset. Relative to the Engine startup folder.</param>
        /// <param name="timeoutInMilliseconds">Custom timeout value in milliseconds.</param>
        /// <returns>Asset instance if loaded, null otherwise</returns>
        [HideInEditor]
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Asset LoadInternal(string internalPath, double timeoutInMilliseconds = 30000.0)
        {
            return LoadInternal<Asset>(internalPath, timeoutInMilliseconds);
        }

        /// <summary>
        /// Loads asset to the Content Pool and holds it until it won't be referenced by any object. Returns null if asset is missing. Actual asset data loading is performed on a other thread in async.
        /// Waits until asset will be loaded. It's equivalent to LoadAsync + WaitForLoaded.
        /// </summary>
        /// <param name="id">Asset unique ID.</param>
        /// <param name="timeoutInMilliseconds">Custom timeout value in milliseconds.</param>
        /// <typeparam name="T">Type of the asset to load. Includes any asset types derived from the type.</typeparam>
        /// <returns>Asset instance if loaded, null otherwise</returns>
        public static T Load<T>(Guid id, double timeoutInMilliseconds = 30000.0) where T : Asset
        {
            var asset = LoadAsync<T>(id);
            if (asset && asset.WaitForLoaded(timeoutInMilliseconds) == false)
                return asset;
            return null;
        }

        /// <summary>
        /// Loads asset to the Content Pool and holds it until it won't be referenced by any object. Returns null if asset is missing. Actual asset data loading is performed on a other thread in async.
        /// Waits until asset will be loaded. It's equivalent to LoadAsync + WaitForLoaded.
        /// </summary>
        /// <param name="path">Path to the asset.</param>
        /// <param name="timeoutInMilliseconds">Custom timeout value in milliseconds.</param>
        /// <typeparam name="T">Type of the asset to load. Includes any asset types derived from the type.</typeparam>
        /// <returns>Asset instance if loaded, null otherwise</returns>
        public static T Load<T>(string path, double timeoutInMilliseconds = 30000.0) where T : Asset
        {
            var asset = LoadAsync<T>(path);
            if (asset && asset.WaitForLoaded(timeoutInMilliseconds) == false)
                return asset;
            return null;
        }

        /// <summary>
        /// Loads asset to the Content Pool and holds it until it won't be referenced by any object. Returns null if asset is missing. Actual asset data loading is performed on a other thread in async.
        /// Waits until asset will be loaded. It's equivalent to LoadAsync + WaitForLoaded.
        /// </summary>
        /// <param name="internalPath">Internal path to the asset. Relative to the Engine startup folder and without an asset file extension.</param>
        /// <param name="timeoutInMilliseconds">Custom timeout value in milliseconds.</param>
        /// <typeparam name="T">Type of the asset to load. Includes any asset types derived from the type.</typeparam>
        /// <returns>Asset instance if loaded, null otherwise</returns>
        [HideInEditor]
        public static T LoadInternal<T>(string internalPath, double timeoutInMilliseconds = 30000.0) where T : Asset
        {
            var asset = LoadAsyncInternal<T>(internalPath);
            if (asset && asset.WaitForLoaded(timeoutInMilliseconds) == false)
                return asset;
            return null;
        }

        /// <summary>
        /// Creates temporary and virtual asset of the given type.
        /// Virtual assets have limited usage but allow to use custom assets data at runtime.
        /// Virtual assets are temporary and exist until application exit.
        /// </summary>
        /// <typeparam name="T">Type of the asset to create. Includes any asset types derived from the type.</typeparam>
        /// <returns>Asset instance if created, null otherwise. See log for error message if need to.</returns>
        public static T CreateVirtualAsset<T>() where T : Asset
        {
            return (T)Internal_CreateVirtualAsset(typeof(T));
        }
    }
}
