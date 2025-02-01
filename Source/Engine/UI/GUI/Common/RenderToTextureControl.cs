// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

namespace FlaxEngine.GUI
{
    /// <summary>
    /// UI container control that can render children to texture and display pre-cached texture instead of drawing children every frame. It can be also used to render part of UI to texture and use it in material or shader.
    /// </summary>
    [ActorToolbox("GUI")]
    public class RenderToTextureControl : ContainerControl
    {
        private bool _invalid, _redrawRegistered, _isDuringTextureDraw;
        private bool _autoSize = true;
        private GPUTexture _texture;
        private Float2 _textureSize;

        /// <summary>
        /// Gets the texture with cached children controls.
        /// </summary>
        public GPUTexture Texture => _texture;

        /// <summary>
        /// Gets or sets a value indicating whether automatically update size of texture when control dimensions gets changed.
        /// </summary>
        [EditorOrder(10), Tooltip("If checked, size of the texture will be automatically updated when control dimensions gets changed.")]
        public bool AutomaticTextureSize
        {
            get => _autoSize;
            set
            {
                if (_autoSize == value)
                    return;
                _autoSize = value;
                if (_autoSize)
                    TextureSize = Size;
            }
        }

        /// <summary>
        /// Gets or sets the size of the texture (in pixels).
        /// </summary>
        [EditorOrder(20), VisibleIf("CanEditTextureSize"), Limit(0, 4096), Tooltip("The size of the texture (in pixels).")]
        public Float2 TextureSize
        {
            get => _textureSize;
            set
            {
                if (_textureSize == value)
                    return;
                _textureSize = value;
                Invalidate();
            }
        }

        /// <summary>
        /// Gets or sets the value whether cached texture data should be invalidated automatically (eg. when child control changes). 
        /// </summary>
        public bool AutomaticInvalidate { get; set; } = true;

#if FLAX_EDITOR
        private bool CanEditTextureSize => !_autoSize;
#endif
        /// <summary>
        /// Invalidates the cached image of children controls and invokes the redraw to the texture.
        /// </summary>
        [Tooltip("Invalidates the cached image of children controls and invokes the redraw to the texture.")]
        public void Invalidate()
        {
            _invalid = true;

            if (!_redrawRegistered)
            {
                _redrawRegistered = true;
                Scripting.Draw += OnDraw;
            }
        }

        private void OnDraw()
        {
            if (_redrawRegistered)
            {
                _redrawRegistered = false;
                Scripting.Draw -= OnDraw;
            }
            if (!_invalid)
                return;
            _invalid = false;

            if (!_texture)
                _texture = new GPUTexture();
            if (_texture.Size != _textureSize)
            {
                var desc = GPUTextureDescription.New2D((int)_textureSize.X, (int)_textureSize.Y, PixelFormat.R8G8B8A8_UNorm);
                if (_texture.Init(ref desc))
                {
                    Debug.Logger.LogHandler.LogWrite(LogType.Error, "Failed to allocate texture for RenderToTextureControl");
                    return;
                }
            }
            if (!_texture || !_texture.IsAllocated)
                return;

            Profiler.BeginEventGPU("RenderToTextureControl");
            var context = GPUDevice.Instance.MainContext;
            _isDuringTextureDraw = true;
            context.Clear(_texture.View(), Color.Transparent);
            Render2D.Begin(context, _texture);
            try
            {
                var scale = _textureSize / Size;
                Matrix3x3.Scaling(scale.X, scale.Y, 1.0f, out var scaleMatrix);
                Render2D.PushTransform(ref scaleMatrix);
                Draw();
                Render2D.PopTransform();
            }
            finally
            {
                Render2D.End();
            }
            _isDuringTextureDraw = false;
            Profiler.EndEventGPU();
        }

        /// <inheritdoc />
        public override void Draw()
        {
            // Draw cached texture
            if (_texture && !_invalid && !_isDuringTextureDraw)
            {
                var bounds = new Rectangle(Float2.Zero, Size);
                var backgroundColor = BackgroundColor;
                if (backgroundColor.A > 0.0f)
                    Render2D.FillRectangle(bounds, backgroundColor);
                Render2D.DrawTexture(_texture, bounds);
                return;
            }

            // Draw default UI directly
            base.Draw();
        }

        /// <inheritdoc />
        protected override void OnSizeChanged()
        {
            base.OnSizeChanged();

            if (_autoSize)
                TextureSize = Size;
        }

        /// <inheritdoc />
        public override void OnChildResized(Control control)
        {
            base.OnChildResized(control);

            if (AutomaticInvalidate)
                Invalidate();
        }

        /// <inheritdoc />
        public override void OnChildrenChanged()
        {
            base.OnChildrenChanged();

            if (AutomaticInvalidate)
                Invalidate();
        }

        /// <inheritdoc />
        protected override void PerformLayoutBeforeChildren()
        {
            base.PerformLayoutBeforeChildren();

            if (AutomaticInvalidate)
                Invalidate();
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _invalid = false;
            if (_redrawRegistered)
            {
                _redrawRegistered = false;
                Scripting.Draw -= OnDraw;
            }
            Object.Destroy(ref _texture);

            base.OnDestroy();
        }
    }
}
