// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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
        /// Finds all the scripts of the given type in all the loaded scenes.
        /// </summary>
        /// <typeparam name="T">Type of the object.</typeparam>
        /// <returns>Found scripts list.</returns>
        public static T[] GetScripts<T>() where T : Script
        {
            var scripts = GetScripts(typeof(T));
            var result = new T[scripts.Length];
            for (int i = 0; i < scripts.Length; i++)
                result[i] = scripts[i] as T;
            return result;
        }

        /// <summary>
        /// Finds all the actors of the given type in all the loaded scenes.
        /// </summary>
        /// <typeparam name="T">Type of the object.</typeparam>
        /// <returns>Found actors list.</returns>
        public static T[] GetActors<T>() where T : Actor
        {
            var actors = GetActors(typeof(T));
            var result = new T[actors.Length];
            for (int i = 0; i < actors.Length; i++)
                result[i] = actors[i] as T;
            return result;
        }
    }
}
