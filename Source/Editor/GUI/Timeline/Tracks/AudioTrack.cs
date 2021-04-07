// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.IO;
using FlaxEditor.GUI.Timeline.Undo;
using FlaxEditor.Viewport.Previews;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI.Timeline.Tracks
{
    /// <summary>
    /// The timeline media that represents an audio clip media event.
    /// </summary>
    /// <seealso cref="FlaxEditor.GUI.Timeline.Media" />
    public class AudioMedia : SingleMediaAssetMedia
    {
        /// <summary>
        /// True if loop track, otherwise audio clip will stop on the end.
        /// </summary>
        public bool Loop
        {
            get => Track.Loop;
            set
            {
                if (Track.Loop != value)
                {
                    Track.Loop = value;
                    Preview.DrawMode = value ? AudioClipPreview.DrawModes.Looped : AudioClipPreview.DrawModes.Single;
                }
            }
        }

        private sealed class Proxy : ProxyBase<AudioTrack, AudioMedia>
        {
            /// <summary>
            /// Gets or sets the audio clip to play.
            /// </summary>
            [EditorDisplay("General"), EditorOrder(10), Tooltip("The audio clip to play.")]
            public AudioClip Audio
            {
                get => Track.Asset;
                set => Track.Asset = value;
            }

            /// <summary>
            /// Gets or sets the audio clip looping mode.
            /// </summary>
            [EditorDisplay("General"), EditorOrder(20), Tooltip("If checked, the audio clip will loop when playback exceeds its duration. Otherwise it will stop play.")]
            public bool Loop
            {
                get => Track.TrackLoop;
                set => Track.TrackLoop = value;
            }

            /// <inheritdoc />
            public Proxy(AudioTrack track, AudioMedia media)
            : base(track, media)
            {
            }
        }

        /// <summary>
        /// The audio clip preview.
        /// </summary>
        public AudioClipPreview Preview;

        /// <inheritdoc />
        public AudioMedia()
        {
            Preview = new AudioClipPreview
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                DrawMode = AudioClipPreview.DrawModes.Single,
                Parent = this,
            };
        }

        /// <inheritdoc />
        public override void OnTimelineChanged(Track track)
        {
            base.OnTimelineChanged(track);

            PropertiesEditObject = new Proxy(Track as AudioTrack, this);
        }

        /// <inheritdoc />
        public override void OnTimelineZoomChanged()
        {
            base.OnTimelineZoomChanged();

            Preview.ViewScale = Timeline.UnitsPerSecond / AudioClipPreview.UnitsPerSecond * Timeline.Zoom;
        }
    }

    /// <summary>
    /// The timeline track that represents an audio clip playback.
    /// </summary>
    /// <seealso cref="FlaxEditor.GUI.Timeline.Track" />
    public class AudioTrack : SingleMediaAssetTrack<AudioClip, AudioMedia>
    {
        /// <summary>
        /// Gets the archetype.
        /// </summary>
        /// <returns>The archetype.</returns>
        public static TrackArchetype GetArchetype()
        {
            return new TrackArchetype
            {
                TypeId = 5,
                Name = "Audio",
                Create = options => new AudioTrack(ref options),
                Load = LoadTrack,
                Save = SaveTrack,
            };
        }

        private static void LoadTrack(int version, Track track, BinaryReader stream)
        {
            var e = (AudioTrack)track;
            Guid id = new Guid(stream.ReadBytes(16));
            e.Asset = FlaxEngine.Content.LoadAsync<AudioClip>(id);
            var m = e.TrackMedia;
            m.StartFrame = stream.ReadInt32();
            m.DurationFrames = stream.ReadInt32();
            m.Preview.DrawMode = track.Loop ? AudioClipPreview.DrawModes.Looped : AudioClipPreview.DrawModes.Single;
        }

        private static void SaveTrack(Track track, BinaryWriter stream)
        {
            var e = (AudioTrack)track;
            var assetId = e.Asset?.ID ?? Guid.Empty;

            stream.Write(assetId.ToByteArray());

            if (e.Media.Count != 0)
            {
                var m = e.TrackMedia;
                stream.Write(m.StartFrame);
                stream.Write(m.DurationFrames);
            }
            else
            {
                stream.Write(0);
                stream.Write(track.Timeline.DurationFrames);
            }
        }

        /// <summary>
        /// Gets or sets the audio clip looping mode.
        /// </summary>
        public bool TrackLoop
        {
            get => TrackMedia.Loop;
            set
            {
                AudioMedia media = TrackMedia;
                if (media.Loop == value)
                    return;

                media.Loop = value;
                Timeline?.MarkAsEdited();
            }
        }

        private Button _addButton;

        /// <inheritdoc />
        public AudioTrack(ref TrackCreateOptions options)
        : base(ref options)
        {
            // Add button
            const float buttonSize = 14;
            _addButton = new Button
            {
                Text = "+",
                TooltipText = "Add sub-tracks",
                AutoFocus = true,
                AnchorPreset = AnchorPresets.MiddleRight,
                IsScrollable = false,
                Offsets = new Margin(-buttonSize - 2 + _muteCheckbox.Offsets.Left, buttonSize, buttonSize * -0.5f, buttonSize),
                Parent = this,
            };
            _addButton.Clicked += OnAddButtonClicked;
            _picker.Location = new Vector2(_addButton.Left - _picker.Width - 2, 2);
        }

        private void OnAddButtonClicked()
        {
            var cm = new ContextMenu.ContextMenu();
            cm.AddButton("Volume", OnAddVolumeTrack);
            cm.Show(_addButton.Parent, _addButton.BottomLeft);
        }

        private void OnAddVolumeTrack()
        {
            var track = Timeline.NewTrack(AudioVolumeTrack.GetArchetype());
            track.ParentTrack = this;
            track.TrackIndex = TrackIndex + 1;
            track.Name = Guid.NewGuid().ToString("N");
            Timeline.AddTrack(track);

            Expand();
        }

        /// <inheritdoc />
        protected override void OnSubTracksChanged()
        {
            base.OnSubTracksChanged();

            _addButton.Enabled = SubTracks.Count == 0;
        }

        /// <inheritdoc />
        protected override void OnAssetChanged()
        {
            base.OnAssetChanged();

            TrackMedia.Preview.Asset = Asset;
        }
    }

    /// <summary>
    /// The child volume track for audio track. Used to animate audio volume over time.
    /// </summary>
    /// <seealso cref="FlaxEditor.GUI.Timeline.Track" />
    class AudioVolumeTrack : Track
    {
        /// <summary>
        /// Gets the archetype.
        /// </summary>
        /// <returns>The archetype.</returns>
        public static TrackArchetype GetArchetype()
        {
            return new TrackArchetype
            {
                TypeId = 6,
                Name = "Audio Volume",
                DisableSpawnViaGUI = true,
                Create = options => new AudioVolumeTrack(ref options),
                Load = LoadTrack,
                Save = SaveTrack,
            };
        }

        private static void LoadTrack(int version, Track track, BinaryReader stream)
        {
            var e = (AudioVolumeTrack)track;
            int count = stream.ReadInt32();
            var keyframes = new BezierCurve<float>.Keyframe[count];
            for (int i = 0; i < count; i++)
            {
                keyframes[i] = new BezierCurve<float>.Keyframe
                {
                    Time = stream.ReadSingle(),
                    Value = stream.ReadSingle(),
                    TangentIn = stream.ReadSingle(),
                    TangentOut = stream.ReadSingle(),
                };
            }
            e.Curve.SetKeyframes(keyframes);
        }

        private static void SaveTrack(Track track, BinaryWriter stream)
        {
            var e = (AudioVolumeTrack)track;
            var keyframes = e.Curve.Keyframes;
            int count = keyframes.Count;
            stream.Write(count);
            for (int i = 0; i < count; i++)
            {
                var keyframe = keyframes[i];
                stream.Write(keyframe.Time);
                stream.Write(keyframe.Value);
                stream.Write(keyframe.TangentIn);
                stream.Write(keyframe.TangentOut);
            }
        }

        private AudioMedia _audioMedia;
        private const float CollapsedHeight = 20.0f;
        private const float ExpandedHeight = 64.0f;
        private Label _previewValue;
        private byte[] _curveEditingStartData;

        /// <summary>
        /// The volume curve. Values can be in range 0-1 to animate volume intensity and the track playback starts at the parent audio track media beginning. This curve does not loop.
        /// </summary>
        public BezierCurveEditor<float> Curve;

        /// <inheritdoc />
        public AudioVolumeTrack(ref TrackCreateOptions options)
        : base(ref options)
        {
            Title = "Volume";
            Height = CollapsedHeight;

            // Curve editor
            Curve = new BezierCurveEditor<float>
            {
                Visible = false,
                EnableZoom = CurveEditorBase.UseMode.Off,
                EnablePanning = CurveEditorBase.UseMode.Off,
                ScrollBars = ScrollBars.None,
                DefaultValue = 1.0f,
                ShowStartEndLines = true,
            };
            Curve.Edited += OnCurveEdited;
            Curve.EditingStart += OnCurveEditingStart;
            Curve.EditingEnd += OnCurveEditingEnd;
            Curve.UnlockChildrenRecursive();

            // Navigation buttons
            const float buttonSize = 14;
            var icons = Editor.Instance.Icons;
            var rightKey = new ClickableImage
            {
                TooltipText = "Sets the time to the next key",
                AutoFocus = true,
                AnchorPreset = AnchorPresets.MiddleRight,
                IsScrollable = false,
                Color = Style.Current.ForegroundGrey,
                Margin = new Margin(1),
                Brush = new SpriteBrush(icons.ArrowRight32),
                Offsets = new Margin(-buttonSize - 2 + _muteCheckbox.Offsets.Left, buttonSize, buttonSize * -0.5f, buttonSize),
                Parent = this,
            };
            rightKey.Clicked += OnRightKeyClicked;
            var addKey = new ClickableImage
            {
                TooltipText = "Adds a new key at the current time",
                AutoFocus = true,
                AnchorPreset = AnchorPresets.MiddleRight,
                IsScrollable = false,
                Color = Style.Current.ForegroundGrey,
                Margin = new Margin(3),
                Brush = new SpriteBrush(icons.Add48),
                Offsets = new Margin(-buttonSize - 2 + rightKey.Offsets.Left, buttonSize, buttonSize * -0.5f, buttonSize),
                Parent = this,
            };
            addKey.Clicked += OnAddKeyClicked;
            var leftKey = new ClickableImage
            {
                TooltipText = "Sets the time to the previous key",
                AutoFocus = true,
                AnchorPreset = AnchorPresets.MiddleRight,
                IsScrollable = false,
                Color = Style.Current.ForegroundGrey,
                Margin = new Margin(1),
                Brush = new SpriteBrush(icons.ArrowLeft32),
                Offsets = new Margin(-buttonSize - 2 + addKey.Offsets.Left, buttonSize, buttonSize * -0.5f, buttonSize),
                Parent = this,
            };
            leftKey.Clicked += OnLeftKeyClicked;

            // Value preview
            var previewWidth = 50.0f;
            _previewValue = new Label
            {
                AutoFocus = true,
                AnchorPreset = AnchorPresets.MiddleRight,
                IsScrollable = false,
                HorizontalAlignment = TextAlignment.Near,
                Color = Style.Current.ForegroundGrey,
                Margin = new Margin(1),
                Offsets = new Margin(-previewWidth - 2 + leftKey.Offsets.Left, previewWidth, TextBox.DefaultHeight * -0.5f, TextBox.DefaultHeight),
                Parent = this,
            };
        }

        private void OnRightKeyClicked(Image image, MouseButton button)
        {
            if (button == MouseButton.Left && GetNextKeyframeFrame(Timeline.CurrentTime, out var frame))
            {
                Timeline.OnSeek(frame);
            }
        }

        /// <inheritdoc />
        protected override void OnContextMenu(ContextMenu.ContextMenu menu)
        {
            base.OnContextMenu(menu);

            if (_audioMedia == null || Curve == null)
                return;
            menu.AddSeparator();
            menu.AddButton("Copy Preview Value", () =>
            {
                var time = (Timeline.CurrentFrame - _audioMedia.StartFrame) / Timeline.FramesPerSecond;
                Curve.Evaluate(out var value, time, false);
                Clipboard.Text = Utils.RoundTo2DecimalPlaces(Mathf.Saturate(value)).ToString("0.00");
            }).LinkTooltip("Copies the current track value to the clipboard").Enabled = Timeline.ShowPreviewValues;
        }

        /// <inheritdoc />
        public override bool GetNextKeyframeFrame(float time, out int result)
        {
            if (_audioMedia != null)
            {
                var mediaTime = time - _audioMedia.StartFrame / Timeline.FramesPerSecond;
                for (int i = 0; i < Curve.Keyframes.Count; i++)
                {
                    var k = Curve.Keyframes[i];
                    if (k.Time > mediaTime)
                    {
                        result = Mathf.FloorToInt(k.Time * Timeline.FramesPerSecond) + _audioMedia.StartFrame;
                        return true;
                    }
                }
            }
            return base.GetNextKeyframeFrame(time, out result);
        }

        private void OnAddKeyClicked(Image image, MouseButton button)
        {
            var currentFrame = Timeline.CurrentFrame;
            if (button == MouseButton.Left && _audioMedia != null && currentFrame >= _audioMedia.StartFrame && currentFrame < _audioMedia.StartFrame + _audioMedia.DurationFrames)
            {
                var time = (currentFrame - _audioMedia.StartFrame) / Timeline.FramesPerSecond;
                for (int i = Curve.Keyframes.Count - 1; i >= 0; i--)
                {
                    var k = Curve.Keyframes[i];
                    var frame = Mathf.FloorToInt(k.Time * Timeline.FramesPerSecond) + _audioMedia.StartFrame;
                    if (frame == Timeline.CurrentFrame)
                    {
                        // Already added
                        return;
                    }
                }

                using (new TrackUndoBlock(this))
                    Curve.AddKeyframe(new BezierCurve<float>.Keyframe(time, 1.0f));
            }
        }

        private void OnLeftKeyClicked(Image image, MouseButton button)
        {
            if (button == MouseButton.Left && GetPreviousKeyframeFrame(Timeline.CurrentTime, out var frame))
            {
                Timeline.OnSeek(frame);
            }
        }

        /// <inheritdoc />
        public override bool GetPreviousKeyframeFrame(float time, out int result)
        {
            if (_audioMedia != null)
            {
                var mediaTime = time - _audioMedia.StartFrame / Timeline.FramesPerSecond;
                for (int i = Curve.Keyframes.Count - 1; i >= 0; i--)
                {
                    var k = Curve.Keyframes[i];
                    if (k.Time < mediaTime)
                    {
                        result = Mathf.FloorToInt(k.Time * Timeline.FramesPerSecond) + _audioMedia.StartFrame;
                        return true;
                    }
                }
            }
            return base.GetPreviousKeyframeFrame(time, out result);
        }

        private void UpdatePreviewValue()
        {
            if (_audioMedia == null || Curve == null || Timeline == null)
                return;

            var time = (Timeline.CurrentFrame - _audioMedia.StartFrame) / Timeline.FramesPerSecond;
            Curve.Evaluate(out var value, time, false);
            _previewValue.Text = Utils.RoundTo2DecimalPlaces(Mathf.Saturate(value)).ToString("0.00");
        }

        private void UpdateCurve()
        {
            if (_audioMedia == null || Curve == null || Timeline == null)
                return;

            Curve.Bounds = new Rectangle(_audioMedia.X, Y + 1.0f, _audioMedia.Width, Height - 2.0f);

            var expanded = IsExpanded;
            if (expanded)
            {
                //Curve.ViewScale = new Vector2(1.0f, CurveEditor<float>.UnitsPerSecond / Curve.Height);
                Curve.ViewScale = new Vector2(Timeline.Zoom, 0.4f);
                Curve.ViewOffset = new Vector2(0.0f, 30.0f);
            }
            else
            {
                Curve.ViewScale = new Vector2(Timeline.Zoom, 1.0f);
                Curve.ViewOffset = Vector2.Zero;
            }
            Curve.ShowCollapsed = !expanded;
            Curve.ShowBackground = expanded;
            Curve.ShowAxes = expanded;
            Curve.Visible = Visible;
            Curve.UpdateKeyframes();
        }

        private void OnCurveEdited()
        {
            UpdatePreviewValue();
            Timeline.MarkAsEdited();
        }

        private void OnCurveEditingStart()
        {
            _curveEditingStartData = EditTrackAction.CaptureData(this);
        }

        private void OnCurveEditingEnd()
        {
            var after = EditTrackAction.CaptureData(this);
            if (!Utils.ArraysEqual(_curveEditingStartData, after))
                Timeline.Undo.AddAction(new EditTrackAction(Timeline, this, _curveEditingStartData, after));
            _curveEditingStartData = null;
        }

        /// <inheritdoc />
        protected override bool CanDrag => false;

        /// <inheritdoc />
        protected override bool CanRename => false;

        /// <inheritdoc />
        protected override bool CanExpand => true;

        /// <inheritdoc />
        public override void OnParentTrackChanged(Track parent)
        {
            base.OnParentTrackChanged(parent);

            if (_audioMedia != null)
            {
                _audioMedia.StartFrameChanged -= UpdateCurve;
                _audioMedia.DurationFramesChanged -= UpdateCurve;
                _audioMedia = null;
            }

            if (parent is AudioTrack audioTrack)
            {
                var media = audioTrack.TrackMedia;
                media.StartFrameChanged += UpdateCurve;
                media.DurationFramesChanged += UpdateCurve;
                _audioMedia = media;
                UpdateCurve();
                UpdatePreviewValue();
            }
        }

        /// <inheritdoc />
        protected override void OnExpandedChanged()
        {
            Height = IsExpanded ? ExpandedHeight : CollapsedHeight;
            UpdateCurve();

            base.OnExpandedChanged();
        }

        /// <inheritdoc />
        protected override void OnVisibleChanged()
        {
            base.OnVisibleChanged();

            Curve.Visible = Visible;
        }

        /// <inheritdoc />
        public override void OnTimelineChanged(Timeline timeline)
        {
            base.OnTimelineChanged(timeline);

            Curve.Parent = timeline?.MediaPanel;
            Curve.FPS = timeline?.FramesPerSecond;
            UpdateCurve();
            UpdatePreviewValue();
        }

        /// <inheritdoc />
        public override void OnUndo()
        {
            base.OnUndo();

            if (Curve != null)
            {
                // TODO: fix this hack: curve UI doesn't update properly after undo that modified keyframes
                IsExpanded = !IsExpanded;
                IsExpanded = !IsExpanded;
            }
            UpdatePreviewValue();
        }

        /// <inheritdoc />
        public override void OnTimelineZoomChanged()
        {
            base.OnTimelineZoomChanged();

            UpdateCurve();
        }

        /// <inheritdoc />
        public override void OnTimelineArrange()
        {
            base.OnTimelineArrange();

            UpdateCurve();
        }

        /// <inheritdoc />
        public override void OnTimelineFpsChanged(float before, float after)
        {
            base.OnTimelineFpsChanged(before, after);

            Curve.FPS = after;
            UpdatePreviewValue();
        }

        /// <inheritdoc />
        public override void OnTimelineCurrentFrameChanged(int frame)
        {
            base.OnTimelineCurrentFrameChanged(frame);

            UpdatePreviewValue();
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            if (Curve != null)
            {
                Curve.Dispose();
                Curve = null;
            }
            _previewValue = null;

            base.OnDestroy();
        }
    }
}
