// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;
using FlaxEngine.GUI;
using Object = FlaxEngine.Object;

namespace FlaxEditor.Viewport.Previews
{
    /// <summary>
    /// Prefab asset preview editor viewport.
    /// </summary>
    /// <seealso cref="AssetPreview" />
    public class PrefabPreview : AssetPreview
    {
        private Prefab _prefab;
        private Actor _instance;
        private UIControl _uiControlLinked;
        internal bool _hasUILinked;
        internal ContainerControl _uiParentLink;

        /// <summary>
        /// Gets or sets the prefab asset to preview.
        /// </summary>
        public Prefab Prefab
        {
            get => _prefab;
            set
            {
                if (_prefab == value)
                    return;

                // Unset and cleanup spawned instance
                if (_instance)
                {
                    var instance = _instance;
                    Instance = null;
                    Object.Destroy(instance);
                }

                _prefab = value;

                if (_prefab)
                {
                    // Load prefab
                    _prefab.WaitForLoaded();

                    // Spawn prefab
                    var instance = PrefabManager.SpawnPrefab(_prefab, null);
                    if (instance == null)
                    {
                        _prefab = null;
                        throw new Exception("Failed to spawn a prefab for the preview.");
                    }

                    // Set instance
                    Instance = instance;
                }
            }
        }

        /// <summary>
        /// Gets the instance of the prefab spawned for the preview.
        /// </summary>
        public Actor Instance
        {
            get => _instance;
            internal set
            {
                if (_instance == value)
                    return;

                if (_instance)
                {
                    // Unlink UI control
                    if (_uiControlLinked)
                    {
                        if (_uiControlLinked.Control?.Parent == _uiParentLink)
                            _uiControlLinked.Control.Parent = null;
                        _uiControlLinked = null;
                    }
                    foreach (var child in _uiParentLink.Children.ToArray())
                    {
                        if (child is CanvasRootControl canvasRoot)
                        {
                            canvasRoot.Canvas.EditorOverride(null, null);
                        }
                    }

                    // Remove for the preview
                    Task.RemoveCustomActor(_instance);
                }

                _instance = value;
                _hasUILinked = false;

                if (_instance)
                {
                    // Add to the preview
                    Task.AddCustomActor(_instance);
                    UpdateLinkage();
                }
            }
        }

        private void UpdateLinkage()
        {
            // Clear flag
            _hasUILinked = false;

            // Link UI canvases to the preview (eg. after canvas added to the prefab)
            LinkCanvas(_instance);

            // Link UI control to the preview
            if (_uiControlLinked == null &&
                _instance is UIControl uiControl &&
                uiControl.Control != null &&
                uiControl.Control.Parent == null)
            {
                uiControl.Control.Parent = _uiParentLink;
                _uiControlLinked = uiControl;
                _hasUILinked = true;
            }
            else if (_uiControlLinked != null)
            {
                if (_uiControlLinked.Control != null && 
                    _uiControlLinked.Control.Parent == null)
                    _uiControlLinked.Control.Parent = _uiParentLink;
                _hasUILinked = true;
            }
        }

        private void LinkCanvas(Actor actor)
        {
            if (actor is UICanvas uiCanvas)
            {
                uiCanvas.EditorOverride(Task, _uiParentLink);
                if (uiCanvas.GUI.Parent == _uiParentLink)
                    _hasUILinked = true;
            }

            var children = actor.ChildrenCount;
            for (int i = 0; i < children; i++)
                LinkCanvas(actor.GetChild(i));
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="PrefabPreview"/> class.
        /// </summary>
        /// <param name="useWidgets">if set to <c>true</c> use widgets.</param>
        public PrefabPreview(bool useWidgets)
        : base(useWidgets)
        {
            // Link to itself by default
            _uiParentLink = this;
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            base.Update(deltaTime);

            if (_instance != null)
            {
                UpdateLinkage();
            }
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            Prefab = null;

            base.OnDestroy();
        }
    }
}
