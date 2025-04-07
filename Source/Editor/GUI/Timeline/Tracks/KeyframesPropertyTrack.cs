// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;
using FlaxEditor.GUI.Timeline.Undo;
using FlaxEditor.Scripting;
using FlaxEditor.Utilities;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Utilities;

namespace FlaxEditor.GUI.Timeline.Tracks
{
    /// <summary>
    /// The timeline track for animating object property via keyframes collection.
    /// </summary>
    /// <seealso cref="MemberTrack" />
    public class KeyframesPropertyTrack : MemberTrack, IKeyframesEditorContext
    {
        /// <summary>
        /// Gets the archetype.
        /// </summary>
        /// <returns>The archetype.</returns>
        public static TrackArchetype GetArchetype()
        {
            return new TrackArchetype
            {
                TypeId = 9,
                Name = "Property",
                DisableSpawnViaGUI = true,
                Create = options => new KeyframesPropertyTrack(ref options),
                Load = LoadTrack,
                Save = SaveTrack,
            };
        }

        private static void LoadTrack(int version, Track track, BinaryReader stream)
        {
            var e = (KeyframesPropertyTrack)track;

            e.ValueSize = stream.ReadInt32();
            int propertyNameLength = stream.ReadInt32();
            int propertyTypeNameLength = stream.ReadInt32();
            int keyframesCount = stream.ReadInt32();

            var propertyName = stream.ReadBytes(propertyNameLength);
            e.MemberName = Encoding.UTF8.GetString(propertyName, 0, propertyNameLength);
            if (stream.ReadChar() != 0)
                throw new Exception("Invalid track data.");

            var propertyTypeName = stream.ReadBytes(propertyTypeNameLength);
            e.MemberTypeName = Encoding.UTF8.GetString(propertyTypeName, 0, propertyTypeNameLength);
            if (stream.ReadChar() != 0)
                throw new Exception("Invalid track data.");

            var keyframes = new KeyframesEditor.Keyframe[keyframesCount];
            var propertyType = TypeUtils.GetManagedType(e.MemberTypeName);
            if (propertyType == null)
            {
                e.Keyframes.ResetKeyframes();
                stream.ReadBytes(keyframesCount * (sizeof(float) + e.ValueSize));
                if (!string.IsNullOrEmpty(e.MemberTypeName))
                    Editor.LogError("Cannot load track " + e.MemberName + " of type " + e.MemberTypeName + ". Failed to find the value type information.");
                return;
            }

            if (e.ValueSize != 0)
            {
                // POD value type - use raw memory
                var dataBuffer = new byte[e.ValueSize];
                GCHandle handle = GCHandle.Alloc(dataBuffer, GCHandleType.Pinned);
                for (int i = 0; i < keyframesCount; i++)
                {
                    var time = stream.ReadSingle();
                    stream.Read(dataBuffer, 0, e.ValueSize);
                    var value = Marshal.PtrToStructure(handle.AddrOfPinnedObject(), propertyType);

                    keyframes[i] = new KeyframesEditor.Keyframe
                    {
                        Time = time,
                        Value = value,
                    };
                }
                handle.Free();
            }
            else
            {
                // Generic value - use Json storage (as UTF-8)
                for (int i = 0; i < keyframesCount; i++)
                {
                    var time = stream.ReadSingle();
                    var len = stream.ReadInt32();
                    var value = len != 0 ? FlaxEngine.Json.JsonSerializer.Deserialize(Encoding.UTF8.GetString(stream.ReadBytes(len)), propertyType) : null;
                    keyframes[i] = new KeyframesEditor.Keyframe
                    {
                        Time = time,
                        Value = value,
                    };
                }
            }

            e.Keyframes.DefaultValue = e.GetDefaultValue(propertyType);
            e.Keyframes.SetKeyframes(keyframes);
        }

