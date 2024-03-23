// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

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
                Location = PostProcessEffectLocation.AfterAntiAliasingPass;
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

                var options = Editor.Instance.Options.Options;
                float space = options.Viewport.ViewportGridScale, size;
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

                float bigLineIntensity = 0.8f;
                Color bigColor = Color.Gray * bigLineIntensity;
                Color color = bigColor * 0.8f;
                int count = (int)(size / space);
                int midLine = count / 2;
                int bigLinesMod = count / 8;

                Vector3 start = new Vector3(0, 0, size * -0.5f);
                Vector3 end = new Vector3(0, 0, size * 0.5f);

                for (int i = 0; i <= count; i++)
                {
                    start.X = end.X = i * space + start.Z;
                    Color lineColor = color;
                    if (i == midLine)
                        lineColor = Color.Blue * bigLineIntensity;
                    else if (i % bigLinesMod == 0)
                        lineColor = bigColor;
                    DebugDraw.DrawLine(start, end, lineColor);
                }

                start = new Vector3(size * -0.5f, 0, 0);
                end = new Vector3(size * 0.5f, 0, 0);

                for (int i = 0; i <= count; i++)
                {
                    start.Z = end.Z = i * space + start.X;
                    Color lineColor = color;
                    if (i == midLine)
                        lineColor = Color.Red * bigLineIntensity;
                    else if (i % bigLinesMod == 0)
                        lineColor = bigColor;
                    DebugDraw.DrawLine(start, end, lineColor);
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
