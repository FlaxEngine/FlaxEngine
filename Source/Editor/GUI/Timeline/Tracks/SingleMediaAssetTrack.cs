// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.GUI.Timeline.Undo;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI.Timeline.Tracks
{
    /// <summary>
    /// The base class for timeline tracks that use single media with an asset reference.
    /// </summary>
    /// <typeparam name="TAsset">The type of the asset.</typeparam>
    /// <typeparam name="TMedia">The type of the media event.</typeparam>
    /// <seealso cref="FlaxEditor.GUI.Timeline.Track" />
    public abstract class SingleMediaAssetTrack<TAsset, TMedia> : SingleMediaTrack<TMedia>
    where TAsset : Asset
    where TMedia : Media, new()
    {
        /// <summary>
        /// The asset reference picker control.
        /// </summary>
        protected readonly AssetPicker _picker;

        /// <summary>
        /// The asset id.
        /// </summary>
        public Guid AssetID;

        /// <summary>
        /// Gets or sets the asset.
        /// </summary>
        public TAsset Asset
        {
            get => FlaxEngine.Content.LoadAsync<TAsset>(AssetID);
            set
            {
                if (AssetID == value?.ID)
                    return;
                AssetID = value?.ID ?? Guid.Empty;
                _picker.Validator.SelectedAsset = value;
                OnAssetChanged();
                Timeline?.MarkAsEdited();
            }
        }

        /// <inheritdoc />
        protected SingleMediaAssetTrack(ref TrackCreateOptions options)
        : base(ref options)
        {
            var width = 50.0f;
            var height = 36.0f;
            _picker = new AssetPicker(new ScriptType(typeof(TAsset)), Float2.Zero)
            {
                AnchorPreset = AnchorPresets.MiddleRight,
                Offsets = new Margin(-width - 2 + _muteCheckbox.Offsets.Left, width, height * -0.5f, height),
                Parent = this,
            };
            _picker.SelectedItemChanged += OnPickerSelectedItemChanged;
            Height = 4 + _picker.Height;
        }

        private void OnPickerSelectedItemChanged()
        {
            if (Asset == (TAsset)_picker.Validator.SelectedAsset)
                return;
            using (new TrackUndoBlock(this))
                Asset = (TAsset)_picker.Validator.SelectedAsset;
        }

        /// <summary>
        /// Called when selected asset gets changed.
        /// </summary>
        protected virtual void OnAssetChanged()
        {
        }
    }
}
