// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;
using FlaxEditor.GUI.Timeline.Undo;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Utilities;

namespace FlaxEditor.GUI.Timeline.Tracks
{
    /// <summary>
    /// The timeline track for animating object property via Curve.
    /// </summary>
    /// <seealso cref="MemberTrack" />
    public abstract class CurvePropertyTrackBase : MemberTrack, IKeyframesEditorContext
    {
        private byte[] _curveEditingStartData;
        private float _expandedHeight = 120.0f;
        private Splitter _splitter;

        /// <summary>
        /// The curve editor.
        /// </summary>
        public CurveEditorBase Curve;

        private const float CollapsedHeight = 20.0f;

        /// <inheritdoc />
        public CurvePropertyTrackBase(ref TrackCreateOptions options)
        : base(ref options)
        {
            Height = CollapsedHeight;

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

            if (Curve == null)
                return;
            menu.AddSeparator();
            menu.AddButton("Copy Preview Value", () =>
            {
                Curve.Evaluate(out var value, Timeline.CurrentTime);
                Clipboard.Text = FlaxEngine.Json.JsonSerializer.Serialize(value);
            }).LinkTooltip("Copies the current track value to the clipboard").Enabled = Timeline.ShowPreviewValues;
        }

        /// <inheritdoc />
        public override bool GetNextKeyframeFrame(float time, out int result)
        {
            if (Curve != null)
            {
                for (int i = 0; i < Curve.KeyframesCount; i++)
                {
                    Curve.GetKeyframe(i, out var kTime, out _, out _, out _);
                    if (kTime > time)
                    {
                        result = Mathf.FloorToInt(kTime * Timeline.FramesPerSecond);
                        return true;
                    }
                }
            }
            return base.GetNextKeyframeFrame(time, out result);
        }

        private void OnAddKeyClicked(Image image, MouseButton button)
        {
            if (button == MouseButton.Left && Curve != null)
            {
                // Evaluate a value
                var time = Timeline.CurrentTime;
                if (!TryGetValue(out var value))
                    Curve.Evaluate(out value, time);

                // Find keyframe at the current location
                for (int i = Curve.KeyframesCount - 1; i >= 0; i--)
                {
                    Curve.GetKeyframe(i, out var kTime, out var kValue, out _, out _);
                    var frame = Mathf.FloorToInt(kTime * Timeline.FramesPerSecond);
                    if (frame == Timeline.CurrentFrame)
                    {
                        // Skip if value is the same
                        if (Equals(kValue, value))
                            return;

                        // Update existing key value
                        Curve.SetKeyframeValue(i, value);
                        UpdatePreviewValue();
                        return;
                    }
                }

                // Add a new key
                using (new TrackUndoBlock(this))
                    Curve.AddKeyframe(time, value);
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
            if (Curve != null)
            {
                for (int i = Curve.KeyframesCount - 1; i >= 0; i--)
                {
                    Curve.GetKeyframe(i, out var kTime, out _, out _, out _);
                    if (kTime < time)
                    {
                        result = Mathf.FloorToInt(kTime * Timeline.FramesPerSecond);
                        return true;
                    }
                }
            }
            return base.GetPreviousKeyframeFrame(time, out result);
        }

        private void UpdateCurve()
        {
            if (Curve == null || Timeline == null)
                return;
            bool wasVisible = Curve.Visible;
            Curve.Visible = Visible;
            if (!Visible)
            {
                if (wasVisible)
                    Curve.ClearSelection();
                return;
            }
            var expanded = IsExpanded;
            Curve.KeyframesEditorContext = Timeline;
            Curve.CustomViewPanning = Timeline.OnKeyframesViewPanning;
            Curve.Bounds = new Rectangle(Timeline.StartOffset, Y + 1.0f, Timeline.Duration * Timeline.UnitsPerSecond * Timeline.Zoom, Height - 2.0f);
            Curve.ViewScale = new Float2(Timeline.Zoom, Curve.ViewScale.Y);
            Curve.ShowCollapsed = !expanded;
            Curve.ShowAxes = expanded ? CurveEditorBase.UseMode.Horizontal : CurveEditorBase.UseMode.Off;
            Curve.EnableZoom = expanded ? CurveEditorBase.UseMode.Vertical : CurveEditorBase.UseMode.Off;
            Curve.EnablePanning = expanded ? CurveEditorBase.UseMode.Vertical : CurveEditorBase.UseMode.Off;
            Curve.ScrollBars = expanded ? ScrollBars.Vertical : ScrollBars.None;
            Curve.UpdateKeyframes();
            if (expanded)
            {
                if (_splitter == null)
                {
                    _splitter = new Splitter
                    {
                        Moved = OnSplitterMoved,
                        Parent = Curve,
                    };
                }
                _splitter.Bounds = new Rectangle(0, Curve.Height - Splitter.DefaultHeight, Curve.Width, Splitter.DefaultHeight);
            }
        }

        private void OnSplitterMoved(Float2 location)
        {
            var height = Mathf.Clamp(PointToParent(location).Y, 40.0f, 1000.0f);
            if (!Mathf.NearEqual(height, _expandedHeight))
            {
                Height = _expandedHeight = height;
                Timeline.ArrangeTracks();
            }
        }

        private void OnKeyframesEdited()
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
                Timeline.AddBatchedUndoAction(new EditTrackAction(Timeline, this, _curveEditingStartData, after));
            _curveEditingStartData = null;
        }

