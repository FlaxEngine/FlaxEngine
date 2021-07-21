// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.ComponentModel;
using FlaxEditor.Content;
using FlaxEditor.Scripting;
using FlaxEditor.Surface;
using FlaxEditor.Viewport.Previews;
using FlaxEngine;

// ReSharper disable UnusedMember.Local
// ReSharper disable UnusedMember.Global
// ReSharper disable MemberCanBePrivate.Local

namespace FlaxEditor.Windows.Assets
{
    /// <summary>
    /// Material window allows to view and edit <see cref="Material"/> asset.
    /// </summary>
    /// <seealso cref="Material" />
    /// <seealso cref="MaterialSurface" />
    /// <seealso cref="MaterialPreview" />
    public sealed class MaterialWindow : VisjectSurfaceWindow<Material, MaterialSurface, MaterialPreview>
    {
        private readonly ScriptType[] _newParameterTypes =
        {
            new ScriptType(typeof(float)),
            new ScriptType(typeof(Texture)),
            new ScriptType(typeof(NormalMap)),
            new ScriptType(typeof(CubeTexture)),
            new ScriptType(typeof(GPUTexture)),
            new ScriptType(typeof(ChannelMask)),
            new ScriptType(typeof(bool)),
            new ScriptType(typeof(int)),
            new ScriptType(typeof(Vector2)),
            new ScriptType(typeof(Vector3)),
            new ScriptType(typeof(Vector4)),
            new ScriptType(typeof(Color)),
            new ScriptType(typeof(Quaternion)),
            new ScriptType(typeof(Transform)),
            new ScriptType(typeof(Matrix)),
        };

        /// <summary>
        /// The material properties proxy object.
        /// </summary>
        private sealed class PropertiesProxy
        {
            // General

            [EditorOrder(10), EditorDisplay("General"), Tooltip("Material domain type.")]
            public MaterialDomain Domain;

            [EditorOrder(20), EditorDisplay("General"), Tooltip("Defines how material inputs and properties are combined to result the final surface color.")]
            public MaterialShadingModel ShadingModel;

            [EditorOrder(30), EditorDisplay("General"), Tooltip("Determinates how materials' color should be blended with the background colors.")]
            public MaterialBlendMode BlendMode;

            // Rendering

            [EditorOrder(100), DefaultValue(CullMode.Normal), EditorDisplay("Rendering"), Tooltip("Defines the primitives culling mode used during geometry rendering.")]
            public CullMode CullMode;

            [EditorOrder(110), DefaultValue(false), EditorDisplay("Rendering"), Tooltip("If checked, geometry will be rendered in wireframe mode without solid triangles fill.")]
            public bool Wireframe;

            [EditorOrder(120), DefaultValue(true), EditorDisplay("Rendering"), Tooltip("Enables performing depth test during material rendering.")]
            public bool DepthTest;

            [EditorOrder(130), DefaultValue(true), EditorDisplay("Rendering"), Tooltip("Enable writing to the depth buffer during material rendering.")]
            public bool DepthWrite;

            // Transparency

            [EditorOrder(200), DefaultValue(true), EditorDisplay("Transparency"), Tooltip("Enables reflections when rendering material.")]
            public bool EnableReflections;

            [EditorOrder(210), DefaultValue(true), EditorDisplay("Transparency"), Tooltip("Enables fog effects when rendering material.")]
            public bool EnableFog;

            [EditorOrder(220), DefaultValue(true), EditorDisplay("Transparency"), Tooltip("Enables distortion effect when rendering.")]
            public bool EnableDistortion;

            [EditorOrder(225), DefaultValue(false), EditorDisplay("Transparency"), Tooltip("Enables refraction offset based on the difference between the per-pixel normal and the per-vertex normal. Useful for large water-like surfaces.")]
            public bool PixelNormalOffsetRefraction;

            [EditorOrder(230), DefaultValue(0.12f), EditorDisplay("Transparency"), Tooltip("Controls opacity values clipping point."), Limit(0.0f, 1.0f, 0.01f)]
            public float OpacityThreshold;

            // Tessellation

            [EditorOrder(300), DefaultValue(TessellationMethod.None), EditorDisplay("Tessellation"), Tooltip("Mesh tessellation method.")]
            public TessellationMethod TessellationMode;

            [EditorOrder(310), DefaultValue(15), EditorDisplay("Tessellation"), Tooltip("Maximum triangle tessellation factor."), Limit(1, 60, 0.01f)]
            public int MaxTessellationFactor;

            // Misc

            [EditorOrder(400), DefaultValue(false), EditorDisplay("Misc"), Tooltip("If checked, material input normal will be assumed as world-space rather than tangent-space.")]
            public bool InputWorldSpaceNormal;

