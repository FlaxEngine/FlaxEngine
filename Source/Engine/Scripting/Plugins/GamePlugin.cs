// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

namespace FlaxEngine
{
    /// <summary>
    /// Base class for all plugins used at runtime in game.
    /// </summary>
    /// <remarks>
    /// Plugins should have a public and parameter-less constructor.
    /// </remarks>
    /// <seealso cref="FlaxEngine.Plugin" />
    public abstract class GamePlugin : Plugin
    {
#if FLAX_EDITOR
        /// <summary>
        /// Event called during game cooking in Editor to collect any assets that this plugin uses. Can be used to inject content for plugins.
        /// </summary>
        /// <param name="assets">The result assets list (always valid).</param>
        public virtual void OnCollectAssets(System.Collections.Generic.List<System.Guid> assets)
        {
        }
#endif
    }
}