        private void UpdatePreviewValue()
        {
            if (Curve == null)
            {
                _previewValue.Text = string.Empty;
                return;
            }

            var time = Timeline.CurrentTime;
            Curve.Evaluate(out var value, time);
            _previewValue.Text = GetValueText(value);
        }

        /// <summary>
        /// Creates the curve.
        /// </summary>
        /// <param name="propertyType">Type of the property (keyframes value).</param>
        /// <param name="curveEditorType">Type of the curve editor (generic type of the curve editor).</param>
        protected void CreateCurve(Type propertyType, Type curveEditorType)
        {
            curveEditorType = curveEditorType.MakeGenericType(propertyType);
            Curve = (CurveEditorBase)Activator.CreateInstance(curveEditorType);
            Curve.EnableZoom = CurveEditorBase.UseMode.Vertical;
            Curve.EnablePanning = CurveEditorBase.UseMode.Vertical;
            Curve.ShowBackground = false;
            Curve.ScrollBars = ScrollBars.Vertical;
            Curve.Parent = Timeline?.MediaPanel;
            Curve.FPS = Timeline?.FramesPerSecond;
            Curve.Edited += OnKeyframesEdited;
            Curve.EditingStart += OnCurveEditingStart;
            Curve.EditingEnd += OnCurveEditingEnd;
            if (Timeline != null)
            {
                Curve.UnlockChildrenRecursive();
                UpdateCurve();
            }
        }

        private void DisposeCurve()
        {
            if (Curve == null)
                return;
            Curve.Edited -= OnKeyframesEdited;
            Curve.Dispose();
            Curve = null;
            _splitter = null;
        }

        /// <inheritdoc />
        public override object Evaluate(float time)
        {
            if (Curve != null)
            {
                Curve.Evaluate(out var result, time);
                return result;
            }

            return base.Evaluate(time);
        }

        /// <inheritdoc />
        public override bool CanExpand => true;

        /// <inheritdoc />
        protected override void OnMemberChanged(MemberInfo value, Type type)
        {
            base.OnMemberChanged(value, type);

            DisposeCurve();
        }

        /// <inheritdoc />
        protected override void OnVisibleChanged()
        {
            base.OnVisibleChanged();

            UpdateCurve();
        }

        /// <inheritdoc />
        protected override void OnExpandedChanged()
        {
            Height = IsExpanded ? _expandedHeight : CollapsedHeight;
            UpdateCurve();
            if (IsExpanded)
                Curve.ShowWholeCurve();

            base.OnExpandedChanged();
        }

        /// <inheritdoc />
        public override void OnTimelineChanged(Timeline timeline)
        {
            base.OnTimelineChanged(timeline);

            if (Curve != null)
            {
                Curve.Parent = timeline?.MediaPanel;
                Curve.FPS = timeline?.FramesPerSecond;
                UpdateCurve();
            }
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

            if (Curve != null)
            {
                Curve.FPS = after;
            }
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
            DisposeCurve();

            base.OnDestroy();
        }

        /// <inheritdoc />
        public new void OnKeyframesDeselect(IKeyframesEditor editor)
        {
            if (Curve != null && Curve.Visible)
                Curve.OnKeyframesDeselect(editor);
        }

