// Copyright (c) Wojciech Figat. All rights reserved.

using System.IO;
using FlaxEditor.GUI.Input;
using FlaxEditor.GUI.Timeline.Undo;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI.Timeline.Tracks
{
    /// <summary>
    /// The timeline track that represents a folder used to group and organize tracks.
    /// </summary>
    /// <seealso cref="FlaxEditor.GUI.Timeline.Track" />
    public class FolderTrack : ConductorTrack
    {
        /// <summary>
        /// Gets the archetype.
        /// </summary>
        /// <returns>The archetype.</returns>
        public static TrackArchetype GetArchetype()
        {
            return new TrackArchetype
            {
                TypeId = 1,
                Name = "Folder",
                Icon = Editor.Instance.Icons.Folder32,
                Create = options => new FolderTrack(ref options),
                Load = LoadTrack,
                Save = SaveTrack,
            };
        }

        private static void LoadTrack(int version, Track track, BinaryReader stream)
        {
            var e = (FolderTrack)track;
            switch (version)
            {
            case 1:
                e.Color = new Color(stream.ReadByte(), stream.ReadByte(), stream.ReadByte(), stream.ReadByte());
                break;
            }
            e.IconColor = e.Color;
        }

        private static void SaveTrack(Track track, BinaryWriter stream)
        {
            var e = (FolderTrack)track;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="FolderTrack"/> class.
        /// </summary>
        /// <param name="options">The options.</param>
        public FolderTrack(ref TrackCreateOptions options)
        : base(ref options)
        {
            const float buttonSize = 14;
            var colorPickerButton = new Image
            {
                TooltipText = "Change folder color",
                AutoFocus = true,
                AnchorPreset = AnchorPresets.MiddleRight,
                IsScrollable = false,
                Color = Style.Current.ForegroundGrey,
                Margin = new Margin(1),
                Brush = new SpriteBrush(Style.Current.Settings),
                Offsets = new Margin(-buttonSize - 2 + _muteCheckbox.Offsets.Left, buttonSize, buttonSize * -0.5f, buttonSize),
                Parent = this,
            };
            colorPickerButton.Clicked += OnColorPickerButtonClicked;
        }

        private void OnColorPickerButtonClicked(Image image, MouseButton button)
        {
            if (button == MouseButton.Left)
            {
                ColorValueBox.ShowPickColorDialog?.Invoke(this, Color, OnColorChanged, null, false);
            }
        }

        private void OnColorChanged(Color color, bool sliding)
        {
            using (new TrackUndoBlock(this))
                Color = IconColor = color;
            Timeline.MarkAsEdited();
        }

        /// <inheritdoc />
        protected override bool CanAddChildTrack(Track track)
        {
            return true;
        }
    }
}
