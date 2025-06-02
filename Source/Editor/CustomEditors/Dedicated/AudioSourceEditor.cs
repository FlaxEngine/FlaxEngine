// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Dedicated
{
    /// <summary>
    /// Custom editor for <see cref="AudioSource"/>.
    /// </summary>
    [CustomEditor(typeof(AudioSource)), DefaultEditor]
    public class AudioSourceEditor : ActorEditor
    {
        private Label _infoLabel;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            base.Initialize(layout);

            // Show playback options during simulation
            if (Editor.IsPlayMode)
            {
                var playbackGroup = layout.Group("Playback");
                playbackGroup.Panel.Open();

                _infoLabel = playbackGroup.Label(string.Empty).Label;
                _infoLabel.AutoHeight = true;

                var grid = playbackGroup.UniformGrid();
                var gridControl = grid.CustomControl;
                gridControl.ClipChildren = false;
                gridControl.Height = Button.DefaultHeight;
                gridControl.SlotsHorizontally = 3;
                gridControl.SlotsVertically = 1;
                grid.Button("Play").Button.Clicked += () => Foreach(x => x.Play());
                grid.Button("Pause").Button.Clicked += () => Foreach(x => x.Pause());
                grid.Button("Stop").Button.Clicked += () => Foreach(x => x.Stop());
            }
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            if (_infoLabel != null)
            {
                var text = string.Empty;
                foreach (var value in Values)
                {
                    if (value is AudioSource audioSource && audioSource.Clip)
                        text += $"Time: {audioSource.Time:##0.0}s / {audioSource.Clip.Length:##0.0}s\n";
                }
                _infoLabel.Text = text;
            }
        }

        private void Foreach(Action<AudioSource> func)
        {
            foreach (var value in Values)
            {
                if (value is AudioSource audioSource)
                    func(audioSource);
            }
        }
    }
}
