// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;

namespace FlaxEditor.Gizmo
{
    /// <summary>
    /// Interface for editor viewports that can contain and use <see cref="EditorPrimitives"/>.
    /// </summary>
    [HideInEditor]
    public interface IEditorPrimitivesOwner
    {
        /// <summary>
        /// Draws the custom editor primitives.
        /// </summary>
        /// <param name="context">The GPU commands context.</param>
        /// <param name="renderContext">The rendering context.</param>
        /// <param name="target">The output texture to render to.</param>
        /// <param name="targetDepth">The scene depth buffer that can be used to z-buffering.</param>
        void DrawEditorPrimitives(GPUContext context, ref RenderContext renderContext, GPUTexture target, GPUTexture targetDepth);
    }

    /// <summary>
    /// In-build postFx used to render debug shapes, gizmo tools and other editor primitives to MSAA render target and composite it with the editor preview window.
    /// </summary>
    [HideInEditor]
    public sealed class EditorPrimitives : PostProcessEffect
    {
        /// <summary>
        /// The target viewport.
        /// </summary>
        public IEditorPrimitivesOwner Viewport;

        /// <inheritdoc />
        public EditorPrimitives()
        {
            Order = 100;
        }

        /// <inheritdoc />
        public override void Render(GPUContext context, ref RenderContext renderContext, GPUTexture input, GPUTexture output)
        {
            if (Viewport == null)
                throw new NullReferenceException();

            Profiler.BeginEventGPU("Editor Primitives");

            // Check if use MSAA
            var format = output.Format;
            var formatFeatures = GPUDevice.Instance.GetFormatFeatures(format);
            bool enableMsaa = formatFeatures.MSAALevelMax >= MSAALevel.X4 && Editor.Instance.Options.Options.Visual.EnableMSAAForDebugDraw;

            // Prepare
            var msaaLevel = enableMsaa ? MSAALevel.X4 : MSAALevel.None;
            var width = output.Width;
            var height = output.Height;
            var desc = GPUTextureDescription.New2D(width, height, format, GPUTextureFlags.RenderTarget | GPUTextureFlags.ShaderResource, 1, 1, msaaLevel);
            var target = RenderTargetPool.Get(ref desc);
            desc = GPUTextureDescription.New2D(width, height, PixelFormat.D24_UNorm_S8_UInt, GPUTextureFlags.DepthStencil, 1, 1, msaaLevel);
            var targetDepth = RenderTargetPool.Get(ref desc);

            // Copy frame and clear depth
            context.Draw(target, input);
            context.ClearDepth(targetDepth.View());
            context.SetViewport(width, height);
            context.SetRenderTarget(targetDepth.View(), target.View());

            // Draw gizmos and other editor primitives
            var renderList = RenderList.GetFromPool();
            var prevList = renderContext.List;
            renderContext.List = renderList;
            renderContext.View.Pass = DrawPass.GBuffer | DrawPass.Forward;
            try
            {
                Viewport.DrawEditorPrimitives(context, ref renderContext, target, targetDepth);
            }
            catch (Exception ex)
            {
                Editor.LogWarning(ex);
            }

            // Sort draw calls
            renderList.SortDrawCalls(ref renderContext, false, DrawCallsListType.GBuffer);
            renderList.SortDrawCalls(ref renderContext, false, DrawCallsListType.GBufferNoDecals);
            renderList.SortDrawCalls(ref renderContext, true, DrawCallsListType.Forward);

            // Perform the rendering
            renderContext.View.Pass = DrawPass.GBuffer;
            renderList.ExecuteDrawCalls(ref renderContext, DrawCallsListType.GBuffer);
            renderList.ExecuteDrawCalls(ref renderContext, DrawCallsListType.GBufferNoDecals);
            renderContext.View.Pass = DrawPass.Forward;
            renderList.ExecuteDrawCalls(ref renderContext, DrawCallsListType.Forward);

            // Resolve MSAA texture
            if (enableMsaa)
                context.ResolveMultisample(target, output);
            else
                context.Draw(output, target);

            // Cleanup
            RenderTargetPool.Release(targetDepth);
            RenderTargetPool.Release(target);
            RenderList.ReturnToPool(renderList);
            renderContext.List = prevList;

            Profiler.EndEventGPU();
        }
    }
}
