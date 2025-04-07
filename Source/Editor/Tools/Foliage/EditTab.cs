// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.CustomEditors;
using FlaxEditor.CustomEditors.Editors;
using FlaxEditor.GUI.Tabs;
using FlaxEditor.Tools.Foliage.Undo;
using FlaxEngine;
using FlaxEngine.GUI;

// ReSharper disable UnusedMember.Local

namespace FlaxEditor.Tools.Foliage
{
    /// <summary>
    /// Foliage instances editor tab. Allows to pick and edit a single foliage instance properties.
    /// </summary>
    /// <seealso cref="GUI.Tabs.Tab" />
    public class EditTab : Tab
    {
        /// <summary>
        /// The object for foliage settings adjusting via Custom Editor.
        /// </summary>
        [CustomEditor(typeof(ProxyObjectEditor))]
        private sealed class ProxyObject
        {
            /// <summary>
            /// The gizmo mode.
            /// </summary>
            [HideInEditor]
            public readonly EditFoliageGizmoMode Mode;

            /// <summary>
            /// The selected foliage actor.
            /// </summary>
            [HideInEditor]
            public FlaxEngine.Foliage Foliage;

            /// <summary>
            /// The selected foliage instance index.
            /// </summary>
            [HideInEditor]
            public int InstanceIndex;

            /// <summary>
            /// Initializes a new instance of the <see cref="ProxyObject"/> class.
            /// </summary>
            /// <param name="mode">The foliage editing gizmo mode.</param>
            public ProxyObject(EditFoliageGizmoMode mode)
            {
                Mode = mode;
                InstanceIndex = -1;
            }

            private FoliageInstance _instance;

            public void SyncOptions()
            {
                if (Foliage != null && InstanceIndex > -1 && InstanceIndex < Foliage.InstancesCount)
                {
                    _instance = Foliage.GetInstance(InstanceIndex);
                }
            }

            public void SetOptions()
            {
                if (Foliage != null && InstanceIndex > -1 && InstanceIndex < Foliage.InstancesCount)
                {
                    Foliage.SetInstanceTransform(InstanceIndex, ref _instance.Transform);
                    Foliage.RebuildClusters();
                }
            }

            //

            [EditorOrder(-10), EditorDisplay("Instance"), ReadOnly, Tooltip("The foliage instance zero-based index (read-only).")]
            public int Index
            {
                get => InstanceIndex;
                set => throw new Exception();
            }

            [EditorOrder(0), EditorDisplay("Instance"), ReadOnly, Tooltip("The foliage instance model (read-only).")]
            public Model Model
            {
                get => _instance.Type >= 0 && _instance.Type < Foliage.InstancesCount ? Foliage.GetFoliageType(_instance.Type).Model : null;
                set => throw new Exception();
            }

            [EditorOrder(10), EditorDisplay("Instance"), Tooltip("The local-space position of the mesh relative to the foliage actor.")]
            public Vector3 Position
            {
                get => _instance.Transform.Translation;
                set
                {
                    _instance.Transform.Translation = value;
                    SetOptions();
                }
            }

            [EditorOrder(20), EditorDisplay("Instance"), Tooltip("The local-space rotation of the mesh relative to the foliage actor.")]
            public Quaternion Rotation
            {
                get => _instance.Transform.Orientation;
                set
                {
                    _instance.Transform.Orientation = value;
                    SetOptions();
                }
            }

            [EditorOrder(30), EditorDisplay("Instance"), Tooltip("The local-space scale of the mesh relative to the foliage actor.")]
            public Float3 Scale
            {
                get => _instance.Transform.Scale;
                set
                {
                    _instance.Transform.Scale = value;
                    SetOptions();
                }
            }
        }

        /// <summary>
        /// The custom editor for <see cref="ProxyObject"/>.
        /// </summary>
        /// <seealso cref="FlaxEditor.CustomEditors.Editors.GenericEditor" />
        private sealed class ProxyObjectEditor : GenericEditor
        {
            /// <inheritdoc />
            public override void Initialize(LayoutElementsContainer layout)
            {
                base.Initialize(layout);

                var proxyObject = (ProxyObject)Values[0];
                if (proxyObject.InstanceIndex != -1)
                {
                    var area = layout.Space(32);
                    var button = new Button(4, 6, 100)
                    {
                        Text = "Delete",
                        TooltipText = "Removes the selected foliage instance",
                        Parent = area.ContainerControl
                    };
                    button.Clicked += OnDeleteButtonClicked;
                }
            }

            private void OnDeleteButtonClicked()
            {
                var proxyObject = (ProxyObject)Values[0];
                var mode = proxyObject.Mode;
                var foliage = mode.SelectedFoliage;
                if (!foliage)
                    throw new InvalidOperationException("No foliage selected.");
                var instanceIndex = mode.SelectedInstanceIndex;
                if (instanceIndex < 0 || instanceIndex >= foliage.InstancesCount)
                    throw new InvalidOperationException("No foliage instance selected.");

                // Delete instance and deselect it
                var action = new DeleteInstanceAction(foliage, instanceIndex);
                action.Do();
                Editor.Instance.Undo?.AddAction(new MultiUndoAction(action, new EditSelectedInstanceIndexAction(instanceIndex, -1)));
                mode.SelectedInstanceIndex = -1;
            }

            /// <inheritdoc />
            public override void Refresh()
            {
                // Sync selected foliage options once before update to prevent too many data copies when fetching data from UI properties accessors
                var proxyObject = (ProxyObject)Values[0];
                proxyObject.SyncOptions();

                base.Refresh();
            }
        }

        private readonly ProxyObject _proxy;
        private readonly CustomEditorPresenter _presenter;

        /// <summary>
        /// The parent foliage tab.
        /// </summary>
        public readonly FoliageTab Tab;

        /// <summary>
        /// The related gizmo mode.
        /// </summary>
        public readonly EditFoliageGizmoMode Mode;

        /// <summary>
        /// Initializes a new instance of the <see cref="EditTab"/> class.
        /// </summary>
        /// <param name="tab">The parent tab.</param>
        /// <param name="mode">The related gizmo mode.</param>
        public EditTab(FoliageTab tab, EditFoliageGizmoMode mode)
        : base("Edit")
        {
            Mode = mode;
            Tab = tab;
            Tab.SelectedFoliageChanged += OnSelectedFoliageChanged;
            mode.SelectedInstanceIndexChanged += OnSelectedInstanceIndexChanged;
            _proxy = new ProxyObject(mode);

            // Main panel
            var panel = new Panel(ScrollBars.Vertical)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                Parent = this
            };

            // Options editor
            var editor = new CustomEditorPresenter(tab.Editor.Undo, "No foliage instance selected");
            editor.Panel.Parent = panel;
            editor.Modified += OnModified;
            _presenter = editor;
        }

        private void OnSelectedInstanceIndexChanged()
        {
            _proxy.InstanceIndex = Mode.SelectedInstanceIndex;
            if (_proxy.InstanceIndex == -1)
            {
                _presenter.Deselect();
            }
            else
            {
                _presenter.Select(_proxy);
            }
            _presenter.Panel.Focus();
        }

        private void OnModified()
        {
            Editor.Instance.Scene.MarkSceneEdited(_proxy.Foliage?.Scene);
        }

        private void OnSelectedFoliageChanged()
        {
            Mode.SelectedInstanceIndex = -1;
            _proxy.Foliage = Tab.SelectedFoliage;
        }
    }
}
