// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;
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
        internal UIControl _uiControlLinked;

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
                        if (_uiControlLinked.Control?.Parent == this)
                            _uiControlLinked.Control.Parent = null;
                        _uiControlLinked = null;
                    }

                    // Remove for the preview
                    Task.RemoveCustomActor(_instance);
                }

                _instance = value;

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
            // Link UI canvases to the preview (eg. after canvas added to the prefab)
            LinkCanvas(_instance);

            // Link UI control to the preview
            if (_uiControlLinked == null &&
                _instance is UIControl uiControl &&
                uiControl.Control != null &&
                uiControl.Control.Parent == null)
            {
                uiControl.Control.Parent = this;
                _uiControlLinked = uiControl;
            }
        }

        private void LinkCanvas(Actor actor)
        {
            if (actor is UICanvas uiCanvas)
                uiCanvas.EditorOverride(Task, this);
            var children = actor.ChildrenCount;
            for (int i = 0; i < children; i++)
            {
                LinkCanvas(actor.GetChild(i));
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="PrefabPreview"/> class.
        /// </summary>
        /// <param name="useWidgets">if set to <c>true</c> use widgets.</param>
        public PrefabPreview(bool useWidgets)
        : base(useWidgets)
        {
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
            if (IsDisposing)
                return;
            Prefab = null;

            base.OnDestroy();
        }
    }
}
