// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Surface;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEditor.Viewport.Widgets;
using FlaxEditor.GUI.ContextMenu;
using Object = FlaxEngine.Object;
using FlaxEditor.GUI;
using FlaxEditor.Scripting;

namespace FlaxEditor.Viewport.Previews
{
    /// <summary>
    /// Material or Material Instance asset preview editor viewport.
    /// </summary>
    /// <seealso cref="AssetPreview" />
    public class MaterialPreview : AssetPreview, IVisjectSurfaceOwner
    {
        private static readonly string[] Models =
        {
            "Sphere",
            "Cube",
            "Plane",
            "Cylinder",
            "Cone"
        };

        private static readonly Transform[] Transforms =
        {
            new Transform(Vector3.Zero, Quaternion.RotationY(Mathf.Pi), new Vector3(0.45f)),
            new Transform(Vector3.Zero, Quaternion.RotationY(Mathf.Pi), new Vector3(0.45f)),
            new Transform(Vector3.Zero, Quaternion.Identity, new Vector3(0.45f)),
            new Transform(Vector3.Zero, Quaternion.RotationY(Mathf.Pi), new Vector3(0.45f)),
            new Transform(Vector3.Zero, Quaternion.RotationY(Mathf.Pi), new Vector3(0.45f)),
        };

        private StaticModel _previewModel;
        private Decal _decal;
        private Terrain _terrain;
        private Spline _spline;
        private SplineModel _splineModel;
        private ParticleEffect _particleEffect;
        private MaterialBase _particleEffectMaterial;
        private ParticleEmitter _particleEffectEmitter;
        private ParticleSystem _particleEffectSystem;
        private ParticleEmitterSurface _particleEffectSurface;
        private MaterialBase _material;
        private int _selectedModelIndex;
        private Image _guiMaterialControl;
        private readonly MaterialBase[] _postFxMaterialsCache = new MaterialBase[1];
        private ContextMenu _modelWidgetButtonMenu;
        private AssetPicker _customModelPicker;
        private Model _customModel;

        /// <summary>
        /// Gets or sets the material asset to preview. It can be <see cref="FlaxEngine.Material"/> or <see cref="FlaxEngine.MaterialInstance"/>.
        /// </summary>
        public MaterialBase Material
        {
            get => _material;
            set
            {
                if (_material != value)
                {
                    _material = value;
                    UpdateMaterial();
                }
            }
        }

        /// <summary>
        /// Gets or sets the selected preview model index.
        /// </summary>
        public int SelectedModelIndex
        {
            get => _selectedModelIndex;
            set
            {
                if (value == -1) // Using Custom Model
                    return;
                if (value < 0 || value > Models.Length)
                    throw new ArgumentOutOfRangeException();

                if (_customModelPicker != null)
                    _customModelPicker.Validator.SelectedAsset = null;
                _selectedModelIndex = value;
                _previewModel.Model = FlaxEngine.Content.LoadAsyncInternal<Model>("Editor/Primitives/" + Models[value]);
                _previewModel.Transform = Transforms[value];
            }
        }