        private static void SaveTrack(Track track, BinaryWriter stream)
        {
            var e = (KeyframesPropertyTrack)track;

            var propertyName = e.MemberName ?? string.Empty;
            var propertyNameData = Encoding.UTF8.GetBytes(propertyName);
            if (propertyNameData.Length != propertyName.Length)
                throw new Exception(string.Format("The object member name bytes data has different size as UTF8 bytes. Type {0}.", propertyName));

            var propertyTypeName = e.MemberTypeName ?? string.Empty;
            var propertyTypeNameData = Encoding.UTF8.GetBytes(propertyTypeName);
            if (propertyTypeNameData.Length != propertyTypeName.Length)
                throw new Exception(string.Format("The object member typename bytes data has different size as UTF8 bytes. Type {0}.", propertyTypeName));

            var keyframes = e.Keyframes.Keyframes;

            stream.Write(e.ValueSize);
            stream.Write(propertyNameData.Length);
            stream.Write(propertyTypeNameData.Length);
            stream.Write(keyframes.Count);

            stream.Write(propertyNameData);
            stream.Write('\0');

            stream.Write(propertyTypeNameData);
            stream.Write('\0');

            if (e.ValueSize != 0)
            {
                // POD value type - use raw memory
                var dataBuffer = new byte[e.ValueSize];
                IntPtr ptr = Marshal.AllocHGlobal(e.ValueSize);
                for (int i = 0; i < keyframes.Count; i++)
                {
                    var keyframe = keyframes[i];
                    Marshal.StructureToPtr(keyframe.Value, ptr, true);
                    Marshal.Copy(ptr, dataBuffer, 0, e.ValueSize);
                    stream.Write(keyframe.Time);
                    stream.Write(dataBuffer);
                }
                Marshal.FreeHGlobal(ptr);
            }
            else
            {
                // Generic value - use Json storage (as UTF-8)
                for (int i = 0; i < keyframes.Count; i++)
                {
                    var keyframe = keyframes[i];
                    stream.Write(keyframe.Time);
                    var json = keyframe.Value != null ? FlaxEngine.Json.JsonSerializer.Serialize(keyframe.Value) : null;
                    var len = json?.Length ?? 0;
                    stream.Write(len);
                    if (len > 0)
                        stream.Write(Encoding.UTF8.GetBytes(json));
                }
            }
        }

        private byte[] _keyframesEditingStartData;

        /// <summary>
        /// The keyframes editor.
        /// </summary>
        public KeyframesEditor Keyframes;

