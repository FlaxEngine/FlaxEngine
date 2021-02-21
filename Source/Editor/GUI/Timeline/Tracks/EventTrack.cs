// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;
using FlaxEditor.GUI.Timeline.Undo;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI.Timeline.Tracks
{
    /// <summary>
    /// The timeline track for invoking events on a certain points in the time.
    /// </summary>
    /// <seealso cref="MemberTrack" />
    public class EventTrack : MemberTrack
    {
        /// <summary>
        /// Gets the archetype.
        /// </summary>
        /// <returns>The archetype.</returns>
        public static TrackArchetype GetArchetype()
        {
            return new TrackArchetype
            {
                TypeId = 15,
                Name = "Event",
                DisableSpawnViaGUI = true,
                Create = options => new EventTrack(ref options),
                Load = LoadTrack,
                Save = SaveTrack,
            };
        }

        private static void LoadTrack(int version, Track track, BinaryReader stream)
        {
            var e = (EventTrack)track;

            var paramsCount = stream.ReadInt32();
            e.EventParamsSizes = new int[paramsCount];
            e.EventParamsTypes = new Type[paramsCount];
            int eventsCount = stream.ReadInt32();
            e.MemberName = LoadName(stream);

            bool isInvalid = false;
            for (int i = 0; i < paramsCount; i++)
            {
                e.EventParamsSizes[i] = stream.ReadInt32();
                var paramTypeName = LoadName(stream);
                e.EventParamsTypes[i] = Scripting.TypeUtils.GetType(paramTypeName).Type;
                if (e.EventParamsTypes[i] == null)
                    isInvalid = true;
            }

            if (isInvalid)
            {
                e.Events.ResetKeyframes();
                stream.ReadBytes(eventsCount * (sizeof(float) + e.EventParamsSizes.Sum()));
                Editor.LogError("Cannot load track " + e.MemberName + " of type " + e.MemberTypeName + ". Failed to find the value type information.");
                return;
            }

            e.OnEventParamsChanged();

            var events = new KeyframesEditor.Keyframe[eventsCount];
            var dataBuffer = paramsCount != 0 ? new byte[e.EventParamsSizes.Max()] : null;
            GCHandle handle = paramsCount != 0 ? GCHandle.Alloc(dataBuffer, GCHandleType.Pinned) : new GCHandle();
            for (int i = 0; i < eventsCount; i++)
            {
                var time = stream.ReadSingle();

                var key = new EventKey
                {
                    Parameters = new object[paramsCount]
                };

                for (int j = 0; j < paramsCount; j++)
                {
                    stream.Read(dataBuffer, 0, e.EventParamsSizes[j]);
                    key.Parameters[j] = Marshal.PtrToStructure(handle.AddrOfPinnedObject(), e.EventParamsTypes[j]);
                }

                events[i] = new KeyframesEditor.Keyframe
                {
                    Time = time,
                    Value = key,
                };
            }
            if (handle.IsAllocated)
                handle.Free();

            e.Events.SetKeyframes(events);
        }

        private static void SaveTrack(Track track, BinaryWriter stream)
        {
            var e = (EventTrack)track;

            var events = e.Events.Keyframes;
            var paramsCount = e.EventParamsTypes.Length;

            stream.Write(paramsCount);
            stream.Write(events.Count);
            SaveName(stream, e.MemberName);

            for (int i = 0; i < e.EventParamsTypes.Length; i++)
            {
                stream.Write(e.EventParamsSizes[i]);
                SaveName(stream, e.EventParamsTypes[i]?.FullName);
            }

            var dataBuffer = paramsCount != 0 ? new byte[e.EventParamsSizes.Max()] : null;
            IntPtr ptr = paramsCount != 0 ? Marshal.AllocHGlobal(dataBuffer.Length) : IntPtr.Zero;
            for (int i = 0; i < events.Count; i++)
            {
                var keyframe = events[i];
                var key = (EventKey)keyframe.Value;

                stream.Write(keyframe.Time);

                for (int j = 0; j < paramsCount; j++)
                {
                    Marshal.StructureToPtr(key.Parameters[j], ptr, true);
                    Marshal.Copy(ptr, dataBuffer, 0, e.EventParamsSizes[j]);
                    stream.Write(dataBuffer, 0, e.EventParamsSizes[j]);
                }
            }
            Marshal.FreeHGlobal(ptr);
        }

        private byte[] _eventsEditingStartData;

        /// <summary>
        /// The event keyframes editor.
        /// </summary>
        public KeyframesEditor Events;

        /// <summary>
        /// The event parameters data sizes collection.
        /// </summary>
        public int[] EventParamsSizes = Utils.GetEmptyArray<int>();

        /// <summary>
        /// The event parameters types collection.
        /// </summary>
        public Type[] EventParamsTypes = Utils.GetEmptyArray<Type>();

        /// <summary>
        /// The event key data.
        /// </summary>
        public struct EventKey
        {
            /// <summary>
            /// The parameters values.
            /// </summary>
            [EditorDisplay("Parameters", EditorDisplayAttribute.InlineStyle), ExpandGroups]
            [Collection(CanReorderItems = false, ReadOnly = true)]
            public object[] Parameters;

            /// <inheritdoc />
            public override string ToString()
            {
                if (Parameters == null || Parameters.Length == 0)
                    return "()";
                var sb = new StringBuilder();
                sb.Append('(');
                for (int i = 0; i < Parameters.Length; i++)
                {
                    if (i != 0)
                        sb.Append(", ");
                    sb.Append(Parameters[i] ?? "<null>");
                }
                sb.Append(')');
                return sb.ToString();
            }
        }

        /// <inheritdoc />
        public EventTrack(ref TrackCreateOptions options)
        : base(ref options, true, false)
        {
            Height = 20.0f;

            // Events editor
            Events = new KeyframesEditor
            {
                EnableZoom = false,
                EnablePanning = false,
                ScrollBars = ScrollBars.None,
            };
            Events.Edited += OnEventsEdited;
            Events.EditingStart += OnEventsEditingStart;
            Events.EditingEnd += OnEventsEditingEnd;
            Events.UnlockChildrenRecursive();

            _addKey.Clicked += OnAddKeyClicked;
            _leftKey.Clicked += OnLeftKeyClicked;
            _rightKey.Clicked += OnRightKeyClicked;
        }

        private void OnRightKeyClicked(Image image, MouseButton button)
        {
            if (button == MouseButton.Left && GetNextKeyframeFrame(Timeline.CurrentTime, out var frame))
            {
                Timeline.OnSeek(frame);
            }
        }

        /// <inheritdoc />
        public override bool GetNextKeyframeFrame(float time, out int result)
        {
            for (int i = 0; i < Events.Keyframes.Count; i++)
            {
                var k = Events.Keyframes[i];
                if (k.Time > time)
                {
                    result = Mathf.FloorToInt(k.Time * Timeline.FramesPerSecond);
                    return true;
                }
            }
            return base.GetNextKeyframeFrame(time, out result);
        }

        private void OnAddKeyClicked(Image image, MouseButton button)
        {
            if (button == MouseButton.Left)
            {
                // Evaluate a value
                var time = Timeline.CurrentTime;
                if (!TryGetValue(out var value))
                    value = Events.Evaluate(time);

                // Find event at the current location
                for (int i = Events.Keyframes.Count - 1; i >= 0; i--)
                {
                    var k = Events.Keyframes[i];
                    var frame = Mathf.FloorToInt(k.Time * Timeline.FramesPerSecond);
                    if (frame == Timeline.CurrentFrame)
                    {
                        // Skip if value is the same
                        if (k.Value == value)
                            return;

                        // Update existing event value
                        Events.SetKeyframe(i, value);
                        return;
                    }
                }

                // Add a new key
                using (new TrackUndoBlock(this))
                    Events.AddKeyframe(new KeyframesEditor.Keyframe(time, value));
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
            for (int i = Events.Keyframes.Count - 1; i >= 0; i--)
            {
                var k = Events.Keyframes[i];
                if (k.Time < time)
                {
                    result = Mathf.FloorToInt(k.Time * Timeline.FramesPerSecond);
                    return true;
                }
            }
            return base.GetPreviousKeyframeFrame(time, out result);
        }

        private void UpdateEvents()
        {
            if (Events == null)
                return;

            Events.Bounds = new Rectangle(Timeline.StartOffset, Y + 1.0f, Timeline.Duration * Timeline.UnitsPerSecond * Timeline.Zoom, Height - 2.0f);
            Events.ViewScale = new Vector2(Timeline.Zoom, 1.0f);
            Events.Visible = Visible;
            Events.UpdateKeyframes();
        }

        private void OnEventsEdited()
        {
            Timeline.MarkAsEdited();
        }

        private void OnEventsEditingStart()
        {
            _eventsEditingStartData = EditTrackAction.CaptureData(this);
        }

        private void OnEventsEditingEnd()
        {
            var after = EditTrackAction.CaptureData(this);
            if (!Utils.ArraysEqual(_eventsEditingStartData, after))
                Timeline.Undo.AddAction(new EditTrackAction(Timeline, this, _eventsEditingStartData, after));
            _eventsEditingStartData = null;
        }

        private void OnEventParamsChanged()
        {
            // Calculate data size
            EventParamsSizes = new int[EventParamsTypes.Length];
            for (int i = 0; i < EventParamsTypes.Length; i++)
            {
                var type = EventParamsTypes[i];
                EventParamsSizes[i] = Marshal.SizeOf(type.IsEnum ? Enum.GetUnderlyingType(type) : type);
            }

            // Build default value
            var defaultValue = new EventKey
            {
                Parameters = new object[EventParamsTypes.Length]
            };

            for (int i = 0; i < EventParamsTypes.Length; i++)
                defaultValue.Parameters[i] = Activator.CreateInstance(EventParamsTypes[i]);
            Events.DefaultValue = defaultValue;
        }

        /// <inheritdoc />
        protected override MemberTypes MemberTypes => MemberTypes.Method;

        /// <inheritdoc />
        protected override void OnMemberChanged(MemberInfo value, Type type)
        {
            base.OnMemberChanged(value, type);

            Events.ResetKeyframes();

            if (value is MethodInfo m)
            {
                EventParamsTypes = m.GetParameters().Select(x => x.ParameterType).ToArray();
            }
            else
            {
                EventParamsTypes = Utils.GetEmptyArray<Type>();
            }
            OnEventParamsChanged();
        }

        /// <inheritdoc />
        protected override void OnVisibleChanged()
        {
            base.OnVisibleChanged();

            Events.Visible = Visible;
        }

        /// <inheritdoc />
        public override void OnTimelineChanged(Timeline timeline)
        {
            if (Timeline != null)
            {
                Timeline.ShowPreviewValuesChanged -= OnTimelineShowPreviewValuesChanged;
            }

            base.OnTimelineChanged(timeline);

            if (Timeline != null)
            {
                if (_previewValue != null)
                    _previewValue.Visible = Timeline.ShowPreviewValues;
                Timeline.ShowPreviewValuesChanged += OnTimelineShowPreviewValuesChanged;
            }

            Events.Parent = timeline?.MediaPanel;
            Events.FPS = timeline?.FramesPerSecond;

            UpdateEvents();
        }

        private void OnTimelineShowPreviewValuesChanged()
        {
            if (_previewValue != null)
                _previewValue.Visible = Timeline.ShowPreviewValues;
        }

        /// <inheritdoc />
        public override void OnTimelineZoomChanged()
        {
            base.OnTimelineZoomChanged();

            UpdateEvents();
        }

        /// <inheritdoc />
        public override void OnTimelineArrange()
        {
            base.OnTimelineArrange();

            UpdateEvents();
        }

        /// <inheritdoc />
        public override void OnTimelineFpsChanged(float before, float after)
        {
            base.OnTimelineFpsChanged(before, after);

            Events.FPS = after;
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            if (Events != null)
            {
                Events.Dispose();
                Events = null;
            }

            base.OnDestroy();
        }
    }
}
