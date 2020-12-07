// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

namespace FlaxEngine
{
    /// <summary>
    /// Custom postFx which can modify final image by processing it with material based filters.
    /// The base class for all post process effects used by the graphics pipeline.
    /// Allows to extend frame rendering logic and apply custom effects such as outline, night vision, contrast etc.
    /// </summary>
    /// <remarks>
    /// Override this class and implement custom post fx logic.
    /// Use <b>MainRenderTask.Instance.CustomPostFx.Add(myPostFx)</b> to attach your script to rendering.
    /// Or add script to camera.
    /// </remarks>
    public abstract class PostProcessEffect : Script
    {
        /// <summary>
        /// Gets a value indicating whether this effect can be rendered.
        /// </summary>
        public virtual bool CanRender => Enabled;

        /// <summary>
        /// Gets a value indicating whether use a single render target as both input and output. Use this if your effect doesn't need to copy the input buffer to the output but can render directly to the single texture. Can be used to optimize game performance.
        /// </summary>
        public virtual bool UseSingleTarget => false;

        /// <summary>
        /// Gets the effect rendering location within rendering pipeline.
        /// </summary>
        public virtual PostProcessEffectLocation Location => PostProcessEffectLocation.Default;

        /// <summary>
        /// Gets the effect rendering order. Registered post effects are sorted before rendering (from the lowest order to the highest order).
        /// </summary>
        public virtual int Order => 0;

        /// <summary>
        /// Performs custom postFx rendering.
        /// </summary>
        /// <param name="context">The GPU commands context.</param>
        /// <param name="renderContext">The rendering context.</param>
        /// <param name="input">The input texture.</param>
        /// <param name="output">The output texture.</param>
        public abstract void Render(GPUContext context, ref RenderContext renderContext, GPUTexture input, GPUTexture output);

        internal static void FetchInfo(PostProcessEffect obj, out PostProcessEffectLocation location, out bool useSingleTarget)
        {
            location = obj.Location;
            useSingleTarget = obj.UseSingleTarget;
        }
    }
}
