// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    partial class Level
    {
        /// <summary>
        /// Unloads all active scenes and loads the given scene (in the background).
        /// </summary>
        /// <param name="sceneAssetId">The scene asset identifier (scene to load).</param>
        /// <returns>True if action fails (given asset is not a scene asset, missing data, scene loading error), otherwise false.</returns>
        public static bool ChangeSceneAsync(Guid sceneAssetId)
        {
            UnloadAllScenesAsync();
            return LoadSceneAsync(sceneAssetId);
        }

        /// <summary>
        /// Unloads all active scenes and loads the given scene (in the background).
        /// </summary>
        /// <param name="sceneAsset">The asset with the scene to load.</param>
        /// <returns>True if action fails (given asset is not a scene asset, missing data, scene loading error), otherwise false.</returns>
        public static bool ChangeSceneAsync(SceneReference sceneAsset)
        {
            return ChangeSceneAsync(sceneAsset.ID);
        }

        /// <summary>
        /// Loads scene from the asset.
        /// </summary>
        /// <param name="sceneAsset">The asset with the scene to load.</param>
        /// <returns>True if action fails (given asset is not a scene asset, missing data, scene loading error), otherwise false.</returns>
        public static bool LoadScene(SceneReference sceneAsset)
        {
            return LoadScene(sceneAsset.ID);
        }

        /// <summary>
        /// Loads scene from the asset. Done in the background.
        /// </summary>
        /// <param name="sceneAsset">The asset with the scene to load.</param>
        /// <returns>True if failed (given asset is not a scene asset, missing data), otherwise false.</returns>
        public static bool LoadSceneAsync(SceneReference sceneAsset)
        {
            return LoadSceneAsync(sceneAsset.ID);
        }

        /// <summary>
        /// Tries to find script of the given type in all loaded scenes.
        /// </summary>
        /// <typeparam name="T">Type of the object.</typeparam>
        /// <returns>Found script or null.</returns>
        public static T FindScript<T>() where T : Script
        {
            return FindScript(typeof(T)) as T;
        }

        /// <summary>
        /// Tries to find actor of the given type in all loaded scenes.
        /// </summary>
        /// <typeparam name="T">Type of the object.</typeparam>
        /// <returns>Found actor or null.</returns>
        public static T FindActor<T>() where T : Actor
        {
            return FindActor(typeof(T)) as T;
        }

        /// <summary>
        /// Tries to find actor of the given type and name in all loaded scenes.
        /// </summary>
        /// <param name="name">Name of the object.</param>
        /// <typeparam name="T">Type of the object.</typeparam>
        /// <returns>Found actor or null.</returns>
        public static T FindActor<T>(string name) where T : Actor
        {
            return FindActor(typeof(T), name) as T;
        }

        /// <summary>
        /// Tries to find actor of the given type and tag in a root actor or all loaded scenes.
        /// </summary>
        /// <param name="tag">A tag on the object.</param>
        /// <param name="root">The custom root actor to start searching from (hierarchical), otherwise null to search all loaded scenes.</param>
        /// <typeparam name="T">Type of the object.</typeparam>
        /// <returns>Found actor or null.</returns>
        public static T FindActor<T>(Tag tag, Actor root = null) where T : Actor
        {
            return FindActor(typeof(T), tag, root) as T;
        }

        /// <summary>
        /// Tries to find actor with the given ID in all loaded scenes. It's very fast O(1) lookup.
        /// </summary>
        /// <typeparam name="T">Type of the object.</typeparam>
        /// <param name="id">The id.</param>
        /// <returns>Found actor or null.</returns>
        public static T FindActor<T>(ref Guid id) where T : Actor
        {
            return FindActor(id) as T;
        }

        /// <summary>
        /// Finds all the scripts of the given type in an actor or all the loaded scenes.
        /// </summary>
        /// <typeparam name="T">Type of the object.</typeparam>
        /// <param name="root">The root to find scripts. If null, will search in all scenes</param>
        /// <returns>Found scripts list.</returns>
        public static T[] GetScripts<T>(Actor root = null) where T : Script
        {
            var scripts = GetScripts(typeof(T), root);
            if (typeof(T) == typeof(Script))
                return (T[])scripts;
            if (scripts.Length == 0)
                return Array.Empty<T>();
            var result = new T[scripts.Length];
            for (int i = 0; i < scripts.Length; i++)
                result[i] = scripts[i] as T;
            return result;
        }

        /// <summary>
        /// Finds all the actors of the given type in all the loaded scenes.
        /// </summary>
        /// <typeparam name="T">Type of the object.</typeparam>
        /// <param name="activeOnly">Finds only active actors.</param>
        /// <returns>Found actors list.</returns>
        public static T[] GetActors<T>(bool activeOnly = false) where T : Actor
        {
            var actors = GetActors(typeof(T), activeOnly);
            if (typeof(T) == typeof(Actor))
                return (T[])actors;
            if (actors.Length == 0)
                return Array.Empty<T>();
            var result = new T[actors.Length];
            for (int i = 0; i < actors.Length; i++)
                result[i] = actors[i] as T;
            return result;
        }
    }
}
