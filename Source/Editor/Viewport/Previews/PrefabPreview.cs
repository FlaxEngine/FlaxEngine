// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
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
        /// <summary>
        /// The currently spawned prefab instance owner. Used to link some actors such as UIControl to preview scene and view.
        /// </summary>
        internal static PrefabPreview LoadingPreview;

        /// <summary>
        /// The list of active prefab previews. Used to link some actors such as UIControl to preview scene and view.
        /// </summary>
        internal static List<PrefabPreview> ActivePreviews;

        private Prefab _prefab;
        private Actor _instance;
        internal UIControl customControlLinked;

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
                    LoadingPreview = this;
                    var instance = PrefabManager.SpawnPrefab(_prefab, null);
                    LoadingPreview = null;
                    if (instance == null)
                    {
                        _prefab = null;
                        ActivePreviews.Remove(this);
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
                    if (customControlLinked)
                    {
                        if (customControlLinked.Control?.Parent == this)
                            customControlLinked.Control.Parent = null;
                        customControlLinked = null;
                    }

                    // Remove for the preview
                    Task.RemoveCustomActor(_instance);
                }

                _instance = value;

                if (_instance)
                {
                    // Add to the preview
                    Task.AddCustomActor(_instance);

                    // Link UI canvases to the preview
                    LinkCanvas(_instance);
                }
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
            if (ActivePreviews == null)
                ActivePreviews = new List<PrefabPreview>();
            ActivePreviews.Add(this);
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            base.Update(deltaTime);

            if (_instance != null)
            {
                // Link UI canvases to the preview (eg. after canvas added to the prefab)
                LinkCanvas(_instance);
            }
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            ActivePreviews.Remove(this);
            Prefab = null;

            base.OnDestroy();
        }
    }
}
