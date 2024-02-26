// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.Content;
using FlaxEditor.GUI.Drag;
using FlaxEditor.GUI.Timeline.Tracks;
using FlaxEditor.SceneGraph;
using FlaxEngine;

namespace FlaxEditor.GUI.Timeline
{
    /// <summary>
    /// The timeline editor for scene animation asset.
    /// </summary>
    /// <seealso cref="FlaxEditor.GUI.Timeline.Timeline" />
    public sealed class SceneAnimationTimeline : Timeline
    {
        private sealed class Proxy : ProxyBase<SceneAnimationTimeline>
        {
            /// <inheritdoc />
            public Proxy(SceneAnimationTimeline timeline)
            : base(timeline)
            {
            }
        }

        private SceneAnimationPlayer _player;
        private bool _showSelected3dTrack = true;
        internal Guid _id;

        /// <summary>
        /// Gets or sets the animation player actor used for the timeline preview.
        /// </summary>
        public SceneAnimationPlayer Player
        {
            get => _player != null ? _player : null;
            set
            {
                if (_player == value)
                    return;
                _player = value;
                UpdatePlaybackState();
                PlayerChanged?.Invoke();
            }
        }

        /// <summary>
        /// Occurs when the selected player gets changed.
        /// </summary>
        public event Action PlayerChanged;

        /// <summary>
        /// Gets or sets the selected 3D track showing option value.
        /// </summary>
        public bool ShowSelected3dTrack
        {
            get => _showSelected3dTrack;
            set
            {
                if (_showSelected3dTrack == value)
                    return;
                _showSelected3dTrack = value;
                ShowSelected3dTrackChanged?.Invoke();
            }
        }

        /// <summary>
        /// Occurs when selected 3D track showing option gets changed.
        /// </summary>
        public event Action ShowSelected3dTrackChanged;

        /// <summary>
        /// Initializes a new instance of the <see cref="SceneAnimationTimeline"/> class.
        /// </summary>
        /// <param name="undo">The undo/redo to use for the history actions recording. Optional, can be null to disable undo support.</param>
        public SceneAnimationTimeline(FlaxEditor.Undo undo)
        : base(PlaybackButtons.Play | PlaybackButtons.Stop | PlaybackButtons.Navigation, undo)
        {
            PlaybackState = PlaybackStates.Seeking;
            PropertiesEditObject = new Proxy(this);

            // Setup track types
            TrackArchetypes.Add(FolderTrack.GetArchetype());
            TrackArchetypes.Add(PostProcessMaterialTrack.GetArchetype());
            TrackArchetypes.Add(NestedSceneAnimationTrack.GetArchetype());
            TrackArchetypes.Add(ScreenFadeTrack.GetArchetype());
            TrackArchetypes.Add(AudioTrack.GetArchetype());
            TrackArchetypes.Add(AudioVolumeTrack.GetArchetype());
            TrackArchetypes.Add(ActorTrack.GetArchetype());
            TrackArchetypes.Add(ScriptTrack.GetArchetype());
            TrackArchetypes.Add(KeyframesPropertyTrack.GetArchetype());
            TrackArchetypes.Add(CurvePropertyTrack.GetArchetype());
            TrackArchetypes.Add(StringPropertyTrack.GetArchetype());
            TrackArchetypes.Add(ObjectReferencePropertyTrack.GetArchetype());
            TrackArchetypes.Add(StructPropertyTrack.GetArchetype());
            TrackArchetypes.Add(ObjectPropertyTrack.GetArchetype());
            TrackArchetypes.Add(EventTrack.GetArchetype());
            TrackArchetypes.Add(CameraCutTrack.GetArchetype());
        }

        /// <inheritdoc />
        protected override void SetupDragDrop()
        {
            base.SetupDragDrop();

            DragHandlers.Add(new DragHandler(new DragActors(IsValidActor), OnDragActor));
            DragHandlers.Add(new DragHandler(new DragScripts(IsValidScript), OnDragScript));
            DragHandlers.Add(new DragHandler(new DragAssets(IsValidAsset), OnDragAsset));
        }

        private static bool IsValidActor(ActorNode actorNode)
        {
            return actorNode.Actor;
        }

