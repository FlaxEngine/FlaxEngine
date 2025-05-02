// Copyright (c) Wojciech Figat. All rights reserved.

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
        private FlaxEngine.GUI.Button _bake;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            base.Initialize(layout);

            if (Values.HasDifferentTypes == false)
            {
                layout.Space(10);
                _bake = layout.Button("Bake").Button;
                _bake.Clicked += BakeButtonClicked;
            }
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            if (_bake != null)
            {
                _bake.Enabled = Values.Count != 0 && Values[0] is EnvironmentProbe probe && !probe.IsUsingCustomProbe;
            }
        }

        private void BakeButtonClicked()
        {
            for (int i = 0; i < Values.Count; i++)
            {
                if (Values[i] is EnvironmentProbe envProbe && !envProbe.IsUsingCustomProbe)
                {
                    envProbe.Bake();
                    Editor.Instance.Scene.MarkSceneEdited(envProbe.Scene);
                }
            }
        }
    }
}
