// Copyright (c) Wojciech Figat. All rights reserved.

namespace FlaxEngine
{
    /// <summary>
    /// Draw method within this interface is used for <see cref="Render2D"/>.CallDrawing single DrawCall
    /// <remarks>Each frame new Queue is sent to GPU from this CPU bound method</remarks>
    /// </summary>
    /// <seealso cref="Render2D.CallDrawing(IDrawable,GPUContext,GPUTexture)"/>
    /// <seealso cref="PostProcessEffect.Render"/>
    public interface IDrawable
    {
        /// <summary>
        /// Render2D drawing methods should be used within this method during render phase to be visible. 
        /// </summary>
        void Draw();
    }
}