        /// <inheritdoc />
        public new void OnKeyframesSelection(IKeyframesEditor editor, ContainerControl control, Rectangle selection)
        {
            if (Curve != null && Curve.Visible)
                Curve.OnKeyframesSelection(editor, control, selection);
        }

        /// <inheritdoc />
        public new int OnKeyframesSelectionCount()
        {
            return Curve != null && Curve.Visible ? Curve.OnKeyframesSelectionCount() : 0;
        }

        /// <inheritdoc />
        public new void OnKeyframesDelete(IKeyframesEditor editor)
        {
            if (Curve != null && Curve.Visible)
                Curve.OnKeyframesDelete(editor);
        }

        /// <inheritdoc />
        public new void OnKeyframesMove(IKeyframesEditor editor, ContainerControl control, Float2 location, bool start, bool end)
        {
            if (Curve != null && Curve.Visible)
                Curve.OnKeyframesMove(editor, control, location, start, end);
        }

        /// <inheritdoc />
        public new void OnKeyframesCopy(IKeyframesEditor editor, float? timeOffset, StringBuilder data)
        {
            if (Curve != null && Curve.Visible)
                Curve.OnKeyframesCopy(editor, timeOffset, data);
        }

        /// <inheritdoc />
        public new void OnKeyframesPaste(IKeyframesEditor editor, float? timeOffset, string[] datas, ref int index)
        {
            if (Curve != null && Curve.Visible)
                Curve.OnKeyframesPaste(editor, timeOffset, datas, ref index);
        }

        /// <inheritdoc />
        public new void OnKeyframesGet(Action<string, float, object> get)
        {
            Curve?.OnKeyframesGet(Name, get);
        }

        /// <inheritdoc />
        public new void OnKeyframesSet(List<KeyValuePair<float, object>> keyframes)
        {
            Curve?.OnKeyframesSet(keyframes);
        }
    }

    /// <summary>
    /// The timeline track for animating object property via Bezier Curve.
    /// </summary>
    /// <seealso cref="MemberTrack" />
    /// <seealso cref="CurvePropertyTrackBase" />
    public sealed class CurvePropertyTrack : CurvePropertyTrackBase
    {
        /// <summary>
        /// Gets the archetype.
        /// </summary>
        /// <returns>The archetype.</returns>
        public static TrackArchetype GetArchetype()
        {
            return new TrackArchetype
            {
                TypeId = 10,
                Name = "Property",
                DisableSpawnViaGUI = true,
                Create = options => new CurvePropertyTrack(ref options),
                Load = LoadTrack,
                Save = SaveTrack,
            };
        }

        private static void LoadTrack(int version, Track track, BinaryReader stream)
        {
            var e = (CurvePropertyTrack)track;

            int valueSize = stream.ReadInt32();
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

            var keyframes = new object[keyframesCount];
            var propertyType = TypeUtils.GetType(e.MemberTypeName).Type;
            if (propertyType == null)
            {
                stream.ReadBytes(keyframesCount * (sizeof(float) + valueSize * 3));
                if (!string.IsNullOrEmpty(e.MemberTypeName))
                    Editor.LogError("Cannot load track " + e.MemberName + " of type " + e.MemberTypeName + ". Failed to find the value type information.");
                return;
            }

            e.ValueSize = e.GetValueDataSize(propertyType);
            var dataBuffer = new byte[valueSize];
            GCHandle handle = GCHandle.Alloc(dataBuffer, GCHandleType.Pinned);
            for (int i = 0; i < keyframesCount; i++)
            {
                var time = stream.ReadSingle();
                var value = ReadValue(stream, ref handle, dataBuffer, e.ValueSize, propertyType);
                var tangentIn = ReadValue(stream, ref handle, dataBuffer, e.ValueSize, propertyType);
                var tangentOut = ReadValue(stream, ref handle, dataBuffer, e.ValueSize, propertyType);

                keyframes[i] = new BezierCurve<object>.Keyframe
                {
                    Time = time,
                    Value = value,
                    TangentIn = tangentIn,
                    TangentOut = tangentOut,
                };
            }
            handle.Free();

            if (e.Curve != null && e.Curve.ValueType != propertyType)
            {
                e.Curve.Dispose();
                e.Curve = null;
            }
            if (e.Curve == null)
            {
                e.CreateCurve(propertyType, typeof(BezierCurveEditor<>));
            }
            e.Curve.SetKeyframes(keyframes);
        }