        /// <inheritdoc />
        public KeyframesPropertyTrack(ref TrackCreateOptions options)
        : base(ref options)
        {
            Height = 20.0f;

            // Keyframes editor
            Keyframes = new KeyframesEditor
            {
                EnableZoom = false,
                EnablePanning = false,
                ScrollBars = ScrollBars.None,
            };
            Keyframes.Edited += OnKeyframesEdited;
            Keyframes.EditingStart += OnKeyframesEditingStart;
            Keyframes.EditingEnd += OnKeyframesEditingEnd;
            Keyframes.UnlockChildrenRecursive();

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
        protected override void OnContextMenu(ContextMenu.ContextMenu menu)
        {
            base.OnContextMenu(menu);

            menu.AddSeparator();
            menu.AddButton("Copy Preview Value", () =>
            {
                var value = Keyframes.Evaluate(Timeline.CurrentTime);
                Clipboard.Text = FlaxEngine.Json.JsonSerializer.Serialize(value);
            }).LinkTooltip("Copies the current track value to the clipboard").Enabled = Timeline.ShowPreviewValues;
        }

        /// <inheritdoc />
        public override bool GetNextKeyframeFrame(float time, out int result)
        {
            for (int i = 0; i < Keyframes.Keyframes.Count; i++)
            {
                var k = Keyframes.Keyframes[i];
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
                    value = Keyframes.Evaluate(time);

                // Find keyframe at the current location
                for (int i = Keyframes.Keyframes.Count - 1; i >= 0; i--)
                {
                    var k = Keyframes.Keyframes[i];
                    var frame = Mathf.FloorToInt(k.Time * Timeline.FramesPerSecond);
                    if (frame == Timeline.CurrentFrame)
                    {
                        // Skip if value is the same
                        if (Equals(k.Value, value))
                            return;

                        // Update existing key value
                        Keyframes.SetKeyframe(i, value);
                        UpdatePreviewValue();
                        return;
                    }
                }

                // Add a new key
                using (new TrackUndoBlock(this))
                    Keyframes.AddKeyframe(new KeyframesEditor.Keyframe(time, value));
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
            for (int i = Keyframes.Keyframes.Count - 1; i >= 0; i--)
            {
                var k = Keyframes.Keyframes[i];
                if (k.Time < time)
                {
                    result = Mathf.FloorToInt(k.Time * Timeline.FramesPerSecond);
                    return true;
                }
            }
            return base.GetPreviousKeyframeFrame(time, out result);
        }

        private void UpdateKeyframes()
        {
            if (Keyframes == null || Timeline == null)
                return;
            bool wasVisible = Keyframes.Visible;
            Keyframes.Visible = Visible;
            if (!Visible)
            {
                if (wasVisible)
                    Keyframes.ClearSelection();
                return;
            }
            Keyframes.KeyframesEditorContext = Timeline;
            Keyframes.CustomViewPanning = Timeline.OnKeyframesViewPanning;
            Keyframes.Bounds = new Rectangle(Timeline.StartOffset, Y + 1.0f, Timeline.Duration * Timeline.UnitsPerSecond * Timeline.Zoom, Height - 2.0f);
            Keyframes.ViewScale = new Float2(Timeline.Zoom, 1.0f);
            Keyframes.UpdateKeyframes();
        }

        private void OnKeyframesEdited()
        {
            UpdatePreviewValue();
            Timeline.MarkAsEdited();
        }

        private void OnKeyframesEditingStart()
        {
            _keyframesEditingStartData = EditTrackAction.CaptureData(this);
        }

        private void OnKeyframesEditingEnd()
        {
            var after = EditTrackAction.CaptureData(this);
            if (!FlaxEngine.Utils.ArraysEqual(_keyframesEditingStartData, after))
                Timeline.AddBatchedUndoAction(new EditTrackAction(Timeline, this, _keyframesEditingStartData, after));
            _keyframesEditingStartData = null;
        }

        private void UpdatePreviewValue()
        {
            var time = Timeline.CurrentTime;
            var value = Keyframes.Evaluate(time);
            _previewValue.Text = GetValueText(value);
        }

        /// <inheritdoc />
        public override object Evaluate(float time)
        {
            if (Keyframes != null)
                return Keyframes.Evaluate(time);
            return base.Evaluate(time);
        }

        /// <summary>
        /// Gets the default value for the given property type.
        /// </summary>
        /// <param name="propertyType">The type of the property.</param>
        /// <returns>The default value.</returns>
        protected virtual object GetDefaultValue(Type propertyType)
        {
            var value = TypeUtils.GetDefaultValue(new ScriptType(propertyType));
            if (value == null)
                value = Activator.CreateInstance(propertyType);
            return value;
        }

        /// <inheritdoc />
        protected override void OnMemberChanged(MemberInfo value, Type type)
        {
            base.OnMemberChanged(value, type);

            Keyframes.ResetKeyframes();
            if (type != null)
            {
                Keyframes.DefaultValue = GetDefaultValue(type);
            }
        }

        /// <inheritdoc />
        protected override void OnVisibleChanged()
        {
            base.OnVisibleChanged();

            UpdateKeyframes();
        }

        /// <inheritdoc />
        public override void OnTimelineChanged(Timeline timeline)
        {
            base.OnTimelineChanged(timeline);

            Keyframes.Parent = timeline?.MediaPanel;
            Keyframes.FPS = timeline?.FramesPerSecond;

            UpdateKeyframes();
            UpdatePreviewValue();
        }

        /// <inheritdoc />
        public override void OnUndo()
        {
            base.OnUndo();

            UpdatePreviewValue();
        }

        /// <inheritdoc />
        public override void OnTimelineZoomChanged()
        {
            base.OnTimelineZoomChanged();

            UpdateKeyframes();
        }

        /// <inheritdoc />
        public override void OnTimelineArrange()
        {
            base.OnTimelineArrange();

            UpdateKeyframes();
        }

        /// <inheritdoc />
        public override void OnTimelineFpsChanged(float before, float after)
        {
            base.OnTimelineFpsChanged(before, after);

            Keyframes.FPS = after;
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
            if (Keyframes != null)
            {
                Keyframes.Dispose();
                Keyframes = null;
            }

            base.OnDestroy();
        }

        /// <inheritdoc />
        public new void OnKeyframesDeselect(IKeyframesEditor editor)
        {
            if (Keyframes != null && Keyframes.Visible)
                Keyframes.OnKeyframesDeselect(editor);
        }

        /// <inheritdoc />
        public new void OnKeyframesSelection(IKeyframesEditor editor, ContainerControl control, Rectangle selection)
        {
            if (Keyframes != null && Keyframes.Visible)
                Keyframes.OnKeyframesSelection(editor, control, selection);
        }

        /// <inheritdoc />
        public new int OnKeyframesSelectionCount()
        {
            return Keyframes != null && Keyframes.Visible ? Keyframes.OnKeyframesSelectionCount() : 0;
        }

        /// <inheritdoc />
        public new void OnKeyframesDelete(IKeyframesEditor editor)
        {
            if (Keyframes != null && Keyframes.Visible)
                Keyframes.OnKeyframesDelete(editor);
        }

        /// <inheritdoc />
        public new void OnKeyframesMove(IKeyframesEditor editor, ContainerControl control, Float2 location, bool start, bool end)
        {
            if (Keyframes != null && Keyframes.Visible)
                Keyframes.OnKeyframesMove(editor, control, location, start, end);
        }

        /// <inheritdoc />
        public new void OnKeyframesCopy(IKeyframesEditor editor, float? timeOffset, StringBuilder data)
        {
            if (Keyframes != null && Keyframes.Visible)
                Keyframes.OnKeyframesCopy(editor, timeOffset, data);
        }

        /// <inheritdoc />
        public new void OnKeyframesPaste(IKeyframesEditor editor, float? timeOffset, string[] datas, ref int index)
        {
            if (Keyframes != null && Keyframes.Visible)
                Keyframes.OnKeyframesPaste(editor, timeOffset, datas, ref index);
        }

        /// <inheritdoc />
        public new void OnKeyframesGet(Action<string, float, object> get)
        {
            Keyframes?.OnKeyframesGet(Name, get);
        }

        /// <inheritdoc />
        public new void OnKeyframesSet(List<KeyValuePair<float, object>> keyframes)
        {
            Keyframes?.OnKeyframesSet(keyframes);
        }
    }
}
