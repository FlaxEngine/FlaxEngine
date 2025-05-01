// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI.Timeline.Tracks
{
    /// <summary>
    /// The timeline track that can proxy the sub-tracks keyframes for easier batched editing.
    /// </summary>
    /// <seealso cref="FlaxEditor.GUI.Timeline.Track" />
    public abstract class ConductorTrack : Track, IKeyframesEditorContext
    {
        private sealed class ConductionUndoAction : IUndoAction
        {
            public bool IsDo;
            public Timeline Timeline;
            public string Track;

            private void Refresh()
            {
                Refresh(Timeline.FindTrack(Track));
            }

            private void Refresh(Track track)
            {
                if (track is ConductorTrack conductor && conductor.Proxy != null)
                {
                    conductor.LoadProxy();
                    foreach (var subTrack in conductor.SubTracks)
                        Refresh(subTrack);
                }
            }

            public string ActionString => "Edit track";

            public void Do()
            {
                if (IsDo)
                    Refresh();
            }

            public void Undo()
            {
                if (!IsDo)
                    Refresh();
            }

            public void Dispose()
            {
                Timeline = null;
                Track = null;
            }
        }

        private HashSet<string> _proxyTracks;

        /// <summary>
        /// The batched keyframes editor for all sub-tracks.
        /// </summary>
        protected KeyframesEditor Proxy;

        /// <summary>
        /// Initializes a new instance of the <see cref="ConductorTrack"/> class.
        /// </summary>
        /// <param name="options">The track initial options.</param>
        /// <param name="useProxyKeyframes">True if show sub-tracks keyframes as a proxy on this track, otherwise false.</param>
        protected ConductorTrack(ref TrackCreateOptions options, bool useProxyKeyframes = true)
        : base(ref options)
        {
            if (useProxyKeyframes)
            {
                // Proxy keyframes editor
                Proxy = new KeyframesEditor
                {
                    EnableZoom = false,
                    EnablePanning = false,
                    EnableKeyframesValueEdit = false,
                    DefaultValue = new ProxyKey(),
                    ScrollBars = ScrollBars.None,
                };
                Proxy.Edited += OnProxyEdited;
                Proxy.EditingEnd += OnProxyEditingEnd;
                Proxy.UnlockChildrenRecursive();
            }
        }

        private void UpdateProxyKeyframes()
        {
            if (Proxy == null || Timeline == null)
                return;
            bool wasVisible = Proxy.Visible;
            Proxy.Visible = Visible && IsCollapsed && SubTracks.Any(x => x is IKeyframesEditorContext);
            if (!Visible)
            {
                if (wasVisible)
                    Proxy.ClearSelection();
                return;
            }
            Proxy.KeyframesEditorContext = Timeline;
            Proxy.CustomViewPanning = Timeline.OnKeyframesViewPanning;
            Proxy.Bounds = new Rectangle(Timeline.StartOffset, Y + 1.0f, Timeline.Duration * Timeline.UnitsPerSecond * Timeline.Zoom, Height - 2.0f);
            Proxy.ViewScale = new Float2(Timeline.Zoom, 1.0f);
            if (!wasVisible)
                LoadProxy();
            else
                Proxy.UpdateKeyframes();
        }

        private void OnProxyEdited()
        {
            Timeline.MarkAsEdited();
        }

        private void OnProxyEditingEnd()
        {
            SaveProxy();
        }

        /// <summary>
        /// The proxy key data.
        /// </summary>
        public struct ProxyKey
        {
            /// <summary>
            /// The keyframes.
            /// </summary>
            [EditorDisplay("Keyframes", EditorDisplayAttribute.InlineStyle), ExpandGroups]
            [Collection(CanReorderItems = false, CanResize = true)]
            public List<KeyValuePair<string, object>> Keyframes;

            /// <inheritdoc />
            public override string ToString()
            {
                if (Keyframes == null || Keyframes.Count == 0)
                    return string.Empty;
                var sb = new StringBuilder();
                for (int i = 0; i < Keyframes.Count; i++)
                {
                    if (i != 0)
                        sb.Append(", ");
                    var k = Keyframes[i];
                    if (k.Value != null)
                        sb.Append(k.Value);
                }
                return sb.ToString();
            }
        }

        private void LoadProxy()
        {
            if (_proxyTracks == null)
                _proxyTracks = new HashSet<string>();
            else
                _proxyTracks.Clear();

            // Build keyframes list that contains keys from all tracks to proxy
            var keyframes = new List<KeyframesEditor.Keyframe>();
            var eps = Proxy.FPS.HasValue ? 0.5f / Proxy.FPS.Value : Mathf.Epsilon;
            Action<string, float, object> getter = (trackName, time, value) =>
            {
                _proxyTracks.Add(trackName);
                int idx = keyframes.FindIndex(x => Mathf.Abs(x.Time - time) <= eps);
                if (idx == -1)
                {
                    idx = keyframes.Count;
                    keyframes.Add(new KeyframesEditor.Keyframe(time, new ProxyKey { Keyframes = new List<KeyValuePair<string, object>>() }));
                }
                ((ProxyKey)keyframes[idx].Value).Keyframes.Add(new KeyValuePair<string, object>(trackName, value));
            };
            foreach (var subTrack in SubTracks)
            {
                var trackContext = subTrack as IKeyframesEditorContext;
                trackContext?.OnKeyframesGet(getter);
            }

            Proxy.SetKeyframes(keyframes);
        }

        private void SaveProxy()
        {
            // Collect keyframes list per track
            var perTrackKeyframes = new Dictionary<string, List<KeyValuePair<float, object>>>();
            var keyframes = Proxy.Keyframes;
            foreach (var keyframe in keyframes)
            {
                var proxy = (ProxyKey)keyframe.Value;
                if (proxy.Keyframes == null)
                    continue;
                foreach (var e in proxy.Keyframes)
                {
                    if (!perTrackKeyframes.TryGetValue(e.Key, out var trackKeyframes))
                    {
                        trackKeyframes = new List<KeyValuePair<float, object>>();
                        perTrackKeyframes.Add(e.Key, trackKeyframes);
                    }
                    trackKeyframes.Add(new KeyValuePair<float, object>(keyframe.Time, e.Value));
                }
            }

            // Apply the new keyframes to all tracks in this proxy
            Timeline.AddBatchedUndoAction(new ConductionUndoAction { IsDo = false, Timeline = Timeline, Track = Name });
            foreach (var trackName in _proxyTracks)
            {
                var track = Timeline.FindTrack(trackName);
                if (track == null)
                {
                    Editor.LogWarning(string.Format("Failed to find track {0} for keyframes proxy on track {1}.", trackName, Title ?? Name));
                    continue;
                }
                var trackContext = track as IKeyframesEditorContext;
                if (trackContext == null)
                {
                    Editor.LogWarning(string.Format("Track {0} used keyframes proxy on track {1} is invalid.", track.Title ?? track.Name, Title ?? Name));
                    continue;
                }
                perTrackKeyframes.TryGetValue(trackName, out var trackKeyframes);
                trackContext.OnKeyframesSet(trackKeyframes);
            }
            Timeline.AddBatchedUndoAction(new ConductionUndoAction { IsDo = true, Timeline = Timeline, Track = Name });
        }

        /// <inheritdoc />
        protected override void OnVisibleChanged()
        {
            base.OnVisibleChanged();

            UpdateProxyKeyframes();
        }

        /// <inheritdoc />
        protected override void OnExpandedChanged()
        {
            base.OnExpandedChanged();

            UpdateProxyKeyframes();
        }

        /// <inheritdoc />
        public override void OnTimelineChanged(Timeline timeline)
        {
            base.OnTimelineChanged(timeline);

            if (Proxy != null)
            {
                Proxy.Parent = timeline?.MediaPanel;
                Proxy.FPS = timeline?.FramesPerSecond;
                UpdateProxyKeyframes();
            }
        }

        /// <inheritdoc />
        public override void OnTimelineZoomChanged()
        {
            base.OnTimelineZoomChanged();

            UpdateProxyKeyframes();
        }

        /// <inheritdoc />
        public override void OnTimelineArrange()
        {
            base.OnTimelineArrange();

            UpdateProxyKeyframes();
        }

        /// <inheritdoc />
        public override void OnTimelineFpsChanged(float before, float after)
        {
            base.OnTimelineFpsChanged(before, after);

            Proxy.FPS = after;
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            if (Proxy != null)
            {
                Proxy.Dispose();
                Proxy = null;
                _proxyTracks = null;
            }

            base.OnDestroy();
        }

        /// <inheritdoc />
        public void OnKeyframesDeselect(IKeyframesEditor editor)
        {
            if (Proxy != null && Proxy.Visible)
                Proxy.OnKeyframesDeselect(editor);
        }

        /// <inheritdoc />
        public void OnKeyframesSelection(IKeyframesEditor editor, ContainerControl control, Rectangle selection)
        {
            if (Proxy != null && Proxy.Visible)
                Proxy.OnKeyframesSelection(editor, control, selection);
        }

        /// <inheritdoc />
        public int OnKeyframesSelectionCount()
        {
            return Proxy != null && Proxy.Visible ? Proxy.OnKeyframesSelectionCount() : 0;
        }

        /// <inheritdoc />
        public void OnKeyframesDelete(IKeyframesEditor editor)
        {
            if (Proxy != null && Proxy.Visible)
                Proxy.OnKeyframesDelete(editor);
        }

        /// <inheritdoc />
        public void OnKeyframesMove(IKeyframesEditor editor, ContainerControl control, Float2 location, bool start, bool end)
        {
            if (Proxy != null && Proxy.Visible)
                Proxy.OnKeyframesMove(editor, control, location, start, end);
        }

        /// <inheritdoc />
        public void OnKeyframesCopy(IKeyframesEditor editor, float? timeOffset, StringBuilder data)
        {
            if (Proxy != null && Proxy.Visible)
                Proxy.OnKeyframesCopy(editor, timeOffset, data);
        }

        /// <inheritdoc />
        public void OnKeyframesPaste(IKeyframesEditor editor, float? timeOffset, string[] datas, ref int index)
        {
            if (Proxy != null && Proxy.Visible)
                Proxy.OnKeyframesPaste(editor, timeOffset, datas, ref index);
        }

        /// <inheritdoc />
        public void OnKeyframesGet(Action<string, float, object> get)
        {
            foreach (var subTrack in SubTracks)
            {
                if (subTrack is IKeyframesEditorContext trackContext)
                    trackContext.OnKeyframesGet(get);
            }
        }

        /// <inheritdoc />
        public void OnKeyframesSet(List<KeyValuePair<float, object>> keyframes)
        {
        }
    }
}
