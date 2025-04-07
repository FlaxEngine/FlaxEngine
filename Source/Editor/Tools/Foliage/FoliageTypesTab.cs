// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.ComponentModel;
using System.Linq;
using FlaxEditor.Content;
using FlaxEditor.CustomEditors;
using FlaxEditor.CustomEditors.Editors;
using FlaxEditor.GUI;
using FlaxEditor.GUI.Tabs;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Tools.Foliage
{
    /// <summary>
    /// Foliage types editor tab. Allows to add, remove or modify foliage instance types defined for the current foliage object.
    /// </summary>
    /// <seealso cref="GUI.Tabs.Tab" />
    public class FoliageTypesTab : Tab
    {
        /// <summary>
        /// The object for foliage type settings adjusting via Custom Editor.
        /// </summary>
        [CustomEditor(typeof(ProxyObjectEditor))]
        private sealed class ProxyObject
        {
            /// <summary>
            /// The tab.
            /// </summary>
            [HideInEditor]
            public readonly FoliageTypesTab Tab;

            /// <summary>
            /// The foliage actor.
            /// </summary>
            [HideInEditor]
            public FlaxEngine.Foliage Foliage;

            /// <summary>
            /// The selected foliage instance type index.
            /// </summary>
            [HideInEditor]
            public int SelectedFoliageTypeIndex;

            /// <summary>
            /// Initializes a new instance of the <see cref="ProxyObject"/> class.
            /// </summary>
            /// <param name="tab">The tab.</param>
            public ProxyObject(FoliageTypesTab tab)
            {
                Tab = tab;
                SelectedFoliageTypeIndex = -1;
            }

            private FoliageType _type;

            public void SyncOptions()
            {
                _type = Foliage.GetFoliageType(SelectedFoliageTypeIndex);
            }

            //

            [EditorOrder(10), EditorDisplay("Model"), Tooltip("Model to draw by all the foliage instances of this type. It must be unique within the foliage actor and cannot be null.")]
            public Model Model
            {
                get => _type.Model;
                set
                {
                    if (_type.Model == value)
                        return;
                    if (Foliage.FoliageTypes.Any(x => x.Model == value))
                    {
                        MessageBox.Show("The given model is already used by the other foliage type.", "Invalid model", MessageBoxButtons.OK, MessageBoxIcon.Error);
                        return;
                    }
                    _type.Model = value;
                    Tab.UpdateFoliageTypesList();

                    // TODO: support undo for editing foliage type properties
                }
            }

            [EditorOrder(20), EditorDisplay("Model"), Collection(CanResize = true), Tooltip("Model materials override collection. Can be used to change a specific material of the mesh to the custom one without editing the asset.")]
            public MaterialBase[] Materials
            {
                get
                {
                    if (Model.WaitForLoaded())
                        throw new Exception("Failed to load foliage model.");
                    return _type.Materials;
                }
                set
                {
                    if (Model.WaitForLoaded())
                        throw new Exception("Failed to load foliage model.");
                    _type.Materials = value;
                }
            }

            //

            [EditorOrder(100), EditorDisplay("Instance Options"), Limit(0.0f), Tooltip("The per-instance cull distance.")]
            public float CullDistance
            {
                get => _type.CullDistance;
                set
                {
                    if (Mathf.NearEqual(_type.CullDistance, value))
                        return;
                    _type.CullDistance = value;
                    Foliage.UpdateCullDistance();
                }
            }

            [EditorOrder(110), EditorDisplay("Instance Options"), Limit(0.0f), Tooltip("The per-instance cull distance randomization range (randomized per instance and added to master CullDistance value).")]
            public float CullDistanceRandomRange
            {
                get => _type.CullDistanceRandomRange;
                set
                {
                    if (Mathf.NearEqual(_type.CullDistanceRandomRange, value))
                        return;
                    _type.CullDistanceRandomRange = value;
                    Foliage.UpdateCullDistance();
                }
            }

            [EditorOrder(120), DefaultValue(DrawPass.Default), EditorDisplay("Instance Options"), Tooltip("The draw passes to use for rendering this foliage type.")]
            public DrawPass DrawModes
            {
                get => _type.DrawModes;
                set => _type.DrawModes = value;
            }

            [EditorOrder(130), DefaultValue(ShadowsCastingMode.All), EditorDisplay("Instance Options"), Tooltip("The shadows casting mode.")]
            public ShadowsCastingMode ShadowsMode
            {
                get => _type.ShadowsMode;
                set => _type.ShadowsMode = value;
            }

            [EditorOrder(140), EditorDisplay("Instance Options"), Tooltip("Determines whenever this meshes can receive decals.")]
            public bool ReceiveDecals
            {
                get => _type.ReceiveDecals;
                set => _type.ReceiveDecals = value;
            }

            [EditorOrder(145), EditorDisplay("Instance Options"), Limit(0.0f), Tooltip("The scale in lightmap (for instances of this foliage type). Can be used to adjust static lighting quality for the foliage instances.")]
            public float ScaleInLightmap
            {
                get => _type.ScaleInLightmap;
                set => _type.ScaleInLightmap = value;
            }

            [EditorOrder(150), EditorDisplay("Instance Options"), Tooltip("Flag used to determinate whenever use global foliage density scaling for instances of this foliage type.")]
            public bool UseDensityScaling
            {
                get => _type.UseDensityScaling;
                set
                {
                    if (_type.UseDensityScaling == value)
                        return;
                    _type.UseDensityScaling = value;
                    Foliage.RebuildClusters();
                }
            }

            [EditorOrder(160), VisibleIf("UseDensityScaling"), Limit(0, 10, 0.001f), EditorDisplay("Instance Options"), Tooltip("The density scaling scale applied to the global scale for the foliage instances of this type. Can be used to boost or reduce density scaling effect on this foliage type.")]
            public float DensityScalingScale
            {
                get => _type.DensityScalingScale;
                set
                {
                    if (Mathf.NearEqual(_type.DensityScalingScale, value))
                        return;
                    _type.DensityScalingScale = value;
                    Foliage.RebuildClusters();
                }
            }

            //

            [EditorOrder(200), EditorDisplay("Painting"), Limit(0.0f), Tooltip("The foliage instances density defined in instances count per 1000x1000 units area.")]
            public float Density
            {
                get => _type.PaintDensity;
                set => _type.PaintDensity = value;
            }

            [EditorOrder(210), EditorDisplay("Painting"), Limit(0.0f), Tooltip("The minimum radius between foliage instances.")]
            public float Radius
            {
                get => _type.PaintRadius;
                set => _type.PaintRadius = value;
            }

            [EditorOrder(215), EditorDisplay("Painting"), Limit(0.0f, 360.0f), Tooltip("The minimum and maximum ground slope angle to paint foliage on it (in degrees).")]
            public Float2 PaintGroundSlopeAngleRange
            {
                get => new Float2(_type.PaintGroundSlopeAngleMin, _type.PaintGroundSlopeAngleMax);
                set
                {
                    _type.PaintGroundSlopeAngleMin = value.X;
                    _type.PaintGroundSlopeAngleMax = value.Y;
                }
            }

            [EditorOrder(220), EditorDisplay("Painting"), Tooltip("The scaling mode.")]
            public FoliageScalingModes Scaling
            {
                get => _type.PaintScaling;
                set => _type.PaintScaling = value;
            }

            [EditorOrder(230), EditorDisplay("Painting"), Limit(0.0f), CustomEditor(typeof(ActorTransformEditor.ScaleEditor)), Tooltip("The scale minimum values per axis.")]
            public Float3 ScaleMin
            {
                get => _type.PaintScaleMin;
                set => _type.PaintScaleMin = value;
            }

            [EditorOrder(240), EditorDisplay("Painting"), Limit(0.0f), CustomEditor(typeof(ActorTransformEditor.ScaleEditor)), Tooltip("The scale maximum values per axis.")]
            public Float3 ScaleMax
            {
                get => _type.PaintScaleMax;
                set => _type.PaintScaleMax = value;
            }

            //

            [EditorOrder(300), EditorDisplay("Placement", "Offset Y"), Tooltip("The per-instance random offset range on axis Y (min-max).")]
            public Float2 OffsetY
            {
                get => _type.PlacementOffsetY;
                set => _type.PlacementOffsetY = value;
            }

            [EditorOrder(310), EditorDisplay("Placement"), Limit(0.0f), Tooltip("The random pitch angle range (uniform in both ways around normal vector).")]
            public float RandomPitchAngle
            {
                get => _type.PlacementRandomPitchAngle;
                set => _type.PlacementRandomPitchAngle = value;
            }

            [EditorOrder(320), EditorDisplay("Placement"), Limit(0.0f), Tooltip("The random roll angle range (uniform in both ways around normal vector).")]
            public float RandomRollAngle
            {
                get => _type.PlacementRandomRollAngle;
                set => _type.PlacementRandomRollAngle = value;
            }

            [EditorOrder(330), EditorDisplay("Placement", "Align To Normal"), Tooltip("If checked, instances will be aligned to normal of the placed surface.")]
            public bool AlignToNormal
            {
                get => _type.PlacementAlignToNormal;
                set => _type.PlacementAlignToNormal = value;
            }

            [EditorOrder(340), EditorDisplay("Placement"), Tooltip("If checked, instances will use randomized yaw when placed. Random yaw uses will rotation range over the Y axis.")]
            public bool RandomYaw
            {
                get => _type.PlacementRandomYaw;
                set => _type.PlacementRandomYaw = value;
            }
        }

        /// <summary>
        /// The custom editor for <see cref="ProxyObject"/>.
        /// </summary>
        /// <seealso cref="FlaxEditor.CustomEditors.Editors.GenericEditor" />
        private sealed class ProxyObjectEditor : GenericEditor
        {
            private Label _info;

            /// <inheritdoc />
            public override void Initialize(LayoutElementsContainer layout)
            {
                base.Initialize(layout);

                var space = layout.Space(22);
                var removeButton = new Button(2, 2.0f, 80.0f, 18.0f)
                {
                    Text = "Remove",
                    TooltipText = "Removes the selected foliage type and all foliage instances using this type",
                    Parent = space.Spacer
                };
                removeButton.Clicked += OnRemoveButtonClicked;
                _info = new Label(removeButton.Right + 6, 2, 200, 18.0f)
                {
                    HorizontalAlignment = TextAlignment.Near,
                    Parent = space.Spacer
                };
            }

            /// <inheritdoc />
            public override void Refresh()
            {
                // Sync selected foliage options once before update to prevent too many data copies when fetching data from UI properties accessors
                var proxyObject = (ProxyObject)Values[0];
                proxyObject.SyncOptions();

                _info.Text = string.Format("Instances: {0}, Total: {1}", proxyObject.Foliage.GetFoliageTypeInstancesCount(proxyObject.SelectedFoliageTypeIndex), proxyObject.Foliage.InstancesCount);

                base.Refresh();
            }

            private void OnRemoveButtonClicked()
            {
                var proxyObject = (ProxyObject)Values[0];
                proxyObject.Tab.RemoveFoliageType(proxyObject.SelectedFoliageTypeIndex);
            }
        }

        private readonly ProxyObject _proxy;
        private readonly VerticalPanel _items;
        private readonly Button _addFoliageTypeButton;
        private readonly CustomEditorPresenter _presenter;
        private int _foliageTypesCount;

        /// <summary>
        /// The parent foliage tab.
        /// </summary>
        public readonly FoliageTab Tab;

        /// <summary>
        /// Initializes a new instance of the <see cref="FoliageTypesTab"/> class.
        /// </summary>
        /// <param name="tab">The parent tab.</param>
        public FoliageTypesTab(FoliageTab tab)
        : base("Foliage Types")
        {
            Tab = tab;
            Tab.SelectedFoliageChanged += OnSelectedFoliageChanged;
            Tab.SelectedFoliageTypeIndexChanged += OnSelectedFoliageTypeIndexChanged;
            Tab.SelectedFoliageTypesChanged += UpdateFoliageTypesList;
            _proxy = new ProxyObject(this);

            // Main panel
            var splitPanel = new SplitPanel(Orientation.Vertical, ScrollBars.Vertical, ScrollBars.Vertical)
            {
                SplitterValue = 0.2f,
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                Parent = this
            };

            // Foliage types list
            _items = new VerticalPanel
            {
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                Offsets = new Margin(4, 4, 4, 0),
                Pivot = Float2.Zero,
                IsScrollable = true,
                Parent = splitPanel.Panel1
            };

            // Foliage add button
            _addFoliageTypeButton = new Button
            {
                Text = "Add Foliage Type",
                TooltipText = "Add new model to use it as a new foliage type for instancing and spawning in the level",
                Parent = splitPanel.Panel1
            };
            _addFoliageTypeButton.Clicked += OnAddFoliageTypeButtonClicked;

            // Options editor
            // TODO: use editor undo for changing foliage type options
            var editor = new CustomEditorPresenter(null, "No foliage type selected");
            editor.Panel.Parent = splitPanel.Panel2;
            editor.Modified += OnModified;
            _presenter = editor;
        }

        private void OnModified()
        {
            Editor.Instance.Scene.MarkSceneEdited(_proxy.Foliage != null ? _proxy.Foliage.Scene : null);
        }

        private void OnSelectedFoliageChanged()
        {
            _proxy.SelectedFoliageTypeIndex = -1;
            _proxy.Foliage = Tab.SelectedFoliage;

            _presenter.Deselect();

            UpdateFoliageTypesList();
        }

        private void OnSelectedFoliageTypeIndexChanged(int previousIndex, int currentIndex)
        {
            if (previousIndex != -1)
            {
                _items.Children[previousIndex].BackgroundColor = Color.Transparent;
            }

            _proxy.SelectedFoliageTypeIndex = currentIndex;

            if (currentIndex != -1)
            {
                _items.Children[currentIndex].BackgroundColor = Style.Current.BackgroundSelected;
                _proxy.SyncOptions();

                _presenter.Select(_proxy);
                _presenter.BuildLayoutOnUpdate();
            }
            else
            {
                _presenter.Deselect();
            }
            _presenter.Panel.Focus();
        }

        private void RemoveFoliageType(int index)
        {
            // Deselect if selected
            if (Tab.SelectedFoliageTypeIndex == index)
                Tab.SelectedFoliageTypeIndex = -1;

            var foliage = Tab.SelectedFoliage;
            var action = new Undo.EditFoliageAction(foliage);

            foliage.RemoveFoliageType(index);

            action.RecordEnd();
            Tab.Editor.Undo.AddAction(action);

            Tab.OnSelectedFoliageTypesChanged();
        }

        private void OnAddFoliageTypeButtonClicked()
        {
            // Show model picker
            AssetSearchPopup.Show(_addFoliageTypeButton, new Float2(_addFoliageTypeButton.Width * 0.5f, _addFoliageTypeButton.Height), IsItemValidForFoliageModel, OnItemSelectedForFoliageModel);
        }

        private void OnItemSelectedForFoliageModel(AssetItem item)
        {
            var foliage = Tab.SelectedFoliage;
            var model = FlaxEngine.Content.LoadAsync<Model>(item.ID);
            var action = new Undo.EditFoliageAction(foliage);

            foliage.AddFoliageType(model);
            Editor.Instance.Scene.MarkSceneEdited(foliage.Scene);

            action.RecordEnd();
            Tab.Editor.Undo.AddAction(action);

            Tab.OnSelectedFoliageTypesChanged();

            Tab.SelectedFoliageTypeIndex = foliage.FoliageTypesCount - 1;

            RootWindow.Focus();
            Focus();
        }

        private bool IsItemValidForFoliageModel(AssetItem item)
        {
            return item is BinaryAssetItem binaryItem && binaryItem.Type == typeof(Model);
        }

        private void UpdateFoliageTypesList()
        {
            var foliage = Tab.SelectedFoliage;

            // Cleanup previous items
            _items.DisposeChildren();

            // Add new ones
            float y = 0;
            if (foliage != null)
            {
                int typesCount = foliage.FoliageTypesCount;
                _foliageTypesCount = typesCount;
                for (int i = 0; i < typesCount; i++)
                {
                    var model = foliage.GetFoliageType(i).Model;
                    var asset = Tab.Editor.ContentDatabase.FindAsset(model.ID);
                    var itemView = new AssetSearchPopup.AssetItemView(asset)
                    {
                        TooltipText = asset.NamePath,
                        Tag = i,
                        Parent = _items,
                    };
                    itemView.Clicked += OnFoliageTypeListItemClicked;

                    y += itemView.Height + _items.Spacing;
                }
                y += _items.Margin.Height;
            }
            else
            {
                _foliageTypesCount = 0;
            }
            _items.Height = y;

            var selectedFoliageTypeIndex = Tab.SelectedFoliageTypeIndex;
            if (selectedFoliageTypeIndex != -1)
            {
                _items.Children[selectedFoliageTypeIndex].BackgroundColor = Style.Current.BackgroundSelected;
            }

            ArrangeAddFoliageButton();
        }

        private void OnFoliageTypeListItemClicked(ItemsListContextMenu.Item item)
        {
            Tab.SelectedFoliageTypeIndex = (int)item.Tag;
        }

        private void ArrangeAddFoliageButton()
        {
            _addFoliageTypeButton.Location = new Float2((_addFoliageTypeButton.Parent.Width - _addFoliageTypeButton.Width) * 0.5f, _items.Bottom + 4);
        }

        internal void CheckFoliageTypesCount()
        {
            var foliage = Tab.SelectedFoliage;
            var count = foliage ? foliage.FoliageTypesCount : 0;
            if (foliage != null && count != _foliageTypesCount)
            {
                if (Tab.SelectedFoliageTypeIndex >= count)
                    Tab.SelectedFoliageTypeIndex = -1;
                Tab.OnSelectedFoliageTypesChanged();
            }
        }

        /// <inheritdoc />
        protected override void PerformLayoutBeforeChildren()
        {
            base.PerformLayoutBeforeChildren();

            ArrangeAddFoliageButton();
        }
    }
}
