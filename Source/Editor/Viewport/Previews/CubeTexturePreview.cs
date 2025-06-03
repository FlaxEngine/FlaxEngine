// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.Viewport.Widgets;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Viewport.Previews
{
    /// <summary>
    /// Cube Texture asset preview editor viewport.
    /// </summary>
    /// <seealso cref="FlaxEditor.Viewport.Previews.MaterialPreview" />
    public class CubeTexturePreview : MaterialPreview
    {
        private ChannelFlags _channelFlags = ChannelFlags.All;
        private bool _usePointSampler = false;
        private float _mipLevel = -1;
        private ContextMenu _mipWidgetMenu;
        private ContextMenuButton _filterWidgetPointButton;
        private ContextMenuButton _filterWidgetLinearButton;

        /// <summary>
        /// The preview material instance used to draw texture.
        /// </summary>
        protected MaterialInstance _previewMaterial;

        /// <summary>
        /// Sets the cube texture to preview.
        /// </summary>
        /// <value>
        /// The cube texture.
        /// </value>
        public CubeTexture CubeTexture
        {
            set
            {
                // Prepare material and assign texture asset as a parameter
                if (_previewMaterial == null || _previewMaterial.WaitForLoaded())
                {
                    // Error
                    Editor.LogError("Cannot load preview material.");
                    return;
                }

                var baseMaterial = _previewMaterial.BaseMaterial;
                if (baseMaterial == null || baseMaterial.WaitForLoaded())
                {
                    // Error
                    Editor.LogError("Cannot load base material for preview material.");
                    return;
                }

                _previewMaterial.SetParameterValue("CubeTexture", value);
            }
        }

        /// <summary>
        /// Gets or sets the view channels to show.
        /// </summary>
        public ChannelFlags ViewChannels
        {
            get => _channelFlags;
            set
            {
                if (_channelFlags != value)
                {
                    _channelFlags = value;
                    UpdateMask();
                }
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether use point sampler when drawing the texture. The default value is false.
        /// </summary>
        public bool UsePointSampler
        {
            get => _usePointSampler;
            set
            {
                if (_usePointSampler != value)
                {
                    _usePointSampler = value;
                    _previewMaterial.SetParameterValue("PointSampler", value);
                }
            }
        }

        /// <summary>
        /// Gets or sets the mip level to show. The default value is -1.
        /// </summary>
        public float MipLevel
        {
            get => _mipLevel;
            set
            {
                if (!Mathf.NearEqual(_mipLevel, value))
                {
                    _mipLevel = value;
                    _previewMaterial.SetParameterValue("Mip", value);
                }
            }
        }

        /// <inheritdoc />
        public CubeTexturePreview(bool useWidgets)
        : base(useWidgets)
        {
            // Create virtual material material
            _previewMaterial = FlaxEngine.Content.CreateVirtualAsset<MaterialInstance>();
            if (_previewMaterial != null)
                _previewMaterial.BaseMaterial = FlaxEngine.Content.LoadAsyncInternal<Material>("Editor/CubeTexturePreviewMaterial");

            // Add widgets
            if (useWidgets)
            {
                // Channels widget
                var channelsWidget = new ViewportWidgetsContainer(ViewportWidgetLocation.UpperLeft);
                //
                var channelR = new ViewportWidgetButton("R", SpriteHandle.Invalid, null, true)
                {
                    Checked = true,
                    TooltipText = "Show/hide texture red channel",
                    Parent = channelsWidget
                };
                channelR.Toggled += button => ViewChannels = button.Checked ? ViewChannels | ChannelFlags.Red : (ViewChannels & ~ChannelFlags.Red);
                var channelG = new ViewportWidgetButton("G", SpriteHandle.Invalid, null, true)
                {
                    Checked = true,
                    TooltipText = "Show/hide texture green channel",
                    Parent = channelsWidget
                };
                channelG.Toggled += button => ViewChannels = button.Checked ? ViewChannels | ChannelFlags.Green : (ViewChannels & ~ChannelFlags.Green);
                var channelB = new ViewportWidgetButton("B", SpriteHandle.Invalid, null, true)
                {
                    Checked = true,
                    TooltipText = "Show/hide texture blue channel",
                    Parent = channelsWidget
                };
                channelB.Toggled += button => ViewChannels = button.Checked ? ViewChannels | ChannelFlags.Blue : (ViewChannels & ~ChannelFlags.Blue);
                var channelA = new ViewportWidgetButton("A", SpriteHandle.Invalid, null, true)
                {
                    Checked = true,
                    TooltipText = "Show/hide texture alpha channel",
                    Parent = channelsWidget
                };
                channelA.Toggled += button => ViewChannels = button.Checked ? ViewChannels | ChannelFlags.Alpha : (ViewChannels & ~ChannelFlags.Alpha);
                //
                channelsWidget.Parent = this;

                // Mip widget
                var mipWidget = new ViewportWidgetsContainer(ViewportWidgetLocation.UpperLeft);
                _mipWidgetMenu = new ContextMenu();
                _mipWidgetMenu.VisibleChanged += OnMipWidgetMenuOnVisibleChanged;
                var mipWidgetButton = new ViewportWidgetButton("Mip", SpriteHandle.Invalid, _mipWidgetMenu)
                {
                    TooltipText = "The mip level to show. The default is -1.",
                    Parent = mipWidget
                };
                //
                mipWidget.Parent = this;

                // Filter widget
                var filterWidget = new ViewportWidgetsContainer(ViewportWidgetLocation.UpperLeft);
                var filterWidgetMenu = new ContextMenu();
                filterWidgetMenu.VisibleChanged += OnFilterWidgetMenuVisibleChanged;
                _filterWidgetPointButton = filterWidgetMenu.AddButton("Point", () => UsePointSampler = true);
                _filterWidgetLinearButton = filterWidgetMenu.AddButton("Linear", () => UsePointSampler = false);
                var filterWidgetButton = new ViewportWidgetButton("Filter", SpriteHandle.Invalid, filterWidgetMenu)
                {
                    TooltipText = "The texture preview filtering mode. The default is Linear.",
                    Parent = filterWidget
                };
                //
                filterWidget.Parent = this;
            }

            // Link it
            Material = _previewMaterial;
        }

        private void OnFilterWidgetMenuVisibleChanged(Control control)
        {
            if (!control.Visible)
                return;

            _filterWidgetPointButton.Checked = UsePointSampler;
            _filterWidgetLinearButton.Checked = !UsePointSampler;
        }

        private void OnMipWidgetMenuOnVisibleChanged(Control control)
        {
            if (!control.Visible)
                return;

            var textureObj = _previewMaterial.GetParameterValue("CubeTexture");

            if (textureObj is TextureBase texture && !texture.WaitForLoaded())
            {
                _mipWidgetMenu.ItemsContainer.DisposeChildren();
                var mipLevels = texture.MipLevels;
                for (int i = -1; i < mipLevels; i++)
                {
                    var button = _mipWidgetMenu.AddButton(i.ToString(), OnMipWidgetClicked);
                    button.Tag = i;
                    button.Checked = Mathf.Abs(MipLevel - i) < 0.9f;
                }
            }
        }

        private void OnMipWidgetClicked(ContextMenuButton button)
        {
            MipLevel = (int)button.Tag;
        }

        private void UpdateMask()
        {
            Vector4 mask = Vector4.One;
            if ((_channelFlags & ChannelFlags.Red) == 0)
                mask.X = 0;
            if ((_channelFlags & ChannelFlags.Green) == 0)
                mask.Y = 0;
            if ((_channelFlags & ChannelFlags.Blue) == 0)
                mask.Z = 0;
            if ((_channelFlags & ChannelFlags.Alpha) == 0)
                mask.W = 0;
            _previewMaterial.SetParameterValue("Mask", mask);
        }

        /// <inheritdoc />
        protected override void PerformLayoutBeforeChildren()
        {
            base.PerformLayoutBeforeChildren();

            ViewportWidgetsContainer.ArrangeWidgets(this);
        }

        /// <inheritdoc />
        public override bool HasLoadedAssets => base.HasLoadedAssets && _previewMaterial.IsLoaded && _previewMaterial.BaseMaterial.IsLoaded;

        /// <inheritdoc />
        public override void OnDestroy()
        {
            Material = null;
            FlaxEngine.Object.Destroy(ref _previewMaterial);

            base.OnDestroy();
        }
    }
}
