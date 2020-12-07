// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using System.Collections.Generic;

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
        public static void DrawSceneDepth(GPUContext context, SceneRenderTask task, GPUTexture output, List<Actor> customActors)
        {
            Internal_DrawSceneDepth(FlaxEngine.Object.GetUnmanagedPtr(context), FlaxEngine.Object.GetUnmanagedPtr(task), FlaxEngine.Object.GetUnmanagedPtr(output), Utils.ExtractArrayFromList(customActors));
        }
    }
}
