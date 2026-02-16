// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.CustomEditors.Dedicated
{
    /// <summary>
    /// Custom editor for <see cref="NavMeshBoundsVolume"/>.
    /// </summary>
    /// <seealso cref="ActorEditor" />
    [CustomEditor(typeof(NavMeshBoundsVolume)), DefaultEditor]
    internal class NavMeshBoundsVolumeEditor : ActorEditor
    {
        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            base.Initialize(layout);

            if (Values.HasDifferentTypes == false)
            {
                var button = layout.Button("Build");
                button.Button.Clicked += OnBuildClicked;
            }
        }

        private void OnBuildClicked()
        {
            foreach (var value in Values)
            {
                if (value is NavMeshBoundsVolume volume)
                {
                    Navigation.BuildNavMesh(volume.Box, volume.Scene);
                    Editor.Instance.Scene.MarkSceneEdited(volume.Scene);
                }
            }
        }
    }
}