        private static void OnDragActor(Timeline timeline, DragHelper drag)
        {
            foreach (var actorNode in ((DragActors)drag).Objects)
            {
                ActorTrack track;
                if (actorNode.Actor is Camera)
                    track = (CameraCutTrack)timeline.NewTrack(CameraCutTrack.GetArchetype());
                else
                    track = (ActorTrack)timeline.NewTrack(ActorTrack.GetArchetype());
                track.Actor = actorNode.Actor;
                track.Rename(actorNode.Name);
                timeline.AddTrack(track);
            }
        }

        private static bool IsValidScript(Script script)
        {
            return script && script.Actor;
        }

        private static void OnDragScript(Timeline timeline, DragHelper drag)
        {
            foreach (var script in ((DragScripts)drag).Objects)
            {
                var actor = script.Actor;
                var track = (ActorTrack)timeline.NewTrack(ActorTrack.GetArchetype());
                track.Actor = actor;
                track.Rename(actor.Name);
                timeline.AddTrack(track);
                track.AddScriptTrack(script);
            }
        }

        private static bool IsValidAsset(AssetItem assetItem)
        {
            if (assetItem is BinaryAssetItem binaryAssetItem)
            {
                if (typeof(MaterialBase).IsAssignableFrom(binaryAssetItem.Type))
                {
                    var material = FlaxEngine.Content.LoadAsync<MaterialBase>(binaryAssetItem.ID);
                    if (material && !material.WaitForLoaded() && material.IsPostFx)
                        return true;
                }
                else if (typeof(SceneAnimation).IsAssignableFrom(binaryAssetItem.Type))
                {
                    var sceneAnimation = FlaxEngine.Content.LoadAsync<SceneAnimation>(binaryAssetItem.ID);
                    if (sceneAnimation)
                        return true;
                }
                else if (typeof(AudioClip).IsAssignableFrom(binaryAssetItem.Type))
                {
                    var audioClip = FlaxEngine.Content.LoadAsync<AudioClip>(binaryAssetItem.ID);
                    if (audioClip)
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
                    if (typeof(MaterialBase).IsAssignableFrom(binaryAssetItem.Type))
                    {
                        var material = FlaxEngine.Content.LoadAsync<MaterialBase>(binaryAssetItem.ID);
                        if (material && !material.WaitForLoaded() && material.IsPostFx)
                        {
                            var track = (PostProcessMaterialTrack)timeline.NewTrack(PostProcessMaterialTrack.GetArchetype());
                            track.Asset = material;
                            track.Rename(assetItem.ShortName);
                            timeline.AddTrack(track);
                        }
                    }
                    else if (typeof(SceneAnimation).IsAssignableFrom(binaryAssetItem.Type))
                    {
                        var sceneAnimation = FlaxEngine.Content.LoadAsync<SceneAnimation>(binaryAssetItem.ID);
                        if (!sceneAnimation || sceneAnimation.WaitForLoaded())
                            continue;
                        var track = (NestedSceneAnimationTrack)timeline.NewTrack(NestedSceneAnimationTrack.GetArchetype());
                        track.Asset = sceneAnimation;
                        track.TrackMedia.Duration = sceneAnimation.Duration;
                        track.Rename(assetItem.ShortName);
                        timeline.AddTrack(track);
                    }
                    else if (typeof(AudioClip).IsAssignableFrom(binaryAssetItem.Type))
                    {
                        var audioClip = FlaxEngine.Content.LoadAsync<AudioClip>(binaryAssetItem.ID);
                        if (!audioClip || audioClip.WaitForLoaded())
                            continue;
                        var track = (AudioTrack)timeline.NewTrack(AudioTrack.GetArchetype());
                        track.Asset = audioClip;
                        track.TrackMedia.Duration = audioClip.Length;
                        track.Rename(assetItem.ShortName);
                        timeline.AddTrack(track);
                    }
                }
            }
        }

        private void UpdatePlaybackState()
        {
            PlaybackStates state;
            if (!_player)
                state = PlaybackStates.Seeking;
            else if (_player.IsPlaying)
                state = PlaybackStates.Playing;
            else if (_player.IsPaused)
                state = PlaybackStates.Paused;
            else if (_player.IsStopped)
                state = PlaybackStates.Stopped;
            else
                state = PlaybackStates.Disabled;

            PlaybackState = state;

            if (_player && _player.Animation)
            {
                _player.Animation.WaitForLoaded();
                CurrentFrame = (int)(_player.Time * _player.Animation.FramesPerSecond);
            }
        }

        /// <inheritdoc />
        public override void OnPlay()
        {
            var time = CurrentTime;
            _player.Play();
            _player.Time = time;

            base.OnPlay();
        }

