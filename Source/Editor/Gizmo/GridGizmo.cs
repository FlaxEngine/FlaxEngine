// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;

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
            private IntPtr _debugDrawContext;

            public Renderer()
            {
                Order = -100;
                UseSingleTarget = true;
                Location = PostProcessEffectLocation.BeforeForwardPass;
            }

            ~Renderer()
            {
                if (_debugDrawContext != IntPtr.Zero)
                {
                    DebugDraw.FreeContext(_debugDrawContext);
                    _debugDrawContext = IntPtr.Zero;
                }
            }

            public override void Render(GPUContext context, ref RenderContext renderContext, GPUTexture input, GPUTexture output)
            {
                Profiler.BeginEventGPU("Editor Grid");

                if (_debugDrawContext == IntPtr.Zero)
                    _debugDrawContext = DebugDraw.AllocateContext();
                DebugDraw.SetContext(_debugDrawContext);
                DebugDraw.UpdateContext(_debugDrawContext, 1.0f / Mathf.Max(Engine.FramesPerSecond, 1));

                var viewPos = (Vector3)renderContext.View.Position;
                var plane = new Plane(Vector3.Zero, Vector3.UnitY);
                var dst = CollisionsHelper.DistancePlanePoint(ref plane, ref viewPos);

                float space = Editor.Instance.Options.Options.Viewport.ViewportGridScale, size;
                if (dst <= 500.0f)
                {
                    size = 8000;
                }
                else if (dst <= 2000.0f)
                {
                    space *= 2;
                    size = 8000;
                }
                else
                {
                    space *= 20;
                    size = 100000;
                }

                Color color = Color.Gray * 0.7f;
                int count = (int)(size / space);

                Vector3 start = new Vector3(0, 0, size * -0.5f);
                Vector3 end = new Vector3(0, 0, size * 0.5f);

                for (int i = 0; i <= count; i++)
                {
                    start.X = end.X = i * space + start.Z;
                    DebugDraw.DrawLine(start, end, color);
                }

                start = new Vector3(size * -0.5f, 0, 0);
                end = new Vector3(size * 0.5f, 0, 0);

                for (int i = 0; i <= count; i++)
                {
                    start.Z = end.Z = i * space + start.X;
                    DebugDraw.DrawLine(start, end, color);
                }

                DebugDraw.Draw(ref renderContext, input.View(), null, true);
                DebugDraw.SetContext(IntPtr.Zero);

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
