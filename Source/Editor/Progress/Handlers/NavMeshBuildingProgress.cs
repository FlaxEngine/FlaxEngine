// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.Progress.Handlers
{
    /// <summary>
    /// Navigation mesh building progress reporting handler.
    /// </summary>
    /// <seealso cref="FlaxEditor.Progress.ProgressHandler" />
    public sealed class NavMeshBuildingProgress : ProgressHandler
    {
        private bool _isActive;

        internal bool DirtyScenesOnEnd;

        /// <summary>
        /// Initializes a new instance of the <see cref="NavMeshBuildingProgress"/> class.
        /// </summary>
        public NavMeshBuildingProgress()
        {
            FlaxEngine.Scripting.Update += OnUpdate;
        }

        private void OnUpdate()
        {
            bool isActive = Navigation.IsBuildingNavMesh;

            if (_isActive != isActive)
            {
                _isActive = isActive;

                if (isActive)
                {
                    OnStart();
                }
                else
                {
                    if (DirtyScenesOnEnd)
                    {
                        DirtyScenesOnEnd = false;
                        Editor.Instance.Scene.MarkAllScenesEdited();
                    }
                    OnEnd();
                }
            }

            if (isActive)
            {
                OnUpdate(Navigation.NavMeshBuildingProgress, "Building navmesh...");
            }
        }
    }
}