        /// <inheritdoc />
        public override void OnPause()
        {
            _player.Pause();

            base.OnPause();
        }

        /// <inheritdoc />
        public override void OnStop()
        {
            _player.Stop();

            base.OnStop();
        }

        /// <inheritdoc />
        public override void OnSeek(int frame)
        {
            if (_player != null && _player.Animation)
            {
                _player.Animation.WaitForLoaded();
                _player.Time = frame / _player.Animation.FramesPerSecond;
            }
            else
            {
                CurrentFrame = frame;
            }

            base.OnSeek(frame);
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            base.Update(deltaTime);

            UpdatePlaybackState();

            // Draw all selected 3D position tracks as Bezier curve in editor viewport
            if (ShowSelected3dTrack)
            {
                foreach (var track in SelectedTracks)
                {
                    if (
                        track is CurvePropertyTrack curveTrack &&
                        curveTrack.ValueSize == Vector3.SizeInBytes &&
                        curveTrack.MemberTypeName == typeof(Vector3).FullName &&
                        string.Equals(curveTrack.MemberName, "position", StringComparison.OrdinalIgnoreCase)
                    )
                    {
                        var curve = (BezierCurveEditor<Vector3>)curveTrack.Curve;
                        var keyframes = curve.Keyframes;
                        for (var i = 0; i < keyframes.Count; i++)
                        {
                            var k = keyframes[i];

                            DebugDraw.DrawSphere(new BoundingSphere(k.Value, 10.0f), Color.Red);
                            DebugDraw.DrawSphere(new BoundingSphere(k.Value, 9.5f), new Color(1, 0, 0, 0.1f), 0, false);

                            if (!k.TangentIn.IsZero)
                            {
                                var t = k.Value + k.TangentIn;
                                DebugDraw.DrawLine(k.Value, t, Color.Yellow);
                                DebugDraw.DrawLine(k.Value, t, Color.Yellow.AlphaMultiplied(0.1f), 0, false);
                                var box = BoundingBox.FromSphere(new BoundingSphere(t, 6.0f));
                                DebugDraw.DrawBox(box, Color.AliceBlue);
                                DebugDraw.DrawBox(box, Color.AliceBlue.AlphaMultiplied(0.1f), 0, false);
                            }

                            if (!k.TangentOut.IsZero)
                            {
                                var t = k.Value + k.TangentOut;
                                DebugDraw.DrawLine(k.Value, t, Color.Yellow);
                                DebugDraw.DrawLine(k.Value, t, Color.Yellow.AlphaMultiplied(0.1f), 0, false);
                                var box = BoundingBox.FromSphere(new BoundingSphere(t, 6.0f));
                                DebugDraw.DrawBox(box, Color.AliceBlue);
                                DebugDraw.DrawBox(box, Color.AliceBlue.AlphaMultiplied(0.1f), 0, false);
                            }

                            if (i != 0)
                            {
                                var l = keyframes[i - 1];
                                float d = (k.Time - l.Time) / 3.0f;
                                DebugDraw.DrawBezier(l.Value, l.Value + d * l.TangentOut, k.Value + d * k.TangentIn, k.Value, Color.Red);
                            }
                        }
                    }
                }
            }
        }

        /// <inheritdoc />
        protected override void OnShowViewContextMenu(ContextMenu.ContextMenu menu)
        {
            base.OnShowViewContextMenu(menu);

            menu.AddButton("Show selected 3D tracks", () => ShowSelected3dTrack = !ShowSelected3dTrack).Checked = ShowSelected3dTrack;
        }

        /// <summary>
        /// Loads the timeline from the specified <see cref="FlaxEngine.SceneAnimation"/> asset.
        /// </summary>
        /// <param name="asset">The asset.</param>
        public void Load(SceneAnimation asset)
        {
            Profiler.BeginEvent("Asset.LoadTimeline");
            var data = asset.LoadTimeline();
            Profiler.EndEvent();

            Profiler.BeginEvent("Timeline.Load");
            Load(data);
            Profiler.EndEvent();
        }

        /// <summary>
        /// Saves the timeline data to the <see cref="FlaxEngine.SceneAnimation"/> asset.
        /// </summary>
        /// <param name="asset">The asset.</param>
        public void Save(SceneAnimation asset)
        {
            var data = Save();
            asset.SaveTimeline(data);
            asset.Reload();
        }
    }
}
