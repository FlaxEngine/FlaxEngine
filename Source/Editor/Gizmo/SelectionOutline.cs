// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.Options;
using FlaxEditor.SceneGraph;
using FlaxEngine;

namespace FlaxEditor.Gizmo
{
    /// <summary>
    /// In-build postFx used to render outline for selected objects in editor.
    /// </summary>
    [HideInEditor]
    public class SelectionOutline : PostProcessEffect
    {
        private Material _outlineMaterial;
        private MaterialInstance _material;
        private Color _color0, _color1;
        private bool _enabled;
        private bool _useEditorOptions;

        /// <summary>
        /// The cached actors list used for drawing (reusable to reduce memory allocations). Always cleared before and after objects rendering.
        /// </summary>
        protected List<Actor> _actors;

        /// <summary>
        /// The selection getter.
        /// </summary>
        public Func<List<SceneGraphNode>> SelectionGetter;

        /// <summary>
        /// Gets or sets a value indicating whether show selection outline effect.
        /// </summary>
        public bool ShowSelectionOutline
        {
            get => _enabled;
            set => _enabled = value;
        }

        /// <summary>
        /// Gets or sets the selection outline first color (top of the screen-space gradient).
        /// </summary>
        public Color SelectionOutlineColor0
        {
            get => _color0;
            set => _color0 = value;
        }

        /// <summary>
        /// Gets or sets the selection outline second color (bottom of the screen-space gradient).
        /// </summary>
        public Color SelectionOutlineColor1
        {
            get => _color1;
            set => _color1 = value;
        }

        /// <summary>
        /// Gets or sets a value indicating whether use editor options for selection outline color and visibility. Otherwise, if disabled it can be controlled from code.
        /// </summary>
        public bool UseEditorOptions
        {
            get => _useEditorOptions;
            set
            {
                if (_useEditorOptions != value)
                {
                    var options = Editor.Instance.Options;

                    if (_useEditorOptions)
                        options.OptionsChanged -= OnOptionsChanged;

                    _useEditorOptions = value;

                    if (_useEditorOptions)
                        options.OptionsChanged += OnOptionsChanged;
                }
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="SelectionOutline"/> class.
        /// </summary>
        public SelectionOutline()
        {
            Order = -90000; // Draw before any other editor shapes (except grid gizmo)

            _outlineMaterial = FlaxEngine.Content.LoadAsyncInternal<Material>("Editor/Gizmo/SelectionOutlineMaterial");
            if (_outlineMaterial)
            {
                _material = _outlineMaterial.CreateVirtualInstance();
            }
            else
            {
                Editor.LogWarning("Failed to load gizmo selection outline material");
            }

            _useEditorOptions = true;
            var options = Editor.Instance.Options;
            options.OptionsChanged += OnOptionsChanged;
            OnOptionsChanged(options.Options);
        }

        private void OnOptionsChanged(EditorOptions options)
        {
            _enabled = options.Visual.ShowSelectionOutline;
            _color0 = options.Visual.SelectionOutlineColor0;
            _color1 = options.Visual.SelectionOutlineColor1;
        }

        /// <summary>
        /// Gets a value indicating whether this instance has data ready.
        /// </summary>
        protected bool HasDataReady => _enabled && _material && _outlineMaterial.IsLoaded;

        /// <inheritdoc />
        public override bool CanRender()
        {
            return _enabled && _material && _outlineMaterial.IsLoaded && SelectionGetter().Count > 0;
        }

        /// <inheritdoc />
        public override void Render(GPUContext context, ref RenderContext renderContext, GPUTexture input, GPUTexture output)
        {
            Profiler.BeginEventGPU("Selection Outline");

            // Pick a temporary depth buffer
            var desc = GPUTextureDescription.New2D(input.Width, input.Height, PixelFormat.D32_Float, GPUTextureFlags.DepthStencil | GPUTextureFlags.ShaderResource);
            var customDepth = RenderTargetPool.Get(ref desc);
            context.ClearDepth(customDepth.View());

            // Draw objects to depth buffer
            if (_actors == null)
                _actors = new List<Actor>();
            else
                _actors.Clear();
            DrawSelectionDepth(context, renderContext.Task, customDepth);
            _actors.Clear();

            var near = renderContext.View.Near;
            var far = renderContext.View.Far;
            var projection = renderContext.View.Projection;

            // Render outline
            _material.SetParameterValue("OutlineColor0", _color0);
            _material.SetParameterValue("OutlineColor1", _color1);
            _material.SetParameterValue("CustomDepth", customDepth);
            _material.SetParameterValue("ViewInfo", new Float4(1.0f / projection.M11, 1.0f / projection.M22, far / (far - near), (-far * near) / (far - near) / far));
            Renderer.DrawPostFxMaterial(context, ref renderContext, _material, output, input.View());

            // Cleanup
            RenderTargetPool.Release(customDepth);

            Profiler.EndEventGPU();
        }

        private void CollectActors(Actor actor)
        {
            _actors.Add(actor);

            for (int i = 0; i < actor.ChildrenCount; i++)
                CollectActors(actor.GetChild(i));
        }

        /// <summary>
        /// Draws the selected object to depth buffer.
        /// </summary>
        /// <param name="context">The context.</param>
        /// <param name="task">The task.</param>
        /// <param name="customDepth">The custom depth (output).</param>
        protected virtual void DrawSelectionDepth(GPUContext context, SceneRenderTask task, GPUTexture customDepth)
        {
            // Get selected actors
            var selection = SelectionGetter();
            _actors.Capacity = Mathf.NextPowerOfTwo(Mathf.Max(_actors.Capacity, selection.Count));
            for (int i = 0; i < selection.Count; i++)
            {
                if (selection[i] is ActorNode actorNode && actorNode.Actor != null)
                    CollectActors(actorNode.Actor);
            }
            if (_actors.Count == 0)
                return;

            // Render selected objects depth
            Renderer.DrawSceneDepth(context, task, customDepth, _actors);
        }
    }
}
