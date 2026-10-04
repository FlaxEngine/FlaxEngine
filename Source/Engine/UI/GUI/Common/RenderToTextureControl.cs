// Copyright (c) Wojciech Figat. All rights reserved.

namespace FlaxEngine.GUI
{
    /// <summary>
    /// UI container control that can render children to texture and display pre-cached texture instead of drawing children every frame. It can be also used to render part of UI to texture and use it in material or shader.
    /// </summary>
    [ActorToolbox("GUI")]
    public class RenderToTextureControl : ContainerControl
    {
        private bool _invalid, _redrawRegistered, _isDuringTextureDraw;
        private bool _autoSize = true, _constantInvalidate = false;
        private GPUTexture _texture;
        private Float2 _textureSize;
        private MaterialBase _drawMaterial;
        private MaterialInstance _drawMaterialInstance;
        private string _drawTextureParameterName = "Input";

        /// <summary>
        /// Gets the texture with cached children controls.
        /// </summary>
        public GPUTexture Texture => _texture;

        /// <summary>
        /// Gets or sets a value indicating whether automatically update size of texture when control dimensions gets changed.
        /// </summary>
        [EditorOrder(10)]
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
        [EditorOrder(20), VisibleIf(nameof(AutomaticTextureSize), true), Limit(0, 4096)]
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
        [EditorOrder(30)]
        public bool AutomaticInvalidate { get; set; } = true;

        /// <summary>
        /// Gets or sets the value whether cached texture data should be invalidated every frame (eg. when UI is animated). 
        /// </summary>
        [EditorOrder(40)]
        public bool ConstantInvalidate
        {
            get => _constantInvalidate;
            set
            {
                if (_constantInvalidate != value)
                {
                    _constantInvalidate = value;
                    if (value)
                    {
                        // Register for constant invalidation
                        Invalidate();
                    }
                    else if (_invalid && _redrawRegistered)
                    {
                        // Don't invalidate anymore
                        _redrawRegistered = false;
                        Scripting.Draw -= OnDraw;
                    }
                }
            }
        }

        /// <summary>
        /// Gets or sets the GUI material used to draw the cached texture to the screen.
        /// Can be used to post-process underlying GUI with a custom shader (eg. chromatic-aberration, blur or tint). If not set, simple texture copy is performed.
        /// Materials has to be created with GUI domain and a GPUTexture parameter (default name is "Input") to be used as a source texture.
        /// </summary>
        [EditorOrder(100)]
        public MaterialBase DrawMaterial
        {
            get => _drawMaterial;
            set
            {
                if (_drawMaterial != value)
                {
                    _drawMaterial = value;
                    Invalidate();
                }
            }
        }

        /// <summary>
        /// Gets or sets the name of the GPUTexture parameter on the <see cref="DrawMaterial"/> to use as a source texture.
        /// </summary>
        [EditorOrder(110), VisibleIf("HasDrawMaterial")]
        public string DrawTextureParameterName
        {
            get => _drawTextureParameterName;
            set
            {
                if (_drawTextureParameterName != value)
                {
                    _drawTextureParameterName = value;
                    Invalidate();
                }
            }
        }

#if FLAX_EDITOR
        private bool HasDrawMaterial => _drawMaterial != null;
#endif

        /// <summary>
        /// Invalidates the cached image of children controls and invokes the redraw to the texture.
        /// </summary>
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
            if (!ConstantInvalidate)
            {
                if (_redrawRegistered)
                {
                    _redrawRegistered = false;
                    Scripting.Draw -= OnDraw;
                }
                if (!_invalid)
                    return;
            }
            _invalid = false;

            if (!_texture)
            {
                _texture = new GPUTexture();
#if !BUILD_RELEASE
                _texture.Name = nameof(RenderToTextureControl);
#endif
            }
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
                _isDuringTextureDraw = false;
                Profiler.EndEventGPU();
            }
        }

        /// <inheritdoc />
        public override void Draw()
        {
            // Draw cached texture
            if (_texture && !_invalid && !_isDuringTextureDraw)
            {
                var bounds = new Rectangle(Float2.Zero, Size);

                // Background
                var backgroundColor = BackgroundColor;
                if (backgroundColor.A > 0.0f)
                    Render2D.FillRectangle(bounds, backgroundColor);

                if (_drawMaterial && !_drawMaterial.WaitForLoaded())
                {
                    // Blit with a custom material
                    if (!_drawMaterialInstance)
                        _drawMaterialInstance = Content.CreateVirtualAsset<MaterialInstance>();
                    _drawMaterialInstance.BaseMaterial = _drawMaterial;
                    if (!_drawMaterial.IsGUI)
                    {
                        Debug.Logger.LogHandler.LogWrite(LogType.Error, $"Cannot draw RenderToTextureControl contents because material '{_drawMaterial}' isn't GUI domain");
                        return;
                    }
                    var textureParam = _drawMaterialInstance.GetParameter(_drawTextureParameterName);
                    if (!textureParam)
                    {
                        Debug.Logger.LogHandler.LogWrite(LogType.Error, $"Cannot draw RenderToTextureControl contents because material '{_drawMaterial}' doesn't have parameter '{_drawTextureParameterName}'");
                        return;
                    }
                    if (textureParam.ParameterType != MaterialParameterType.GPUTexture)
                    {
                        Debug.Logger.LogHandler.LogWrite(LogType.Error, $"Cannot draw RenderToTextureControl contents because material '{_drawMaterial}''s parameter '{_drawTextureParameterName}' is not a GPUTexture");
                        return;
                    }
                    textureParam.Value = _texture;
                    Render2D.DrawMaterial(_drawMaterialInstance, bounds);
                }
                else
                {
                    // Simple texture draw
                    Render2D.DrawTexture(_texture, bounds);
                }

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
            _drawMaterial = null;
            Object.Destroy(ref _texture);
            Object.Destroy(ref _drawMaterialInstance);

            base.OnDestroy();
        }
    }
}
