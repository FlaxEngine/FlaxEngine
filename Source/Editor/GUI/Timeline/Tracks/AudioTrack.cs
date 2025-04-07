// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using FlaxEditor.Utilities;
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
    public class AudioMedia : Media
    {
        /// <summary>
        /// True if loop track, otherwise audio clip will stop on the end.
        /// </summary>
        public bool Loop
        {
            get => Track.Loop;
            set
            {
                if (Loop == value)
                    return;
                Track.Loop = value;
                Preview.DrawMode = value ? AudioClipPreview.DrawModes.Looped : AudioClipPreview.DrawModes.Single;
            }
        }

        /// <summary>
        /// Playback offset of the audio (in seconds).
        /// </summary>
        public float Offset
        {
            get => Preview.ViewOffset;
            set
            {
                if (Mathf.NearEqual(Preview.ViewOffset, value))
                    return;
                Preview.ViewOffset = value;
                Timeline?.MarkAsEdited();
            }
        }

        private sealed class Proxy : ProxyBase<AudioTrack, AudioMedia>
        {
            [EditorDisplay("General"), EditorOrder(10), Tooltip("The audio clip to play.")]
            public AudioClip Audio
            {
                get => Track.Asset;
                set => Track.Asset = value;
            }

            [EditorDisplay("General"), EditorOrder(20), Tooltip("If checked, the audio clip will loop when playback exceeds its duration. Otherwise it will stop play.")]
            public bool Loop
            {
                get => Media.Loop;
                set => Media.Loop = value;
            }

            [EditorDisplay("General"), EditorOrder(30), Tooltip("Playback offset of the audio (in seconds).")]
            [Limit(0, float.MaxValue, 0.01f)]
            public float Offset
            {
                get => Media.Offset;
                set => Media.Offset = value;
            }

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
            CanSplit = true;
            CanDelete = true;
            Preview = new AudioClipPreview
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                DrawMode = AudioClipPreview.DrawModes.Single,
                Parent = this,
            };
        }

        /// <inheritdoc />
        protected override void OnStartFrameChanged()
        {
            base.OnStartFrameChanged();

            if (Track != null && Track.SubTracks.Count != 0 && Track.SubTracks[0] is AudioVolumeTrack volumeTrack)
                volumeTrack.UpdateCurve();
        }

        /// <inheritdoc />
        protected override void OnDurationFramesChanged()
        {
            base.OnDurationFramesChanged();

            if (Track != null && Track.SubTracks.Count != 0 && Track.SubTracks[0] is AudioVolumeTrack volumeTrack)
                volumeTrack.UpdateCurve();
        }

        /// <inheritdoc />
        public override void OnTimelineChanged(Track track)
        {
            base.OnTimelineChanged(track);

            PropertiesEditObject = track != null ? new Proxy((AudioTrack)track, this) : null;
        }

        /// <inheritdoc />
        public override void OnTimelineZoomChanged()
        {
            base.OnTimelineZoomChanged();

            Preview.ViewScale = Timeline.UnitsPerSecond / AudioClipPreview.UnitsPerSecond * Timeline.Zoom;
        }

        /// <inheritdoc />
        public override Media Split(int frame)
        {
            var offset = Offset + ((float)(frame - StartFrame) / DurationFrames) * Duration;
            var clone = (AudioMedia)base.Split(frame);
            clone.Preview.ViewOffset = offset;
            clone.Preview.Asset = Preview.Asset;
            clone.Preview.DrawMode = Preview.DrawMode;
            return clone;
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
            Guid id = stream.ReadGuid();
            e.Asset = FlaxEngine.Content.LoadAsync<AudioClip>(id);
            if (version <= 3)
            {
                // [Deprecated on 03.09.2021 expires on 03.09.2023]
                var m = e.TrackMedia;
                m.StartFrame = stream.ReadInt32();
                m.DurationFrames = stream.ReadInt32();
                m.Preview.ViewOffset = 0.0f;
                m.Preview.DrawMode = track.Loop ? AudioClipPreview.DrawModes.Looped : AudioClipPreview.DrawModes.Single;
            }
            else
            {
                var count = stream.ReadInt32();
                while (e.Media.Count > count)
                    e.RemoveMedia(e.Media.Last());
                while (e.Media.Count < count)
                    e.AddMedia(new AudioMedia());
                for (int i = 0; i < count; i++)
                {
                    var m = (AudioMedia)e.Media[i];
                    m.StartFrame = stream.ReadInt32();
                    m.DurationFrames = stream.ReadInt32();
                    m.Preview.ViewOffset = stream.ReadSingle();
                    m.Preview.DrawMode = track.Loop ? AudioClipPreview.DrawModes.Looped : AudioClipPreview.DrawModes.Single;
                    m.Preview.Asset = e.Asset;
                }
            }
        }

        private static void SaveTrack(Track track, BinaryWriter stream)
        {
            var e = (AudioTrack)track;
            stream.WriteGuid(ref e.AssetID);
            var count = e.Media.Count;
            stream.Write(count);
            for (int i = 0; i < count; i++)
            {
                var m = (AudioMedia)e.Media[i];
                stream.Write(m.StartFrame);
                stream.Write(m.DurationFrames);
                stream.Write(m.Offset);
            }
        }

        private Button _addButton;

        /// <inheritdoc />
        public AudioTrack(ref TrackCreateOptions options)
        : base(ref options)
        {
            MinMediaCount = 1;

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
            _picker.Location = new Float2(_addButton.Left - _picker.Width - 2, 2);
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

            foreach (AudioMedia m in Media)
                m.Preview.Asset = Asset;
        }
    }

    /// <summary>
    /// The child volume track for audio track. Used to animate audio volume over time.
    /// </summary>
    /// <seealso cref="FlaxEditor.GUI.Timeline.Track" />
    class AudioVolumeTrack : Track, IKeyframesEditorContext
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
                ShowBackground = false,
            };
            Curve.Edited += OnCurveEdited;
            Curve.EditingStart += OnCurveEditingStart;
            Curve.EditingEnd += OnCurveEditingEnd;
            Curve.UnlockChildrenRecursive();

            // Navigation buttons
            const float keySize = 18;
            const float addSize = 20;
            var icons = Editor.Instance.Icons;
            var rightKey = new Image
            {
                TooltipText = "Sets the time to the next key",
                AutoFocus = true,
                AnchorPreset = AnchorPresets.MiddleRight,
                IsScrollable = false,
                Color = Style.Current.ForegroundGrey,
                Margin = new Margin(1),
                Brush = new SpriteBrush(icons.Right32),
                Offsets = new Margin(-keySize - 2 + _muteCheckbox.Offsets.Left, keySize, keySize * -0.5f, keySize),
                Parent = this,
            };
            rightKey.Clicked += OnRightKeyClicked;
            var addKey = new Image
            {
                TooltipText = "Adds a new key at the current time",
                AutoFocus = true,
                AnchorPreset = AnchorPresets.MiddleRight,
                IsScrollable = false,
                Color = Style.Current.ForegroundGrey,
                Margin = new Margin(3),
                Brush = new SpriteBrush(icons.Add32),
                Offsets = new Margin(-addSize - 2 + rightKey.Offsets.Left, addSize, addSize * -0.5f, addSize),
                Parent = this,
            };
            addKey.Clicked += OnAddKeyClicked;
            var leftKey = new Image
            {
                TooltipText = "Sets the time to the previous key",
                AutoFocus = true,
                AnchorPreset = AnchorPresets.MiddleRight,
                IsScrollable = false,
                Color = Style.Current.ForegroundGrey,
                Margin = new Margin(1),
                Brush = new SpriteBrush(icons.Left32),
                Offsets = new Margin(-keySize - 2 + addKey.Offsets.Left, keySize, keySize * -0.5f, keySize),
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
                TextColor = Style.Current.ForegroundGrey,
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

        private bool GetRangeFrames(out int startFrame, out int endFrame)
        {
            if (ParentTrack != null && ParentTrack.Media.Count != 0)
            {
                startFrame = ParentTrack.Media[0].StartFrame;
                endFrame = ParentTrack.Media[0].EndFrame;
                for (int i = 1; i < ParentTrack.Media.Count; i++)
                {
                    endFrame = Mathf.Max(endFrame, ParentTrack.Media[i].EndFrame);
                }
                return true;
            }
            startFrame = endFrame = 0;
            return false;
        }

        private bool GetRangeMedia(out Media startMedia, out Media endMedia)
        {
            if (ParentTrack != null && ParentTrack.Media.Count != 0)
            {
                startMedia = endMedia = ParentTrack.Media[0];
                for (int i = 1; i < ParentTrack.Media.Count; i++)
                {
                    if (ParentTrack.Media[i].EndFrame >= endMedia.EndFrame)
                        endMedia = ParentTrack.Media[i];
                }
                return true;
            }
            startMedia = endMedia = null;
            return false;
        }

        /// <inheritdoc />
        protected override void OnContextMenu(ContextMenu.ContextMenu menu)
        {
            base.OnContextMenu(menu);

            if (!GetRangeFrames(out var startFrame, out _) || Curve == null)
                return;
            menu.AddSeparator();
            menu.AddButton("Copy Preview Value", () =>
            {
                var time = (Timeline.CurrentFrame - startFrame) / Timeline.FramesPerSecond;
                Curve.Evaluate(out var value, time, false);
                Clipboard.Text = FlaxEngine.Utils.RoundTo2DecimalPlaces(Mathf.Saturate(value)).ToString("0.00");
            }).LinkTooltip("Copies the current track value to the clipboard").Enabled = Timeline.ShowPreviewValues;
        }

        /// <inheritdoc />
        public override bool GetNextKeyframeFrame(float time, out int result)
        {
            if (GetRangeFrames(out var startFrame, out var endFrame))
            {
                var mediaTime = time - startFrame / Timeline.FramesPerSecond;
                for (int i = 0; i < Curve.Keyframes.Count; i++)
                {
                    var k = Curve.Keyframes[i];
                    if (k.Time > mediaTime)
                    {
                        result = Mathf.FloorToInt(k.Time * Timeline.FramesPerSecond) + startFrame;
                        return true;
                    }
                }
            }
            return base.GetNextKeyframeFrame(time, out result);
        }

        private void OnAddKeyClicked(Image image, MouseButton button)
        {
            var currentFrame = Timeline.CurrentFrame;
            if (button == MouseButton.Left && GetRangeFrames(out var startFrame, out var endFrame) && currentFrame >= startFrame && currentFrame < endFrame)
            {
                var time = (currentFrame - startFrame) / Timeline.FramesPerSecond;
                for (int i = Curve.Keyframes.Count - 1; i >= 0; i--)
                {
                    var k = Curve.Keyframes[i];
                    var frame = Mathf.FloorToInt(k.Time * Timeline.FramesPerSecond) + startFrame;
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
            if (GetRangeFrames(out var startFrame, out _))
            {
                var mediaTime = time - startFrame / Timeline.FramesPerSecond;
                for (int i = Curve.Keyframes.Count - 1; i >= 0; i--)
                {
                    var k = Curve.Keyframes[i];
                    if (k.Time < mediaTime)
                    {
                        result = Mathf.FloorToInt(k.Time * Timeline.FramesPerSecond) + startFrame;
                        return true;
                    }
                }
            }
            return base.GetPreviousKeyframeFrame(time, out result);
        }

        private void UpdatePreviewValue()
        {
            if (!GetRangeFrames(out var startFrame, out _) || Curve == null || Timeline == null)
                return;

            var time = (Timeline.CurrentFrame - startFrame) / Timeline.FramesPerSecond;
            Curve.Evaluate(out var value, time, false);
            _previewValue.Text = FlaxEngine.Utils.RoundTo2DecimalPlaces(Mathf.Saturate(value)).ToString("0.00");
        }

        internal void UpdateCurve()
        {
            if (!GetRangeMedia(out var startMedia, out var endMedia) || Curve == null || Timeline == null)
                return;
            bool wasVisible = Curve.Visible;
            Curve.Visible = Visible;
            if (!Visible)
            {
                if (wasVisible)
                    Curve.ClearSelection();
                return;
            }
            Curve.KeyframesEditorContext = Timeline;
            Curve.CustomViewPanning = Timeline.OnKeyframesViewPanning;
            Curve.Bounds = new Rectangle(startMedia.X, Y + 1.0f, endMedia.Right - startMedia.Left, Height - 2.0f);
            var expanded = IsExpanded;
            if (expanded)
            {
                Curve.ViewScale = new Float2(Timeline.Zoom, 0.7f);
                Curve.ViewOffset = new Float2(0.0f, 35.0f);
            }
            else
            {
                Curve.ViewScale = new Float2(Timeline.Zoom, 1.0f);
                Curve.ViewOffset = Float2.Zero;
            }
            Curve.ShowCollapsed = !expanded;
            Curve.ShowAxes = expanded ? CurveEditorBase.UseMode.Horizontal : CurveEditorBase.UseMode.Off;
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
            if (!FlaxEngine.Utils.ArraysEqual(_curveEditingStartData, after))
                Timeline.AddBatchedUndoAction(new EditTrackAction(Timeline, this, _curveEditingStartData, after));
            _curveEditingStartData = null;
        }

        /// <inheritdoc />
        public override bool CanDrag => false;

        /// <inheritdoc />
        public override bool CanRename => false;

        /// <inheritdoc />
        public override bool CanCopyPaste => false;

        /// <inheritdoc />
        public override bool CanExpand => true;

        /// <inheritdoc />
        public override void OnParentTrackChanged(Track parent)
        {
            base.OnParentTrackChanged(parent);

            if (parent != null)
            {
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
        public override void OnDuplicated(Track clone)
        {
            base.OnDuplicated(clone);

            clone.Name = Guid.NewGuid().ToString("N");
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

        /// <inheritdoc />
        public void OnKeyframesDeselect(IKeyframesEditor editor)
        {
            if (Curve != null && Curve.Visible)
                Curve.OnKeyframesDeselect(editor);
        }

        /// <inheritdoc />
        public void OnKeyframesSelection(IKeyframesEditor editor, ContainerControl control, Rectangle selection)
        {
            if (Curve != null && Curve.Visible)
                Curve.OnKeyframesSelection(editor, control, selection);
        }

        /// <inheritdoc />
        public int OnKeyframesSelectionCount()
        {
            return Curve != null && Curve.Visible ? Curve.OnKeyframesSelectionCount() : 0;
        }

        /// <inheritdoc />
        public void OnKeyframesDelete(IKeyframesEditor editor)
        {
            if (Curve != null && Curve.Visible)
                Curve.OnKeyframesDelete(editor);
        }

        /// <inheritdoc />
        public void OnKeyframesMove(IKeyframesEditor editor, ContainerControl control, Float2 location, bool start, bool end)
        {
            if (Curve != null && Curve.Visible)
                Curve.OnKeyframesMove(editor, control, location, start, end);
        }

        /// <inheritdoc />
        public void OnKeyframesCopy(IKeyframesEditor editor, float? timeOffset, System.Text.StringBuilder data)
        {
            if (Curve != null && Curve.Visible)
                Curve.OnKeyframesCopy(editor, timeOffset, data);
        }

        /// <inheritdoc />
        public void OnKeyframesPaste(IKeyframesEditor editor, float? timeOffset, string[] datas, ref int index)
        {
            if (Curve != null && Curve.Visible)
                Curve.OnKeyframesPaste(editor, timeOffset, datas, ref index);
        }

        /// <inheritdoc />
        public void OnKeyframesGet(Action<string, float, object> get)
        {
            Curve?.OnKeyframesGet(Name, get);
        }

        /// <inheritdoc />
        public void OnKeyframesSet(List<KeyValuePair<float, object>> keyframes)
        {
            Curve?.OnKeyframesSet(keyframes);
        }
    }
}
