// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.CustomEditors.Dedicated
{
    /// <summary>
    /// Custom editor for <see cref="EnvironmentProbe"/>.
    /// </summary>
    /// <seealso cref="ActorEditor" />
    [CustomEditor(typeof(EnvironmentProbe)), DefaultEditor]
    public class EnvironmentProbeEditor : ActorEditor
    {
        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            base.Initialize(layout);

            if (Values.HasDifferentTypes == false)
            {
                // Add 'Bake' button
                layout.Space(10);
                var button = layout.Button("Bake");
                button.Button.Clicked += BakeButtonClicked;
            }
        }

        private void BakeButtonClicked()
        {
            for (int i = 0; i < Values.Count; i++)
            {
                if (Values[i] is EnvironmentProbe envProbe)
                {
                    envProbe.Bake();
                    Editor.Instance.Scene.MarkSceneEdited(envProbe.Scene);
                }
            }
        }
    }
}
