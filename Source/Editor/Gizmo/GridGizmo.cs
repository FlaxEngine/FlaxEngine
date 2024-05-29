// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;
using System.Runtime.InteropServices;
using static FlaxEditor.Surface.Archetypes.Animation.StateMachineTransition;

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
                public Matrix WorldMatrix;
                public Matrix ViewProjectionMatrix;
                public Float4 GridColor;
                public Float3 ViewPos;
                public float Far;
                public float GridSize;
            }

            private Float3[] _vertices =
            {
                new Float3(0, 0, 0),
                new Float3(100, 0, 0),
                new Float3(100, 100, 0),
                new Float3(0, 100, 0),
            };

            private static readonly uint[] _triangles =
            {
                0, 2, 1, // Face front
                1, 3, 0,
            };

            private GPUBuffer _vertexBuffer;
            private GPUBuffer _indexBuffer;
            private GPUPipelineState _psCustom;
            private Shader _shader;

            public Shader Shader
            {
                get => _shader;
                set
                {
                    if (_shader != value)
                    {
                        _shader = value;
                        ReleaseShader();
                    }
                }
            }

            private IntPtr _debugDrawContext;

            public unsafe Renderer()
            {
                UseSingleTarget = true;
                Location = PostProcessEffectLocation.Default;

                // Create index buffer for custom geometry drawing
                _indexBuffer = new GPUBuffer();
                fixed (uint* ptr = _triangles)
                {
                    var desc = GPUBufferDescription.Index(sizeof(uint), _triangles.Length, new IntPtr(ptr));
                    _indexBuffer.Init(ref desc);
                }
                Shader = FlaxEngine.Content.LoadAsyncInternal<Shader>("Shaders/Grid");
            }

            ~Renderer()
            {
                if (_debugDrawContext != IntPtr.Zero)
                {
                    DebugDraw.FreeContext(_debugDrawContext);
                    _debugDrawContext = IntPtr.Zero;
                }

                ReleaseShader();
                Destroy(ref _vertexBuffer);
                Destroy(ref _indexBuffer);
            }

            private void ReleaseShader()
            {
                // Release resources using shader
                Destroy(ref _psCustom);
            }

            public override unsafe void Render(GPUContext context, ref RenderContext renderContext, GPUTexture input, GPUTexture output)
            {
                Profiler.BeginEventGPU("Editor Grid");

                if (Shader == null) return;

                // Here we perform custom rendering on top of the in-build drawing
                Vector3 camPos = renderContext.View.WorldPosition;
                float gridSize = renderContext.View.Far + 20000;
                _vertices = new Float3[]
                {
                    new Float3(-gridSize + camPos.X, 0, -gridSize + camPos.Z),
                    new Float3(gridSize + camPos.X, 0, gridSize + camPos.Z),
                    new Float3(-gridSize + camPos.X, 0, gridSize + camPos.Z),
                    new Float3(gridSize + camPos.X, 0, -gridSize + camPos.Z),
                };

                // Create vertex buffer for custom geometry drawing
                _vertexBuffer = new GPUBuffer();
                fixed (Float3* ptr = _vertices)
                {
                    var desc = GPUBufferDescription.Vertex(sizeof(Float3), _vertices.Length, new IntPtr(ptr));
                    _vertexBuffer.Init(ref desc);
                }

                // Setup missing resources
                if (_psCustom == null)
                {
                    _psCustom = new GPUPipelineState();
                    var desc = GPUPipelineState.Description.Default;
                    desc.BlendMode = BlendingMode.AlphaBlend;
                    desc.CullMode = CullMode.TwoSided;
                    desc.VS = Shader.GPU.GetVS("VS_Custom");
                    desc.PS = Shader.GPU.GetPS("PS_Custom");
                    _psCustom.Init(ref desc);
                }

                var options = Editor.Instance.Options.Options;

                // Set constant buffer data (memory copy is used under the hood to copy raw data from CPU to GPU memory)
                var cb = Shader.GPU.GetCB(0);
                if (cb != IntPtr.Zero)
                {
                    var data = new Data();
                    Matrix.Multiply(ref renderContext.View.View, ref renderContext.View.Projection, out var viewProjection);
                    data.WorldMatrix = Matrix.Identity;
                    Matrix.Transpose(ref viewProjection, out data.ViewProjectionMatrix);
                    data.ViewPos = renderContext.View.WorldPosition;
                    data.GridColor = options.Viewport.ViewportGridColor;
                    data.Far = renderContext.View.Far;
                    data.GridSize = options.Viewport.ViewportGridViewDistance;

                    context.UpdateCB(cb, new IntPtr(&data));
                }

                // // Draw geometry using custom Pixel Shader and Vertex Shader
                context.BindCB(0, cb);
                context.BindIB(_indexBuffer);
                context.BindVB(new[] { _vertexBuffer });
                context.SetState(_psCustom);
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
