// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

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
        /// <summary>
        /// The preview that is during prefab instance spawning. Used to link some actors such as UIControl to preview scene and view.
        /// </summary>
        internal static PrefabPreview LoadingPreview;

        private Prefab _prefab;
        private Actor _instance;
        internal Control customControlLinked;

        /// <summary>
        /// Gets or sets the prefab asset to preview.
        /// </summary>
        public Prefab Prefab
        {
            get => _prefab;
            set
            {
                if (_prefab != value)
                {
                    if (_instance)
                    {
                        if (customControlLinked != null)
                        {
                            customControlLinked.Parent = null;
                            customControlLinked = null;
                        }
                        Task.RemoveCustomActor(_instance);
                        Object.Destroy(_instance);
                    }

                    _prefab = value;

                    if (_prefab)
                    {
                        _prefab.WaitForLoaded(); // TODO: use lazy prefab spawning to reduce stalls

                        var prevPreview = LoadingPreview;
                        LoadingPreview = this;

                        _instance = PrefabManager.SpawnPrefab(_prefab, null);

                        LoadingPreview = prevPreview;

                        if (_instance == null)
                        {
                            _prefab = null;
                            throw new FlaxException("Failed to spawn a prefab for the preview.");
                        }
                        Task.AddCustomActor(_instance);
                    }
                }
            }
        }

        /// <summary>
        /// Gets the instance of the prefab spawned for the preview.
        /// </summary>
        public Actor Instance
        {
            get => _instance;
            internal set => _instance = value;
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
        public override void OnDestroy()
        {
            // Cleanup
            Prefab = null;

            base.OnDestroy();
        }
    }
}