        // Used to automatically update which entry is checked.
        // TODO: Maybe a better system with predicate bool checks could be used?
        private void ResetModelContextMenu()
        {
            _modelWidgetButtonMenu.ItemsContainer.DisposeChildren();

            // Fill out all models
            for (int i = 0; i < Models.Length; i++)
            {
                var index = i;
                var button = _modelWidgetButtonMenu.AddButton(Models[index]);
                button.ButtonClicked += _ => SelectedModelIndex = index;
                button.Checked = SelectedModelIndex == index && _customModel == null;
                button.Tag = index;
            }

            _modelWidgetButtonMenu.AddSeparator();
            _customModelPicker = new AssetPicker(new ScriptType(typeof(Model)), Float2.Zero);

            // Label button
            var customModelPickerLabel = _modelWidgetButtonMenu.AddButton("Custom Model:");
            customModelPickerLabel.CloseMenuOnClick = false;
            customModelPickerLabel.Checked = _customModel != null;

            // Container button
            var customModelPickerButton = _modelWidgetButtonMenu.AddButton("");
            customModelPickerButton.Height = _customModelPicker.Height + 4;
            customModelPickerButton.CloseMenuOnClick = false;
            _customModelPicker.Parent = customModelPickerButton;
            _customModelPicker.Validator.SelectedAsset = _customModel;
            _customModelPicker.SelectedItemChanged += () =>
            {
                _customModel = _customModelPicker.Validator.SelectedAsset as Model;
                if (_customModelPicker.Validator.SelectedAsset == null)
                {
                    SelectedModelIndex = 0;
                    ResetModelContextMenu();
                    return;
                }

                _previewModel.Model = _customModel;
                _previewModel.Transform = Transforms[0];
                SelectedModelIndex = -1;
                ResetModelContextMenu();
            };
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="MaterialPreview"/> class.
        /// </summary>
        /// <param name="useWidgets">if set to <c>true</c> use widgets.</param>
        public MaterialPreview(bool useWidgets)
        : base(useWidgets)
        {
            // Setup preview scene
            _previewModel = new StaticModel();
            SelectedModelIndex = 0;

            // Link actors for rendering
            Task.AddCustomActor(_previewModel);

            // Create context menu for primitive switching
            if (useWidgets)
            {
                // Model mode widget
                var modelMode = new ViewportWidgetsContainer(ViewportWidgetLocation.UpperRight);
                _modelWidgetButtonMenu = new ContextMenu();
                _modelWidgetButtonMenu.VisibleChanged += control =>
                {
                    if (!control.Visible)
                        return;
                    ResetModelContextMenu();
                };
                new ViewportWidgetButton("Model", SpriteHandle.Invalid, _modelWidgetButtonMenu)
                {
                    TooltipText = "Change material model",
                    Parent = modelMode,
                };
                modelMode.Parent = this;
            }
        }

        /// <inheritdoc />
        public override bool HasLoadedAssets
        {
            get
            {
                if (!base.HasLoadedAssets)
                    return false;
                UpdateMaterial();
                return true;
            }
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            base.Update(deltaTime);

            UpdateMaterial();
        }

        private void UpdateMaterial()
        {
            // If material is a surface link it to the preview model.
            // Otherwise use postFx volume to render custom postFx material.
            MaterialBase surfaceMaterial = null;
            MaterialBase postFxMaterial = null;
            MaterialBase decalMaterial = null;
            MaterialBase guiMaterial = null;
            MaterialBase terrainMaterial = null;
            MaterialBase particleMaterial = null;
            MaterialBase deformableMaterial = null;
            bool usePreviewActor = true;
            if (_material != null)
            {
                if (_material is MaterialInstance materialInstance && materialInstance.BaseMaterial == null)
                {
                    // Material instance without a base material should not be used
                }
                else
                {
                    switch (_material.Info.Domain)
                    {
                    case MaterialDomain.Surface:
                        surfaceMaterial = _material;
                        break;
                    case MaterialDomain.PostProcess:
                        postFxMaterial = _material;
                        break;
                    case MaterialDomain.Decal:
                        decalMaterial = _material;
                        break;
                    case MaterialDomain.GUI:
                        usePreviewActor = false;
                        guiMaterial = _material;
                        break;
                    case MaterialDomain.Terrain:
                        usePreviewActor = false;
                        terrainMaterial = _material;
                        break;
                    case MaterialDomain.Particle:
                        usePreviewActor = false;
                        particleMaterial = _material;
                        break;
                    case MaterialDomain.Deformable:
                        usePreviewActor = false;
                        deformableMaterial = _material;
                        break;
                    case MaterialDomain.VolumeParticle:
                        usePreviewActor = false;
                        break;
                    default: throw new ArgumentOutOfRangeException();
                    }
                }
            }

            // Surface
            if (_previewModel.Model != null)
            {
                if (_previewModel.Model.WaitForLoaded())
                    throw new Exception("Preview model asset failed to load.");
                _previewModel.SetMaterial(0, surfaceMaterial);
            }
            _previewModel.IsActive = usePreviewActor;

            // PostFx
            _postFxMaterialsCache[0] = postFxMaterial;
            PostFxVolume.PostFxMaterials = new PostFxMaterialsSettings
            {
                Materials = _postFxMaterialsCache,
            };

            // Decal
            if (decalMaterial && _decal == null)
            {
                _decal = new Decal
                {
                    Size = new Vector3(100.0f),
                    LocalOrientation = Quaternion.RotationZ(Mathf.PiOverTwo)
                };
                Task.AddCustomActor(_decal);
            }
            if (_decal)
            {
                _decal.Material = decalMaterial;
            }

            // GUI
            if (guiMaterial && _guiMaterialControl == null)
            {
                _guiMaterialControl = new Image
                {
                    Offsets = Margin.Zero,
                    AnchorPreset = AnchorPresets.StretchAll,
                    KeepAspectRatio = false,
                    Brush = new MaterialBrush(),
                    Parent = this,
                    IndexInParent = 0,
                };
            }
            if (_guiMaterialControl != null)
            {
                ((MaterialBrush)_guiMaterialControl.Brush).Material = guiMaterial;
                _guiMaterialControl.Enabled = _guiMaterialControl.Visible = guiMaterial != null;
            }

            // Terrain
            if (terrainMaterial && _terrain == null)
            {
                _terrain = new Terrain();
                _terrain.Setup(1, 63);
                var chunkSize = _terrain.ChunkSize;
                var heightMapSize = chunkSize * Terrain.PatchEdgeChunksCount + 1;
                var heightMapLength = heightMapSize * heightMapSize;
                var heightmap = new float[heightMapLength];
                var patchCoord = new Int2(0, 0);
                _terrain.AddPatch(ref patchCoord);
                _terrain.SetupPatchHeightMap(ref patchCoord, heightmap, null, true);
                _terrain.LocalPosition = new Vector3(-1000, 0, -1000);
                Task.AddCustomActor(_terrain);
            }
            if (_terrain != null)
            {
                _terrain.IsActive = terrainMaterial != null;
                _terrain.Material = terrainMaterial;
            }

            // Particle
            if (particleMaterial && _particleEffect == null)
            {
                _particleEffect = new ParticleEffect
                {
                    IsLooping = true,
                    UseTimeScale = false
                };
                Task.AddCustomActor(_particleEffect);
            }
            if (_particleEffect != null)
            {
                _particleEffect.IsActive = particleMaterial != null;
                if (particleMaterial)
                    _particleEffect.UpdateSimulation();
                if (_particleEffectMaterial != particleMaterial && particleMaterial)
                {
                    _particleEffectMaterial = particleMaterial;
                    if (!_particleEffectEmitter)
                    {
                        var srcAsset = FlaxEngine.Content.LoadInternal<ParticleEmitter>("Editor/Particles/Particle Material Preview");
                        Editor.Instance.ContentEditing.FastTempAssetClone(srcAsset.Path, out var clonedPath);
                        _particleEffectEmitter = FlaxEngine.Content.Load<ParticleEmitter>(clonedPath);
                    }
                    if (_particleEffectSurface == null)
                        _particleEffectSurface = new ParticleEmitterSurface(this, null, null);
                    if (_particleEffectEmitter)
                    {
                        if (!_particleEffectSurface.Load())
                        {
                            var spriteModuleNode = _particleEffectSurface.FindNode(15, 400);
                            spriteModuleNode.Values[2] = particleMaterial.ID;
                            _particleEffectSurface.Save();
                        }
                    }
                }
            }

            // Deformable
            if (deformableMaterial && _spline == null)
            {
                _spline = new Spline();
                _spline.AddSplineLocalPoint(new Vector3(0, 0, -50.0f), false);
                _spline.AddSplineLocalPoint(new Vector3(0, 0, 50.0f));
                _splineModel = new SplineModel
                {
                    Scale = new Vector3(0.45f),
                    Parent = _spline,
                };
                Task.AddCustomActor(_spline);
            }
            if (_splineModel != null)
            {
                _splineModel.IsActive = deformableMaterial != null;
                _splineModel.Model = _previewModel.Model;
                _splineModel.SetMaterial(0, deformableMaterial);
            }
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _material = null;

            if (_guiMaterialControl != null)
            {
                _guiMaterialControl.Dispose();
                _guiMaterialControl = null;
            }

            Object.Destroy(ref _previewModel);
            Object.Destroy(ref _decal);
            Object.Destroy(ref _terrain);
            Object.Destroy(ref _spline);
            Object.Destroy(ref _splineModel);
            Object.Destroy(ref _particleEffect);
            Object.Destroy(ref _particleEffectEmitter);
            Object.Destroy(ref _particleEffectSystem);
            _particleEffectMaterial = null;
            _particleEffectSurface = null;

            base.OnDestroy();
        }

        /// <inheritdoc />
        public Asset SurfaceAsset => null;

        /// <inheritdoc />
        string ISurfaceContext.SurfaceName => string.Empty;

        /// <inheritdoc />
        byte[] ISurfaceContext.SurfaceData
        {
            get => _particleEffectEmitter.LoadSurface(false);
            set
            {
                _particleEffectEmitter.SaveSurface(value);
                _particleEffectEmitter.Reload();

                if (!_particleEffectSystem)
                {
                    _particleEffectSystem = FlaxEngine.Content.CreateVirtualAsset<ParticleSystem>();
                    _particleEffectSystem.Init(_particleEffectEmitter, 5.0f);
                    _particleEffect.ParticleSystem = _particleEffectSystem;
                }
            }
        }

        /// <inheritdoc />
        public VisjectSurfaceContext ParentContext => null;

        /// <inheritdoc />
        void ISurfaceContext.OnContextCreated(VisjectSurfaceContext context)
        {
        }

        /// <inheritdoc />
        public Undo Undo => null;

        /// <inheritdoc />
        void IVisjectSurfaceOwner.OnSurfaceEditedChanged()
        {
        }

        /// <inheritdoc />
        void IVisjectSurfaceOwner.OnSurfaceGraphEdited()
        {
        }

        /// <inheritdoc />
        void IVisjectSurfaceOwner.OnSurfaceClose()
        {
        }
    }
}
