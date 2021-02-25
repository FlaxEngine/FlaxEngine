// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine.GUI
{
    /// <summary>
    /// A common control used to present rendered frame in the UI.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    [HideInEditor]
    public class RenderOutputControl : ContainerControl
    {
        /// <summary>
        /// The default back buffer format used by the GUI controls presenting rendered frames.
        /// </summary>
        public static PixelFormat BackBufferFormat = PixelFormat.R8G8B8A8_UNorm;

        /// <summary>
        /// The resize check timeout (in seconds).
        /// </summary>
        public const float ResizeCheckTime = 0.9f;

        /// <summary>
        /// The task.
        /// </summary>
        protected SceneRenderTask _task;

        /// <summary>
        /// The back buffer.
        /// </summary>
        protected GPUTexture _backBuffer;

        private GPUTexture _backBufferOld;
        private int _oldBackbufferLiveTimeLeft;
        private float _resizeTime;
        private Int2? _customResolution;

        /// <summary>
        /// Gets the task.
        /// </summary>
        public SceneRenderTask Task => _task;

        /// <summary>
        /// Gets a value indicating whether render to that output only if parent window exists, otherwise false.
        /// </summary>
        public bool RenderOnlyWithWindow { get; set; } = true;

        /// <summary>
        /// Gets a value indicating whether keep aspect ratio of the backbuffer image, otherwise false.
        /// </summary>
        public bool KeepAspectRatio { get; set; } = false;

        /// <summary>
        /// Gets or sets the color of the tint used to color the backbuffer of the render output.
        /// </summary>
        public Color TintColor { get; set; } = Color.White;

        /// <summary>
        /// Gets or sets the brightness of the output.
        /// </summary>
        public float Brightness { get; set; } = 1.0f;

        /// <summary>
        /// Gets or sets the rendering resolution scale. Can be sued to upscale image or to downscale the rendering to save the performance.
        /// </summary>
        public float ResolutionScale { get; set; } = 1.0f;

        /// <summary>
        /// Gets or sets the custom resolution to use for the rendering.
        /// </summary>
        public Int2? CustomResolution
        {
            get => _customResolution;
            set
            {
                if (_customResolution.HasValue != value.HasValue || (_customResolution.HasValue && _customResolution.Value != value.Value))
                {
                    _customResolution = value;
                    SyncBackbufferSize();
                }
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="RenderOutputControl"/> class.
        /// </summary>
        /// <param name="task">The task. Cannot be null.</param>
        /// <exception cref="System.ArgumentNullException">Invalid task.</exception>
        public RenderOutputControl(SceneRenderTask task)
        {
            _task = task ?? throw new ArgumentNullException();

            _backBuffer = GPUDevice.Instance.CreateTexture();
            _resizeTime = ResizeCheckTime;

            _task.Output = _backBuffer;
            _task.End += OnEnd;

            Scripting.Update += OnUpdate;
        }

        private bool WalkTree(Control c)
        {
            while (c != null)
            {
                if (c is RootControl)
                    return false;
                if (c.Visible == false)
                    break;
                c = c.Parent;
            }
            return true;
        }

        /// <summary>
        /// Performs a check if rendering a current frame can be skipped (if control size is too small, has missing data, etc.).
        /// </summary>
        /// <returns>True if skip rendering, otherwise false.</returns>
        protected virtual bool CanSkipRendering()
        {
            // Disable task rendering if control is very small
            const float MinRenderSize = 4;
            if (Width < MinRenderSize || Height < MinRenderSize)
                return true;

            // Disable task rendering if control is not used in a window (has using ParentWindow)
            if (RenderOnlyWithWindow)
            {
                return WalkTree(Parent);
            }

            return false;
        }

        /// <summary>
        /// Called when ask rendering ends.
        /// </summary>
        /// <param name="task">The task.</param>
        /// <param name="context">The GPU execution context.</param>
        protected virtual void OnEnd(RenderTask task, GPUContext context)
        {
            // Check if was using old backbuffer
            if (_backBufferOld)
            {
                _oldBackbufferLiveTimeLeft--;
                if (_oldBackbufferLiveTimeLeft < 0)
                {
                    Object.Destroy(ref _backBufferOld);
                }
            }
        }

        private void OnUpdate()
        {
            if (_task == null)
                return;
            var deltaTime = Time.UnscaledDeltaTime;

            // Check if need to resize the output
            _resizeTime += deltaTime;
            if (_resizeTime >= ResizeCheckTime && Visible && Enabled)
            {
                _resizeTime = 0;
                SyncBackbufferSize();
            }

            // Check if skip rendering
            var wasEnabled = _task.Enabled;
            _task.Enabled = !CanSkipRendering();
            if (wasEnabled != _task.Enabled)
            {
                SyncBackbufferSize();
            }
        }

        /// <inheritdoc />
        public override void Draw()
        {
            var bounds = new Rectangle(Vector2.Zero, Size);

            // Draw background
            var backgroundColor = BackgroundColor;
            if (backgroundColor.A > 0.0f)
            {
                Render2D.FillRectangle(bounds, backgroundColor);
            }

            // Draw backbuffer texture
            var buffer = _backBufferOld ? _backBufferOld : _backBuffer;
            var color = TintColor.RGBMultiplied(Brightness);
            if (KeepAspectRatio)
            {
                float ratioX = bounds.Width / buffer.Width;
                float ratioY = bounds.Height / buffer.Height;
                float ratio = ratioX < ratioY ? ratioX : ratioY;
                bounds = new Rectangle((bounds.Width - buffer.Width * ratio) / 2, (bounds.Height - buffer.Height * ratio) / 2, buffer.Width * ratio, buffer.Height * ratio);
            }
            Render2D.DrawTexture(buffer, bounds, color);

            // Push clipping mask
            if (ClipChildren)
            {
                GetDesireClientArea(out var clientArea);
                Render2D.PushClip(ref clientArea);
            }

            DrawChildren();

            // Pop clipping mask
            if (ClipChildren)
            {
                Render2D.PopClip();
            }
        }

        /// <summary>
        /// Synchronizes size of the back buffer with the size of the control.
        /// </summary>
        public void SyncBackbufferSize()
        {
            float scale = ResolutionScale * Platform.DpiScale;
            int width = Mathf.CeilToInt(Width * scale);
            int height = Mathf.CeilToInt(Height * scale);
            if (_customResolution.HasValue)
            {
                width = _customResolution.Value.X;
                height = _customResolution.Value.Y;
            }
            if (_backBuffer == null || _backBuffer.Width == width && _backBuffer.Height == height)
                return;
            if (width < 1 || height < 1)
            {
                _backBuffer.ReleaseGPU();
                Object.Destroy(ref _backBufferOld);
                return;
            }

            // Cache old backbuffer to remove flickering effect
            if (_backBufferOld == null && _backBuffer.IsAllocated)
            {
                _backBufferOld = _backBuffer;
                _backBuffer = GPUDevice.Instance.CreateTexture();
            }

            // Set timeout to remove old buffer
            _oldBackbufferLiveTimeLeft = 3;

            // Resize backbuffer
            var desc = GPUTextureDescription.New2D(width, height, BackBufferFormat);
            _backBuffer.Init(ref desc);
            _task.Output = _backBuffer;
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            if (IsDisposing)
                return;

            // Cleanup
            Scripting.Update -= OnUpdate;
            if (_task != null)
            {
                _task.Enabled = false;
                //_task.CustomPostFx.Clear();
            }
            Object.Destroy(ref _backBuffer);
            Object.Destroy(ref _backBufferOld);
            Object.Destroy(ref _task);

            base.OnDestroy();
        }
    }
}
