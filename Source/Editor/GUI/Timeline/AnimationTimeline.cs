// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Content;
using FlaxEditor.GUI.Drag;
using FlaxEditor.GUI.Timeline.Tracks;
using FlaxEditor.Viewport.Previews;
using FlaxEngine;
using Object = FlaxEngine.Object;

namespace FlaxEditor.GUI.Timeline
{
    /// <summary>
    /// The timeline editor for animation asset.
    /// </summary>
    /// <seealso cref="FlaxEditor.GUI.Timeline.Timeline" />
    public sealed class AnimationTimeline : Timeline
    {
        private sealed class Proxy : ProxyBase<AnimationTimeline>
        {
            /// <inheritdoc />
            public Proxy(AnimationTimeline timeline)
            : base(timeline)
            {
            }
        }

        private AnimationPreview _preview;
        internal Guid _id;

        /// <summary>
        /// Gets or sets the animated preview used for the animation playback.
        /// </summary>
        public AnimationPreview Preview
        {
            get => _preview;
            set
            {
                if (_preview == value)
                    return;
                _preview = value;
                value?.PreviewActor.UpdateAnimation();
                UpdatePlaybackState();
                PreviewChanged?.Invoke();
            }
        }

        /// <summary>
        /// Occurs when the selected animated model preview gets changed.
        /// </summary>
        public event Action PreviewChanged;

        /// <summary>
        /// Initializes a new instance of the <see cref="AnimationTimeline"/> class.
        /// </summary>
        /// <param name="undo">The undo/redo to use for the history actions recording. Optional, can be null to disable undo support.</param>
        public AnimationTimeline(FlaxEditor.Undo undo)
        : base(PlaybackButtons.Play | PlaybackButtons.Stop, undo, false, true)
        {
            PlaybackState = PlaybackStates.Seeking;
            ShowPreviewValues = false;
            PropertiesEditObject = new Proxy(this);

            // Setup track types
            TrackArchetypes.Add(AnimationChannelTrack.GetArchetype());
            TrackArchetypes.Add(AnimationChannelDataTrack.GetArchetype());
            TrackArchetypes.Add(AnimationEventTrack.GetArchetype());
            TrackArchetypes.Add(NestedAnimationTrack.GetArchetype());
        }

        /// <summary>
        /// Loads the timeline from the specified <see cref="FlaxEngine.Animation"/> asset.
        /// </summary>
        /// <param name="asset">The asset.</param>
        public void Load(Animation asset)
        {
            Profiler.BeginEvent("Asset.LoadTimeline");
            asset.LoadTimeline(out var data);
            Profiler.EndEvent();

            Profiler.BeginEvent("Timeline.Load");
            Load(data);
            Profiler.EndEvent();
        }

        /// <summary>
        /// Saves the timeline data to the <see cref="FlaxEngine.Animation"/> asset.
        /// </summary>
        /// <param name="asset">The asset.</param>
        public void Save(Animation asset)
        {
            var data = Save();
            asset.SaveTimeline(data);
            asset.Reload();
        }

        private void UpdatePlaybackState()
        {
            PlaybackStates state;
            ShowPlaybackButtonsArea = _preview != null;
            if (_preview != null)
            {
                if (_preview.PlayAnimation)
                {
                    state = PlaybackStates.Playing;
                }
                else
                {
                    state = PlaybackStates.Paused;
                }
                var time = Editor.Internal_GetAnimationTime(Object.GetUnmanagedPtr(_preview.PreviewActor));
                CurrentFrame = (int)(time * FramesPerSecond);
            }
            else
            {
                state = PlaybackStates.Seeking;
            }
            PlaybackState = state;
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            base.Update(deltaTime);

            UpdatePlaybackState();
        }

        /// <inheritdoc />
        public override void OnPlay()
        {
            var time = CurrentTime;
            if (_preview != null)
            {
                _preview.Play();
                Editor.Internal_SetAnimationTime(Object.GetUnmanagedPtr(_preview.PreviewActor), time);
            }

            base.OnPlay();
        }

        /// <inheritdoc />
        public override void OnPause()
        {
            _preview.Pause();

            base.OnPause();
        }

        /// <inheritdoc />
        public override void OnStop()
        {
            _preview.Stop();

            base.OnStop();
        }

        /// <inheritdoc />
        public override void OnSeek(int frame)
        {
            if (_preview != null)
            {
                frame = Mathf.Clamp(frame, 0, DurationFrames);
                var time = frame / FramesPerSecond;
                Editor.Internal_SetAnimationTime(Object.GetUnmanagedPtr(_preview.PreviewActor), time);
                if (!_preview.PlayAnimation)
                    _preview.PreviewActor.UpdateAnimation();
            }
            else
            {
                CurrentFrame = frame;
            }

            base.OnSeek(frame);
        }

        /// <inheritdoc />
        protected override void SetupDragDrop()
        {
            base.SetupDragDrop();

            DragHandlers.Add(new DragHandler(new DragAssets(IsValidAsset), OnDragAsset));
        }

        private static bool IsValidAsset(AssetItem assetItem)
        {
            if (assetItem is BinaryAssetItem binaryAssetItem)
            {
                if (typeof(Animation).IsAssignableFrom(binaryAssetItem.Type))
                {
                    var sceneAnimation = FlaxEngine.Content.LoadAsync<Animation>(binaryAssetItem.ID);
                    if (sceneAnimation)
                        return true;
                }
            }
            return false;
        }

        private static void OnDragAsset(Timeline timeline, DragHelper drag)
        {
            foreach (var assetItem in ((DragAssets)drag).Objects)
            {
                if (assetItem is BinaryAssetItem binaryAssetItem)
                {
                    if (typeof(Animation).IsAssignableFrom(binaryAssetItem.Type))
                    {
                        var animation = FlaxEngine.Content.LoadAsync<Animation>(binaryAssetItem.ID);
                        if (!animation || animation.WaitForLoaded())
                            continue;
                        var track = (NestedAnimationTrack)timeline.NewTrack(NestedAnimationTrack.GetArchetype());
                        track.Asset = animation;
                        track.TrackMedia.DurationFrames = (int)(animation.Length * timeline.FramesPerSecond);
                        track.Rename(assetItem.ShortName);
                        timeline.AddTrack(track);
                    }
                }
            }
        }
    }
}
