// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace FlaxEngine
{
    partial class Renderer
    {
        /// <summary>
        /// Draws scene objects depth (to the output Z buffer). The output must be depth texture to write hardware depth to it.
        /// </summary>
        /// <param name="context">The GPU commands context to use.</param>
        /// <param name="task">Render task to use it's view description and the render buffers.</param>
        /// <param name="output">The output texture. Must be valid and created.</param>
        /// <param name="customActors">The custom set of actors to render. If empty, the loaded scenes will be rendered.</param>
        [Unmanaged]
        public static void DrawSceneDepth(GPUContext context, SceneRenderTask task, GPUTexture output, List<Actor> customActors)
        {
            Actor[] temp = null;
            int tempCount = 0;
            if (customActors != null && customActors.Count != 0)
            {
                temp = CollectionsMarshal.AsSpan(customActors).ToArray(); // FIXME
                tempCount = temp.Length;
            }
            Internal_DrawSceneDepth(FlaxEngine.Object.GetUnmanagedPtr(context), FlaxEngine.Object.GetUnmanagedPtr(task), FlaxEngine.Object.GetUnmanagedPtr(output), temp, ref tempCount);
        }

        /// <summary>
        /// Invoked drawing of the scene objects (collects draw calls into RenderList for a given RenderContext).
        /// </summary>
        /// <param name="renderContext">The rendering context.</param>
        /// <param name="customActors">The custom set of actors to render. If empty, the loaded scenes will be rendered.</param>
        [Unmanaged]
        public static void DrawActors(ref RenderContext renderContext, List<Actor> customActors)
        {
            Actor[] temp = null;
            int tempCount = 0;
            if (customActors != null && customActors.Count != 0)
            {
                temp = CollectionsMarshal.AsSpan(customActors).ToArray(); // FIXME
                tempCount = temp.Length;
            }
            Internal_DrawActors(ref renderContext, temp, ref tempCount);
        }
    }
}