            [EditorOrder(410), DefaultValue(false), EditorDisplay("Misc", "Dithered LOD Transition"), Tooltip("If checked, material uses dithered model LOD transition for smoother LODs switching.")]
            public bool DitheredLODTransition;

            [EditorOrder(420), DefaultValue(0.3f), EditorDisplay("Misc"), Tooltip("Controls mask values clipping point."), Limit(0.0f, 1.0f, 0.01f)]
            public float MaskThreshold;

            [EditorOrder(430), DefaultValue(MaterialDecalBlendingMode.Translucent), EditorDisplay("Misc"), Tooltip("The decal material blending mode.")]
            public MaterialDecalBlendingMode DecalBlendingMode;

            [EditorOrder(440), DefaultValue(MaterialPostFxLocation.AfterPostProcessingPass), EditorDisplay("Misc"), Tooltip("The post fx material rendering location.")]
            public MaterialPostFxLocation PostFxLocation;

            // Parameters

            [EditorOrder(1000), EditorDisplay("Parameters"), CustomEditor(typeof(ParametersEditor)), NoSerialize]
            // ReSharper disable once UnusedAutoPropertyAccessor.Local
            public MaterialWindow Window { get; set; }

            [HideInEditor, Serialize]
            // ReSharper disable once UnusedMember.Local
            public List<SurfaceParameter> Parameters
            {
                get => Window.Surface.Parameters;
                set => throw new Exception("No setter.");
            }

            /// <summary>
            /// Gathers parameters from the specified material.
            /// </summary>
            /// <param name="window">The window.</param>
            public void OnLoad(MaterialWindow window)
            {
                // Update cache
                var material = window.Asset;
                var info = material.Info;
                Wireframe = (info.FeaturesFlags & MaterialFeaturesFlags.Wireframe) != 0;
                CullMode = info.CullMode;
                DepthTest = (info.FeaturesFlags & MaterialFeaturesFlags.DisableDepthTest) == 0;
                DepthWrite = (info.FeaturesFlags & MaterialFeaturesFlags.DisableDepthWrite) == 0;
                EnableReflections = (info.FeaturesFlags & MaterialFeaturesFlags.DisableReflections) == 0;
                EnableFog = (info.FeaturesFlags & MaterialFeaturesFlags.DisableFog) == 0;
                EnableDistortion = (info.FeaturesFlags & MaterialFeaturesFlags.DisableDistortion) == 0;
                PixelNormalOffsetRefraction = (info.FeaturesFlags & MaterialFeaturesFlags.PixelNormalOffsetRefraction) != 0;
                InputWorldSpaceNormal = (info.FeaturesFlags & MaterialFeaturesFlags.InputWorldSpaceNormal) != 0;
                DitheredLODTransition = (info.FeaturesFlags & MaterialFeaturesFlags.DitheredLODTransition) != 0;
                OpacityThreshold = info.OpacityThreshold;
                TessellationMode = info.TessellationMode;
                MaxTessellationFactor = info.MaxTessellationFactor;
                MaskThreshold = info.MaskThreshold;
                DecalBlendingMode = info.DecalBlendingMode;
                PostFxLocation = info.PostFxLocation;
                BlendMode = info.BlendMode;
                ShadingModel = info.ShadingModel;
                Domain = info.Domain;

                // Link
                Window = window;
            }

            /// <summary>
            /// Saves the material properties to the material info structure.
            /// </summary>
            /// <param name="info">The material info.</param>
            public void OnSave(ref MaterialInfo info)
            {
                // Update flags
                info.CullMode = CullMode;
                if (Wireframe)
                    info.FeaturesFlags |= MaterialFeaturesFlags.Wireframe;
                if (!DepthTest)
                    info.FeaturesFlags |= MaterialFeaturesFlags.DisableDepthTest;
                if (!DepthWrite)
                    info.FeaturesFlags |= MaterialFeaturesFlags.DisableDepthWrite;
                if (!EnableReflections)
                    info.FeaturesFlags |= MaterialFeaturesFlags.DisableReflections;
                if (!EnableFog)
                    info.FeaturesFlags |= MaterialFeaturesFlags.DisableFog;
                if (!EnableDistortion)
                    info.FeaturesFlags |= MaterialFeaturesFlags.DisableDistortion;
                if (PixelNormalOffsetRefraction)
                    info.FeaturesFlags |= MaterialFeaturesFlags.PixelNormalOffsetRefraction;
                if (InputWorldSpaceNormal)
                    info.FeaturesFlags |= MaterialFeaturesFlags.InputWorldSpaceNormal;
                if (DitheredLODTransition)
                    info.FeaturesFlags |= MaterialFeaturesFlags.DitheredLODTransition;
                info.OpacityThreshold = OpacityThreshold;
                info.TessellationMode = TessellationMode;
                info.MaxTessellationFactor = MaxTessellationFactor;
                info.MaskThreshold = MaskThreshold;
                info.DecalBlendingMode = DecalBlendingMode;
                info.PostFxLocation = PostFxLocation;
                info.BlendMode = BlendMode;
                info.ShadingModel = ShadingModel;
                info.Domain = Domain;
            }