        private static object ReadValue(BinaryReader stream, ref GCHandle handle, byte[] dataBuffer, int valueSize, Type propertyType)
        {
            stream.Read(dataBuffer, 0, dataBuffer.Length);
            if (valueSize != dataBuffer.Length)
            {
                // Convert curve data into the runtime type (eg. when using animation saved with Vector3=Double3 and playing it in a build with Vector3=Float3)
                if (propertyType == typeof(Vector2))
                {
                    if (dataBuffer.Length == Double2.SizeInBytes)
                        return (Vector2)(Double2)Marshal.PtrToStructure(handle.AddrOfPinnedObject(), typeof(Double2));
                    return (Vector2)(Float2)Marshal.PtrToStructure(handle.AddrOfPinnedObject(), typeof(Float2));
                }
                if (propertyType == typeof(Vector3))
                {
                    if (dataBuffer.Length == Double3.SizeInBytes)
                        return (Vector3)(Double3)Marshal.PtrToStructure(handle.AddrOfPinnedObject(), typeof(Double3));
                    return (Vector3)(Float3)Marshal.PtrToStructure(handle.AddrOfPinnedObject(), typeof(Float3));
                }
                if (propertyType == typeof(Vector4))
                {
                    if (dataBuffer.Length == Double4.SizeInBytes)
                        return (Vector4)(Double4)Marshal.PtrToStructure(handle.AddrOfPinnedObject(), typeof(Double4));
                    return (Vector4)(Float4)Marshal.PtrToStructure(handle.AddrOfPinnedObject(), typeof(Float4));
                }
            }
            return Marshal.PtrToStructure(handle.AddrOfPinnedObject(), propertyType);
        }

        private static void SaveTrack(Track track, BinaryWriter stream)
        {
            var e = (CurvePropertyTrack)track;

            var propertyName = e.MemberName ?? string.Empty;
            var propertyNameData = Encoding.UTF8.GetBytes(propertyName);
            if (propertyNameData.Length != propertyName.Length)
                throw new Exception(string.Format("The object member name bytes data has different size as UTF8 bytes. Type {0}.", propertyName));

            var propertyTypeName = e.MemberTypeName ?? string.Empty;
            var propertyTypeNameData = Encoding.UTF8.GetBytes(propertyTypeName);
            if (propertyTypeNameData.Length != propertyTypeName.Length)
                throw new Exception(string.Format("The object member typename bytes data has different size as UTF8 bytes. Type {0}.", propertyTypeName));

            var keyframesCount = e.Curve?.KeyframesCount ?? 0;

            stream.Write(e.ValueSize);
            stream.Write(propertyNameData.Length);
            stream.Write(propertyTypeNameData.Length);
            stream.Write(keyframesCount);

            stream.Write(propertyNameData);
            stream.Write('\0');

            stream.Write(propertyTypeNameData);
            stream.Write('\0');

            if (keyframesCount == 0)
                return;
            var dataBuffer = new byte[e.ValueSize];
            IntPtr ptr = Marshal.AllocHGlobal(e.ValueSize);
            for (int i = 0; i < keyframesCount; i++)
            {
                e.Curve.GetKeyframe(i, out var time, out var value, out var tangentIn, out var tangentOut);
                stream.Write(time);

                Marshal.StructureToPtr(value, ptr, true);
                Marshal.Copy(ptr, dataBuffer, 0, e.ValueSize);
                stream.Write(dataBuffer);

                Marshal.StructureToPtr(tangentIn, ptr, true);
                Marshal.Copy(ptr, dataBuffer, 0, e.ValueSize);
                stream.Write(dataBuffer);

                Marshal.StructureToPtr(tangentOut, ptr, true);
                Marshal.Copy(ptr, dataBuffer, 0, e.ValueSize);
                stream.Write(dataBuffer);
            }
            Marshal.FreeHGlobal(ptr);
        }

        /// <inheritdoc />
        public CurvePropertyTrack(ref TrackCreateOptions options)
        : base(ref options)
        {
        }

        /// <inheritdoc />
        protected override void OnMemberChanged(MemberInfo value, Type type)
        {
            base.OnMemberChanged(value, type);

            if (type != null)
            {
                CreateCurve(type, typeof(BezierCurveEditor<>));
            }
        }
    }
}
