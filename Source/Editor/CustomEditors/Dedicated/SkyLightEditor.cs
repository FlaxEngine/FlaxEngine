// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.CustomEditors.Dedicated
{
    /// <summary>
    /// Custom editor for <see cref="SkyLight"/>.
    /// </summary>
    /// <seealso cref="ActorEditor" />
    [CustomEditor(typeof(SkyLight)), DefaultEditor]
    public class SkyLightEditor : ActorEditor
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
                if (Values[i] is SkyLight skyLight)
                {
                    skyLight.Bake();
                    Editor.Instance.Scene.MarkSceneEdited(skyLight.Scene);
                }
            }
        }
    }
}
