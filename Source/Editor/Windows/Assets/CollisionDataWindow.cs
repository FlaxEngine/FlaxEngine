// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System.Xml;
using FlaxEditor.Content;
using FlaxEditor.Content.Create;
using FlaxEditor.CustomEditors;
using FlaxEditor.CustomEditors.Editors;
using FlaxEditor.CustomEditors.Elements;
using FlaxEditor.Viewport.Cameras;
using FlaxEditor.Viewport.Previews;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Windows.Assets
{
    /// <summary>
    /// Editor window to view/modify <see cref="CollisionData"/> asset.
    /// </summary>
    /// <seealso cref="CollisionData" />
    /// <seealso cref="FlaxEditor.Windows.Assets.AssetEditorWindow" />
    public sealed class CollisionDataWindow : AssetEditorWindowBase<CollisionData>
    {
        /// <summary>
        /// The asset properties proxy object.
        /// </summary>
        [CustomEditor(typeof(Editor))]
        private sealed class PropertiesProxy
        {
            private CollisionDataWindow Window;
            private CollisionData Asset;
            private bool _isCooking;

            [EditorOrder(0), EditorDisplay("General"), Tooltip("Type of the collision data to use")]
            public CollisionDataType Type;

            [EditorOrder(10), EditorDisplay("General"), Tooltip("Source model asset to use for collision data generation")]
            public Model Model;

            [EditorOrder(20), Limit(0, 5), EditorDisplay("General", "Model LOD Index"), Tooltip("Source model LOD index to use for collision data generation (will be clamped to the actual model LODs collection size)")]
            public int ModelLodIndex;

            [EditorOrder(100), EditorDisplay("Convex Mesh", "Convex Flags"), Tooltip("Convex mesh generation flags")]
            public ConvexMeshGenerationFlags ConvexFlags;

            [EditorOrder(110), Limit(8, 255), EditorDisplay("Convex Mesh", "Vertex Limit"), Tooltip("Convex mesh vertex count limit")]
            public int ConvexVertexLimit;

            public class Editor : GenericEditor
            {
                private ButtonElement _cookButton;

                /// <inheritdoc />
                public override void Initialize(LayoutElementsContainer layout)
                {
                    base.Initialize(layout);

                    layout.Space(10);
                    _cookButton = layout.Button("Cook");
                    _cookButton.Button.Clicked += OnCookButtonClicked;
                }

                /// <inheritdoc />
                public override void Refresh()
                {
                    if (_cookButton != null && Values.Count == 1)
                    {
                        var p = (PropertiesProxy)Values[0];
                        if (p._isCooking)
                        {
                            _cookButton.Button.Enabled = false;
                            _cookButton.Button.Text = "Cooking...";
                        }
                        else
                        {
                            _cookButton.Button.Enabled = p.Type != CollisionDataType.None && p.Model != null;
                            _cookButton.Button.Text = "Cook";
                        }
                    }

                    base.Refresh();
                }

                private void OnCookButtonClicked()
                {
                    ((PropertiesProxy)Values[0]).Cook();
                }
            }

            private class CookData : CreateFileEntry
            {
                private PropertiesProxy Proxy;
                private CollisionDataType Type;
                private Model Model;
                private int ModelLodIndex;
                private ConvexMeshGenerationFlags ConvexFlags;
                private int ConvexVertexLimit;

                public CookData(PropertiesProxy proxy, string resultUrl, CollisionDataType type, Model model, int modelLodIndex, ConvexMeshGenerationFlags convexFlags, int convexVertexLimit)
                : base("Collision Data", resultUrl)
                {
                    Proxy = proxy;
                    Type = type;
                    Model = model;
                    ModelLodIndex = modelLodIndex;
                    ConvexFlags = convexFlags;
                    ConvexVertexLimit = convexVertexLimit;
                }

                /// <inheritdoc />
                public override bool Create()
                {
                    bool failed = FlaxEditor.Editor.CookMeshCollision(ResultUrl, Type, Model, ModelLodIndex, ConvexFlags, ConvexVertexLimit);

                    Proxy._isCooking = false;
                    Proxy.Window.UpdateWiresModel();

                    return failed;
                }
            }

            public void Cook()
            {
                _isCooking = true;
                Window.Editor.ContentImporting.Create(new CookData(this, Asset.Path, Type, Model, ModelLodIndex, ConvexFlags, ConvexVertexLimit));
            }

            public void OnLoad(CollisionDataWindow window)
            {
                // Link
                Window = window;
                Asset = window.Asset;

                // Setup cooking parameters
                var options = Asset.Options;
                Type = options.Type;
                if (Type == CollisionDataType.None)
                    Type = CollisionDataType.ConvexMesh;
                Model = FlaxEngine.Content.LoadAsync<Model>(options.Model);
                ModelLodIndex = options.ModelLodIndex;
                ConvexFlags = options.ConvexFlags;
                ConvexVertexLimit = options.ConvexVertexLimit;
            }

            public void OnClean()
            {
                // Unlink
                Window = null;
                Asset = null;
                Model = null;
            }
        }

        private readonly SplitPanel _split;
        private readonly ModelPreview _preview;
        private readonly CustomEditorPresenter _propertiesPresenter;
        private readonly PropertiesProxy _properties;
        private Model _collisionWiresModel;
        private StaticModel _collisionWiresShowActor;
        private bool _updateWireMesh;

        /// <inheritdoc />
        public CollisionDataWindow(Editor editor, AssetItem item)
        : base(editor, item)
        {
            // Toolstrip
            _toolstrip.AddSeparator();
            _toolstrip.AddButton(editor.Icons.Docs32, () => Platform.OpenUrl(Utilities.Constants.DocsUrl + "manual/physics/colliders/collision-data.html")).LinkTooltip("See documentation to learn more");

            // Split Panel
            _split = new SplitPanel(Orientation.Horizontal, ScrollBars.None, ScrollBars.Vertical)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(0, 0, _toolstrip.Bottom, 0),
                SplitterValue = 0.7f,
                Parent = this
            };

            // Model preview
            _preview = new ModelPreview(true)
            {
                ViewportCamera = new FPSCamera(),
                Parent = _split.Panel1
            };

            // Asset properties
            _propertiesPresenter = new CustomEditorPresenter(null);
            _propertiesPresenter.Panel.Parent = _split.Panel2;
            _properties = new PropertiesProxy();
            _propertiesPresenter.Select(_properties);
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            // Sync helper actor size with actual preview model (preview scales model for better usage experience)
            if (_collisionWiresShowActor && _collisionWiresShowActor.IsActive)
            {
                _collisionWiresShowActor.Transform = _preview.PreviewActor.Transform;
            }

            base.Update(deltaTime);
        }

        /// <summary>
        /// Updates the collision data debug model.
        /// </summary>
        private void UpdateWiresModel()
        {
            // Don't update on a importer/worker thread
            if (Platform.CurrentThreadID != Globals.MainThreadID)
            {
                _updateWireMesh = true;
                return;
            }

            if (_collisionWiresModel == null)
            {
                _collisionWiresModel = FlaxEngine.Content.CreateVirtualAsset<Model>();
                _collisionWiresModel.SetupLODs(new[] { 1 });
            }
            Editor.Internal_GetCollisionWires(FlaxEngine.Object.GetUnmanagedPtr(Asset), out var triangles, out var indices);
            if (triangles != null && indices != null)
                _collisionWiresModel.LODs[0].Meshes[0].UpdateMesh(triangles, indices);
            else
                Editor.LogWarning("Failed to get collision wires for " + Asset);
            if (_collisionWiresShowActor == null)
            {
                _collisionWiresShowActor = new StaticModel();
                _preview.Task.AddCustomActor(_collisionWiresShowActor);
            }
            _collisionWiresShowActor.Model = _collisionWiresModel;
            _collisionWiresShowActor.SetMaterial(0, FlaxEngine.Content.LoadAsyncInternal<MaterialBase>(EditorAssets.WiresDebugMaterial));
            _preview.Model = FlaxEngine.Content.LoadAsync<Model>(_asset.Options.Model);
        }

        /// <inheritdoc />
        protected override void UnlinkItem()
        {
            _properties.OnClean();
            _preview.Model = null;

            base.UnlinkItem();
        }

        /// <inheritdoc />
        protected override void OnAssetLinked()
        {
            _preview.Model = null;

            base.OnAssetLinked();
        }

        /// <inheritdoc />
        protected override void OnAssetLoaded()
        {
            _properties.OnLoad(this);
            _propertiesPresenter.BuildLayout();
            ClearEditedFlag();
            UpdateWiresModel();

            base.OnAssetLoaded();
        }

        /// <inheritdoc />
        public override void OnItemReimported(ContentItem item)
        {
            // Refresh the properties (will get new data in OnAssetLoaded)
            _properties.OnClean();
            _propertiesPresenter.BuildLayout();
            ClearEditedFlag();

            base.OnItemReimported(item);
        }

        /// <inheritdoc />
        public override void OnUpdate()
        {
            if (_updateWireMesh)
            {
                _updateWireMesh = false;
                UpdateWiresModel();
            }

            base.OnUpdate();
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            base.OnDestroy();

            Object.Destroy(ref _collisionWiresShowActor);
            Object.Destroy(ref _collisionWiresModel);
        }

        /// <inheritdoc />
        public override bool UseLayoutData => true;

        /// <inheritdoc />
        public override void OnLayoutSerialize(XmlWriter writer)
        {
            writer.WriteAttributeString("Split", _split.SplitterValue.ToString());
        }

        /// <inheritdoc />
        public override void OnLayoutDeserialize(XmlElement node)
        {
            if (float.TryParse(node.GetAttribute("Split"), out float value1))
                _split.SplitterValue = value1;
        }

        /// <inheritdoc />
        public override void OnLayoutDeserialize()
        {
            _split.SplitterValue = 0.7f;
        }
    }
}
