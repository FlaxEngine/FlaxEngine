// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;
using System.Runtime.InteropServices;

namespace FlaxEditor.Gizmo
{
    /// <summary>
    /// Draws a grid to feel better world origin position and the world units.
    /// </summary>
    /// <seealso cref="FlaxEditor.Gizmo.GizmoBase" />
    [HideInEditor]
    public class GridGizmo : GizmoBase
    {
        [HideInEditor]
        private sealed class Renderer : PostProcessEffect
        {
            [StructLayout(LayoutKind.Sequential)]
            private struct Data
            {
                public Matrix ViewProjectionMatrix;
                public Float4 GridColor;
                public Float3 ViewPos;
                public float Far;
                public Float3 ViewOrigin;
                public float GridSize;
            }

            private static readonly uint[] _triangles =
            {
                0, 2, 1, // Face front
                1, 3, 0,
            };

            private GPUBuffer[] _vbs = new GPUBuffer[1];
            private GPUBuffer _vertexBuffer;
            private GPUBuffer _indexBuffer;
            private GPUPipelineState _psGrid;
            private Shader _shader;

            public Renderer()
            {
                UseSingleTarget = true;
                Location = PostProcessEffectLocation.Default;
                Order = -100000; // Draw before any other editor shapes
                _shader = FlaxEngine.Content.LoadAsyncInternal<Shader>("Shaders/Editor/Grid");
            }

            ~Renderer()
            {
                Destroy(ref _psGrid);
                Destroy(ref _vertexBuffer);
                Destroy(ref _indexBuffer);
                _shader = null;
            }

            public override unsafe void Render(GPUContext context, ref RenderContext renderContext, GPUTexture input, GPUTexture output)
            {
                if (_shader == null)
                    return;
                Profiler.BeginEventGPU("Editor Grid");

                var options = Editor.Instance.Options.Options;
                float gridSize = renderContext.View.Far + 20000;

                // Lazy-init resources
                if (_vertexBuffer == null)
                {
                    _vertexBuffer = new GPUBuffer();
                    var desc = GPUBufferDescription.Vertex(sizeof(Float3), 4);
                    _vertexBuffer.Init(ref desc);
                }
                if (_indexBuffer == null)
                {
                    _indexBuffer = new GPUBuffer();
                    fixed (uint* ptr = _triangles)
                    {
                        var desc = GPUBufferDescription.Index(sizeof(uint), _triangles.Length, new IntPtr(ptr));
                        _indexBuffer.Init(ref desc);
                    }
                }
                if (_psGrid == null)
                {
                    _psGrid = new GPUPipelineState();
                    var desc = GPUPipelineState.Description.Default;
                    desc.BlendMode = BlendingMode.AlphaBlend;
                    desc.CullMode = CullMode.TwoSided;
                    desc.VS = _shader.GPU.GetVS("VS_Grid");
                    desc.PS = _shader.GPU.GetPS("PS_Grid");
                    _psGrid.Init(ref desc);
                }

                // Update vertices of the plane
                // TODO: perf this operation in a Vertex Shader
                float y = 1.5f; // Add small bias to reduce Z-fighting with geometry at scene origin
                var vertices = new Float3[]
                {
                    new Float3(-gridSize, y, -gridSize),
                    new Float3(gridSize, y, gridSize),
                    new Float3(-gridSize, y, gridSize),
                    new Float3(gridSize, y, -gridSize),
                };
                fixed (Float3* ptr = vertices)
                {
                    context.UpdateBuffer(_vertexBuffer, new IntPtr(ptr), (uint)(sizeof(Float3) * vertices.Length));
                }

                // Update constant buffer data
                var cb = _shader.GPU.GetCB(0);
                if (cb != IntPtr.Zero)
                {
                    var data = new Data();
                    Matrix.Multiply(ref renderContext.View.View, ref renderContext.View.Projection, out var viewProjection);
                    Matrix.Transpose(ref viewProjection, out data.ViewProjectionMatrix);
                    data.ViewPos = renderContext.View.WorldPosition;
                    data.GridColor = options.Viewport.ViewportGridColor;
                    data.Far = renderContext.View.Far;
                    data.GridSize = options.Viewport.ViewportGridViewDistance;
                    data.ViewOrigin = renderContext.View.Origin;
                    context.UpdateCB(cb, new IntPtr(&data));
                }

                // Draw geometry using custom Pixel Shader and Vertex Shader
                context.BindCB(0, cb);
                context.BindIB(_indexBuffer);
                _vbs[0] = _vertexBuffer;
                context.BindVB(_vbs);
                context.SetState(_psGrid);
                context.SetRenderTarget(renderContext.Buffers.DepthBuffer.View(), input.View());
                context.DrawIndexed((uint)_triangles.Length);

                Profiler.EndEventGPU();
            }
        }

        private Renderer _renderer;

        /// <summary>
        /// Gets or sets a value indicating whether this <see cref="GridGizmo"/> is enabled.
        /// </summary>
        public bool Enabled
        {
            get => _renderer.Enabled;
            set
            {
                if (_renderer.Enabled != value)
                {
                    _renderer.Enabled = value;
                    EnabledChanged?.Invoke(this);
                }
            }
        }

        /// <summary>
        /// Occurs when enabled state gets changed.
        /// </summary>
        public event Action<GridGizmo> EnabledChanged;

        /// <summary>
        /// Initializes a new instance of the <see cref="GridGizmo"/> class.
        /// </summary>
        /// <param name="owner">The gizmos owner.</param>
        public GridGizmo(IGizmoOwner owner)
        : base(owner)
        {
            _renderer = new Renderer();
            owner.RenderTask.AddCustomPostFx(_renderer);
        }

        /// <summary>
        /// Destructor.
        /// </summary>
        ~GridGizmo()
        {
            FlaxEngine.Object.Destroy(ref _renderer);
        }
    }
}