            /// <summary>
            /// Clears temporary data.
            /// </summary>
            public void OnClean()
            {
                // Unlink
                Window = null;
            }
        }

        private readonly PropertiesProxy _properties;

        /// <inheritdoc />
        public MaterialWindow(Editor editor, AssetItem item)
        : base(editor, item)
        {
            // Asset preview
            _preview = new MaterialPreview(true)
            {
                Parent = _split2.Panel1
            };

            // Asset properties proxy
            _properties = new PropertiesProxy();
            _propertiesEditor.Select(_properties);

            // Surface
            _surface = new MaterialSurface(this, Save, _undo)
            {
                Parent = _split1.Panel1,
                Enabled = false
            };

            // Toolstrip
            _toolstrip.AddSeparator();
            _toolstrip.AddButton(editor.Icons.Code64, ShowSourceCode).LinkTooltip("Show generated shader source code");
            _toolstrip.AddButton(editor.Icons.Docs64, () => Platform.OpenUrl(Utilities.Constants.DocsUrl + "manual/graphics/materials/index.html")).LinkTooltip("See documentation to learn more");
        }

        private void ShowSourceCode()
        {
            var source = Editor.GetShaderSourceCode(_asset);
            Utilities.Utils.ShowSourceCodeWindow(source, "Material Source", RootWindow.Window);
        }

        /// <summary>
        /// Gets material info from UI.
        /// </summary>
        /// <param name="info">Output info.</param>
        public void FillMaterialInfo(out MaterialInfo info)
        {
            info = MaterialInfo.CreateDefault();
            _properties.OnSave(ref info);
        }

        /// <summary>
        /// Gets or sets the main material node.
        /// </summary>
        public Surface.Archetypes.Material.SurfaceNodeMaterial MainNode
        {
            get
            {
                var mainNode = _surface.FindNode(1, 1) as Surface.Archetypes.Material.SurfaceNodeMaterial;
                if (mainNode == null)
                {
                    // Error
                    Editor.LogError("Failed to find main material node.");
                }
                return mainNode;
            }
        }

        /// <inheritdoc />
        public override IEnumerable<ScriptType> NewParameterTypes => _newParameterTypes;

        /// <inheritdoc />
        public override void SetParameter(int index, object value)
        {
            try
            {
                Asset.Parameters[index].Value = value;
            }
            catch
            {
                // Ignored
            }

            base.SetParameter(index, value);
        }

        /// <inheritdoc />
        protected override void OnPropertyEdited()
        {
            base.OnPropertyEdited();

            // Refresh main node
            var mainNode = MainNode;
            mainNode?.UpdateBoxes();
        }

        /// <inheritdoc />
        protected override void UnlinkItem()
        {
            _properties.OnClean();
            _preview.Material = null;

            base.UnlinkItem();
        }

        /// <inheritdoc />
        protected override void OnAssetLinked()
        {
            _preview.Material = _asset;

            base.OnAssetLinked();
        }

        /// <inheritdoc />
        public override string SurfaceName => "Material";

        /// <inheritdoc />
        public override byte[] SurfaceData
        {
            get => _asset.LoadSurface(true);
            set
            {
                // Create material info
                FillMaterialInfo(out var info);

                // Save data to the temporary material
                if (_asset.SaveSurface(value, info))
                {
                    // Error
                    _surface.MarkAsEdited();
                    Editor.LogError("Failed to save material surface data");
                }
                _asset.Reload();
            }
        }

        /// <inheritdoc />
        protected override bool LoadSurface()
        {
            // Init material properties and parameters proxy
            _properties.OnLoad(this);

            // Load surface graph
            if (_surface.Load())
            {
                // Error
                Editor.LogError("Failed to load material surface.");
                return true;
            }

            return false;
        }

        /// <inheritdoc />
        protected override bool SaveSurface()
        {
            _surface.Save();
            return false;
        }

        /// <inheritdoc />
        protected override bool CanEditSurfaceOnAssetLoadError => true;

        /// <inheritdoc />
        protected override bool SaveToOriginal()
        {
            // Copy shader cache from the temporary Particle Emitter (will skip compilation on Reload - faster)
            Guid dstId = _item.ID;
            Guid srcId = _asset.ID;
            Editor.Internal_CopyCache(ref dstId, ref srcId);

            return base.SaveToOriginal();
        }
    }
}
