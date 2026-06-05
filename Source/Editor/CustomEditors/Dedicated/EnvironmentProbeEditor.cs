// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Dedicated
{
    /// <summary>
    /// Custom editor for <see cref="EnvironmentProbe"/>.
    /// </summary>
    /// <seealso cref="ActorEditor" />
    [CustomEditor(typeof(EnvironmentProbe)), DefaultEditor]
    public class EnvironmentProbeEditor : ActorEditor
    {
        private Button _bake;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            base.Initialize(layout);

            if (Values.HasDifferentTypes == false)
            {
                var group = layout.Group("Probe");
                group.Panel.ItemsMargin = new Margin(Utilities.Constants.UIMargin * 2);
                _bake = group.Button("Bake").Button;
                _bake.Clicked += BakeButtonClicked;
                var view = group.Button("View", "Opens the probe texture viewer");
                view.Button.Clicked += OnViewButtonClicked;
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

        private void OnViewButtonClicked()
        {
            foreach (var value in Values)
            {
                if (value is EnvironmentProbe probe && probe.ProbeAsset)
                    Editor.Instance.ContentEditing.Open(probe.ProbeAsset);
            }
        }
    }
}
