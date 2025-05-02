// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using FlaxEditor.Utilities;
using FlaxEngine;
using FlaxEngine.GUI;
using Object = FlaxEngine.Object;

namespace FlaxEditor.GUI.Timeline.Tracks
{
    /// <summary>
    /// The timeline media that represents an camera cut media event.
    /// </summary>
    /// <seealso cref="FlaxEditor.GUI.Timeline.Media" />
    public class CameraCutMedia : Media
    {
        private sealed class Proxy : ProxyBase<CameraCutTrack, CameraCutMedia>
        {
            public Proxy(CameraCutTrack track, CameraCutMedia media)
            : base(track, media)
            {
            }
        }

        private Image[] _thumbnails = new Image[3];

        /// <summary>
        /// Initializes a new instance of the <see cref="CameraCutMedia"/> class.
        /// </summary>
        public CameraCutMedia()
        {
            ClipChildren = true;
            CanSplit = true;
            CanDelete = true;
        }

        /// <summary>
        /// Updates the thumbnails.
        /// </summary>
        /// <param name="indices">The icon indices to update (null if update all of them).</param>
        public void UpdateThumbnails(int[] indices = null)
        {
            if (Timeline == null)
                return;

            if (((CameraCutTrack)Track).Camera)
            {
                if (Timeline.CameraCutThumbnailRenderer == null)
                    Timeline.CameraCutThumbnailRenderer = new CameraCutThumbnailRenderer();

                if (indices == null)
                {
                    for (int i = 0; i < _thumbnails.Length; i++)
                        Timeline.CameraCutThumbnailRenderer.AddRequest(new CameraCutThumbnailRenderer.Request(this, i));
                }
                else
                {
                    foreach (var i in indices)
                        Timeline.CameraCutThumbnailRenderer.AddRequest(new CameraCutThumbnailRenderer.Request(this, i));
                }
            }
            else if (Timeline.CameraCutThumbnailRenderer != null)
            {
                if (indices == null)
                {
                    for (int i = 0; i < _thumbnails.Length; i++)
                    {
                        var image = _thumbnails[i];
                        if (image?.Brush != null)
                        {
                            Timeline.CameraCutThumbnailRenderer.ReleaseThumbnail(((SpriteBrush)image.Brush).Sprite);
                            image.Brush = null;
                        }
                    }
                }
                else
                {
                    foreach (var i in indices)
                    {
                        var image = _thumbnails[i];
                        if (image?.Brush != null)
                        {
                            Timeline.CameraCutThumbnailRenderer.ReleaseThumbnail(((SpriteBrush)image.Brush).Sprite);
                            image.Brush = null;
                        }
                    }
                }
            }
        }

        /// <summary>
        /// Called when thumbnail rendering begins.
        /// </summary>
        /// <param name="task">The scene rendering task to customize.</param>
        /// <param name="context">The GPU rendering context.</param>
        /// <param name="req">The request data.</param>
        public void OnThumbnailRenderingBegin(SceneRenderTask task, GPUContext context, ref CameraCutThumbnailRenderer.Request req)
        {
            RenderView view = new RenderView();
            var track = (CameraCutTrack)Track;
            Camera cam = track.Camera;
            var viewport = new FlaxEngine.Viewport(Float2.Zero, task.Buffers.Size);
            Quaternion orientation = Quaternion.Identity;
            view.Near = 10.0f;
            view.Far = 20000.0f;
            bool usePerspective = true;
            float orthoScale = 1.0f;
            float fov = 60.0f;
            float customAspectRatio = 0.0f;
            view.RenderLayersMask = new LayersMask(uint.MaxValue);

            // Try to evaluate camera properties based on the initial camera state
            if (cam)
            {
                view.Position = cam.Position;
                orientation = cam.Orientation;
                view.Near = cam.NearPlane;
                view.Far = cam.FarPlane;
                usePerspective = cam.UsePerspective;
                orthoScale = cam.OrthographicScale;
                fov = cam.FieldOfView;
                customAspectRatio = cam.CustomAspectRatio;
                view.RenderLayersMask = cam.RenderLayersMask;
                view.Flags = cam.RenderFlags;
                view.Mode = cam.RenderMode;
            }

            // Try to evaluate camera properties based on the animated tracks
            float time = Start;
            if (req.ThumbnailIndex == 1)
                time += Duration;
            else if (req.ThumbnailIndex == 2)
                time += Duration * 0.5f;
            foreach (var subTrack in track.SubTracks)
            {
                if (subTrack is MemberTrack memberTrack)
                {
                    object value = memberTrack.Evaluate(time);
                    if (value != null)
                    {
                        // TODO: try to make it better
                        if (memberTrack.MemberName == "Position" && value is Vector3 asPosition)
                            view.Position = asPosition;

                        else if (memberTrack.MemberName == "Orientation" && value is Quaternion asRotation)
                            orientation = asRotation;

                        else if (memberTrack.MemberName == "NearPlane" && value is float asNearPlane)
                            view.Near = asNearPlane;

                        else if (memberTrack.MemberName == "FarPlane" && value is float asFarPlane)
                            view.Far = asFarPlane;

                        else if (memberTrack.MemberName == "UsePerspective" && value is bool asUsePerspective)
                            usePerspective = asUsePerspective;

                        else if (memberTrack.MemberName == "FieldOfView" && value is float asFieldOfView)
                            fov = asFieldOfView;

                        else if (memberTrack.MemberName == "CustomAspectRatio" && value is float asCustomAspectRatio)
                            customAspectRatio = asCustomAspectRatio;

                        else if (memberTrack.MemberName == "OrthographicScale" && value is float asOrthographicScale)
                            orthoScale = asOrthographicScale;
                    }
                }
            }

            // Build view
            view.Direction = Float3.Forward * orientation;
            if (usePerspective)
            {
                float aspect = customAspectRatio <= 0.0f ? viewport.AspectRatio : customAspectRatio;
                view.Projection = Matrix.PerspectiveFov(fov * Mathf.DegreesToRadians, aspect, view.Near, view.Far);
            }
            else
            {
                view.Projection = Matrix.Ortho(viewport.Width * orthoScale, viewport.Height * orthoScale, view.Near, view.Far);
            }

            Vector3 target = view.Position + view.Direction;
            var up = Float3.Transform(Float3.Up, orientation);
            view.View = Matrix.LookAt(view.Position, target, up);
            view.NonJitteredProjection = view.Projection;
            view.TemporalAAJitter = Float4.Zero;
            view.ModelLODDistanceFactor = 100.0f;
            view.Flags = ViewFlags.DefaultGame & ~(ViewFlags.MotionBlur);
            view.UpdateCachedData();
            task.View = view;
        }

        /// <summary>
        /// Called when thumbnail rendering ends. The task output buffer contains ready frame.
        /// </summary>
        /// <param name="task">The scene rendering task to customize.</param>
        /// <param name="context">The GPU rendering context.</param>
        /// <param name="req">The request data.</param>
        /// <param name="sprite">The thumbnail sprite.</param>
        public void OnThumbnailRenderingEnd(SceneRenderTask task, GPUContext context, ref CameraCutThumbnailRenderer.Request req, ref SpriteHandle sprite)
        {
            var image = _thumbnails[req.ThumbnailIndex];
            if (image == null)
            {
                if (req.ThumbnailIndex == 0)
                {
                    image = new Image
                    {
                        AnchorPreset = AnchorPresets.MiddleLeft,
                        Parent = this,
                        Bounds = new Rectangle(2, 2, CameraCutThumbnailRenderer.Width, CameraCutThumbnailRenderer.Height),
                    };
                }
                else if (req.ThumbnailIndex == 1)
                {
                    image = new Image
                    {
                        AnchorPreset = AnchorPresets.MiddleRight,
                        Parent = this,
                        Bounds = new Rectangle(Width - 2 - CameraCutThumbnailRenderer.Width, 2, CameraCutThumbnailRenderer.Width, CameraCutThumbnailRenderer.Height),
                    };
                }
                else
                {
                    image = new Image
                    {
                        AnchorPreset = AnchorPresets.MiddleCenter,
                        Parent = this,
                        Bounds = new Rectangle(Width * 0.5f - 1 - CameraCutThumbnailRenderer.Width * 0.5f, 2, CameraCutThumbnailRenderer.Width, CameraCutThumbnailRenderer.Height),
                    };
                }
                image.UnlockChildrenRecursive();
                _thumbnails[req.ThumbnailIndex] = image;
                UpdateUI();
            }
            else if (image.Brush != null)
            {
                Timeline.CameraCutThumbnailRenderer.ReleaseThumbnail(((SpriteBrush)image.Brush).Sprite);
                image.Brush = null;
            }

            image.Brush = new SpriteBrush(sprite);
        }

        private void UpdateUI()
        {
            if (_thumbnails == null)
                return;
            var width = Width - (_thumbnails.Length + 1) * 2;
            if (width < 10.0f)
            {
                for (int i = 0; i < _thumbnails.Length; i++)
                {
                    var image = _thumbnails[i];
                    if (image != null)
                        image.Visible = false;
                }
                return;
            }
            var count = Mathf.Min(Mathf.FloorToInt(width / CameraCutThumbnailRenderer.Width), _thumbnails.Length);
            if (count == 0 && _thumbnails.Length != 0)
            {
                var image = _thumbnails[0];
                if (image != null)
                {
                    image.Width = Mathf.Min(CameraCutThumbnailRenderer.Width, width);
                    image.SetAnchorPreset(image.AnchorPreset, false);
                    image.Visible = true;
                }
                return;
            }
            for (int i = 0; i < count; i++)
            {
                var image = _thumbnails[i];
                if (image != null)
                {
                    image.Width = CameraCutThumbnailRenderer.Width;
                    image.SetAnchorPreset(image.AnchorPreset, false);
                    image.Visible = true;
                }
            }
            for (int i = count; i < _thumbnails.Length; i++)
            {
                var image = _thumbnails[i];
                if (image != null)
                {
                    image.Visible = false;
                }
            }
        }

        /// <inheritdoc />
        public override void OnTimelineChanged(Track track)
        {
            base.OnTimelineChanged(track);

            PropertiesEditObject = track != null ? new Proxy((CameraCutTrack)track, this) : null;
            UpdateThumbnails();
        }

        /// <inheritdoc />
        protected override void OnStartFrameChanged()
        {
            base.OnStartFrameChanged();

            UpdateThumbnails();
        }

        /// <inheritdoc />
        protected override void OnDurationFramesChanged()
        {
            base.OnDurationFramesChanged();

            UpdateThumbnails(new[] { 1, 2 });
        }

        /// <inheritdoc />
        protected override void OnSizeChanged()
        {
            base.OnSizeChanged();

            UpdateUI();
        }

        /// <inheritdoc />
        public override void OnTimelineContextMenu(ContextMenu.ContextMenu menu, float time, Control controlUnderMouse)
        {
            if (((CameraCutTrack)Track).Camera)
                menu.AddButton("Refresh thumbnails", () => UpdateThumbnails());

            base.OnTimelineContextMenu(menu, time, controlUnderMouse);
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            Timeline?.CameraCutThumbnailRenderer?.RemoveRequest(this);
            _thumbnails = null;

            base.OnDestroy();
        }
    }

    /// <summary>
    /// The helper utility for rendering camera cuts tracks thumbnails.
    /// </summary>
    [HideInEditor]
    public class CameraCutThumbnailRenderer
    {
        /// <summary>
        /// The camera cut thumbnail rendering request.
        /// </summary>
        public struct Request : IEquatable<Request>
        {
            /// <summary>
            /// The media.
            /// </summary>
            public CameraCutMedia Media;

            /// <summary>
            /// The thumbnail index.
            /// </summary>
            public int ThumbnailIndex;

            /// <summary>
            /// Initializes a new instance of the <see cref="Request"/> struct.
            /// </summary>
            /// <param name="media">The media.</param>
            /// <param name="thumbnailIndex"> The index of the thumbnail.</param>
            public Request(CameraCutMedia media, int thumbnailIndex)
            {
                Media = media;
                ThumbnailIndex = thumbnailIndex;
            }

            /// <inheritdoc />
            public bool Equals(Request other)
            {
                return Equals(Media, other.Media) && ThumbnailIndex == other.ThumbnailIndex;
            }

            /// <inheritdoc />
            public override bool Equals(object obj)
            {
                return obj is Request other && Equals(other);
            }

            /// <inheritdoc />
            public override int GetHashCode()
            {
                unchecked
                {
                    return ((Media != null ? Media.GetHashCode() : 0) * 397) ^ ThumbnailIndex;
                }
            }
        }

        /// <summary>
        /// The thumbnails atlas.
        /// </summary>
        private struct Atlas
        {
            /// <summary>
            /// The atlas texture.
            /// </summary>
            public SpriteAtlas Texture;

            /// <summary>
            /// The slots usage flags.
            /// </summary>
            public BitArray SlotsUsage;

            /// <summary>
            /// The used slots count.
            /// </summary>
            public int Count;

            /// <summary>
            /// Gets a value indicating whether this instance is full.
            /// </summary>
            public bool IsFull => SlotsUsage.Length == Count;
        }

        /// <summary>
        /// The thumbnail height.
        /// </summary>
        public static int Height => 64;

        /// <summary>
        /// The thumbnail width.
        /// </summary>
        public static int Width => (int)(Height * (16.0f / 9.0f));

        private List<Request> _queue;
        private SceneRenderTask _task;
        private GPUTexture _output;
        private List<Atlas> _atlases;

        /// <summary>
        /// Initializes a new instance of the <see cref="CameraCutThumbnailRenderer"/> class.
        /// </summary>
        public CameraCutThumbnailRenderer()
        {
            _queue = new List<Request>();
            FlaxEngine.Scripting.Update += OnUpdate;
        }

        /// <summary>
        /// Adds the request for thumbnail rendering.
        /// </summary>
        /// <param name="req">The request.</param>
        public void AddRequest(Request req)
        {
            if (!_queue.Contains(req))
                _queue.Add(req);
        }

        /// <summary>
        /// Removes all the requests that are related to the given media.
        /// </summary>
        /// <param name="media">The media.</param>
        public void RemoveRequest(CameraCutMedia media)
        {
            _queue.RemoveAll(x => x.Media == media);

            // End rendering if queue is not empty
            if (_queue.Count == 0 && _task != null && _task.Enabled)
                _task.Enabled = false;
        }

        /// <summary>
        /// Releases the thumbnail ans frees the sprite slot used by it.
        /// </summary>
        /// <param name="sprite">The sprite.</param>
        public void ReleaseThumbnail(SpriteHandle sprite)
        {
            if (!sprite.IsValid)
                return;

            for (var i = 0; i < _atlases.Count; i++)
            {
                var atlas = _atlases[i];
                if (atlas.Texture == sprite.Atlas)
                {
                    atlas.Count--;
                    atlas.SlotsUsage[sprite.Index] = false;

                    _atlases[i] = atlas;
                    break;
                }
            }
        }

        /// <summary>
        /// Releases object resources.
        /// </summary>
        public void Dispose()
        {
            FlaxEngine.Scripting.Update -= OnUpdate;
            _queue.Clear();
            _queue = null;
            Object.Destroy(ref _task);
            Object.Destroy(ref _output);
            if (_atlases != null)
            {
                foreach (var atlas in _atlases)
                    Object.Destroy(atlas.Texture);
                _atlases.Clear();
                _atlases = null;
            }
        }

        private void OnUpdate()
        {
            if (_queue.Count == 0 || (_task != null && _task.Enabled))
                return;

            // TODO: add delay when processing the requests to reduce perf impact (eg. 0.2s before actual rendering)

            // Setup pipeline
            if (_atlases == null)
                _atlases = new List<Atlas>(4);
            if (_output == null)
            {
                _output = GPUDevice.Instance.CreateTexture("CameraCutMedia.Output");
                var desc = GPUTextureDescription.New2D(Width, Height, PixelFormat.R8G8B8A8_UNorm);
                _output.Init(ref desc);
            }
            if (_task == null)
            {
                _task = Object.New<SceneRenderTask>();
                _task.Output = _output;
                _task.Begin += OnBegin;
                _task.End += OnEnd;
            }

            // Kick off the rendering
            _task.Enabled = true;
        }

        private void OnBegin(RenderTask task, GPUContext context)
        {
            // Setup
            var req = _queue[0];
            req.Media.OnThumbnailRenderingBegin((SceneRenderTask)task, context, ref req);
        }

        private void OnEnd(RenderTask task, GPUContext context)
        {
            // Pick the atlas or create a new one
            int atlasIndex = -1;
            for (int i = 0; i < _atlases.Count; i++)
            {
                if (!_atlases[i].IsFull)
                {
                    atlasIndex = i;
                    break;
                }
            }
            if (atlasIndex == -1)
            {
                // Setup configuration
                var atlasSize = 1024;
                var atlasFormat = PixelFormat.R8G8B8A8_UNorm;
                var width = (float)Width;
                var height = (float)Height;
                var countX = Mathf.FloorToInt(atlasSize / width);
                var countY = Mathf.FloorToInt(atlasSize / height);
                var count = countX * countY;

                // Create sprite atlas texture
                var spriteAtlas = FlaxEngine.Content.CreateVirtualAsset<SpriteAtlas>();
                var data = new byte[atlasSize * atlasSize * PixelFormatExtensions.SizeInBytes(atlasFormat)];
                var initData = new TextureBase.InitData
                {
                    Width = atlasSize,
                    Height = atlasSize,
                    ArraySize = 1,
                    Format = atlasFormat,
                    Mips = new[]
                    {
                        new TextureBase.InitData.MipData
                        {
                            Data = data,
                            RowPitch = data.Length / atlasSize,
                            SlicePitch = data.Length
                        },
                    },
                };
                spriteAtlas.Init(ref initData);

                // Setup sprite atlas slots (each per thumbnail)
                var thumbnailSizeUV = new Float2(width / atlasSize, height / atlasSize);
                for (int i = 0; i < count; i++)
                {
                    var x = i % countX;
                    var y = i / countX;
                    var s = new Sprite
                    {
                        Name = string.Empty,
                        Area = new Rectangle(new Float2(x, y) * thumbnailSizeUV, thumbnailSizeUV),
                    };
                    spriteAtlas.AddSprite(s);
                }

                // Add atlas to the cached ones
                atlasIndex = _atlases.Count;
                _atlases.Add(new Atlas
                {
                    Texture = spriteAtlas,
                    SlotsUsage = new BitArray(count, false),
                    Count = 0,
                });
            }

            // Skip ending if the atlas is not loaded yet (streaming backend uploads texture to GPU or sth)
            var atlas = _atlases[atlasIndex];
            if (atlas.Texture.ResidentMipLevels == 0)
                return;

            // Pick the sprite slot from the atlas
            var spriteIndex = -1;
            for (int i = 0; i < atlas.SlotsUsage.Count; i++)
            {
                if (atlas.SlotsUsage[i] == false)
                {
                    atlas.SlotsUsage[i] = true;
                    spriteIndex = i;
                    break;
                }
            }
            if (spriteIndex == -1)
                throw new Exception();
            atlas.Count++;
            _atlases[atlasIndex] = atlas;
            var sprite = new SpriteHandle(atlas.Texture, spriteIndex);

            // Copy output frame to the sprite atlas slot
            var spriteLocation = sprite.Location;
            context.CopyTexture(atlas.Texture.Texture, 0, (uint)spriteLocation.X, (uint)spriteLocation.Y, 0, _output, 0);

            // Link sprite to the UI
            var req = _queue[0];
            req.Media.OnThumbnailRenderingEnd((SceneRenderTask)task, context, ref req, ref sprite);

            // End
            _queue.RemoveAt(0);
            task.Enabled = false;
        }
    }

    /// <summary>
    /// The timeline track for animating <see cref="FlaxEngine.Camera"/> objects.
    /// </summary>
    /// <seealso cref="ActorTrack" />
    public class CameraCutTrack : ActorTrack
    {
        /// <summary>
        /// Gets the archetype.
        /// </summary>
        /// <returns>The archetype.</returns>
        public new static TrackArchetype GetArchetype()
        {
            return new TrackArchetype
            {
                TypeId = 16,
                Name = "Camera Cut",
                Create = options => new CameraCutTrack(ref options),
                Load = LoadTrack,
                Save = SaveTrack,
            };
        }

        private static void LoadTrack(int version, Track track, BinaryReader stream)
        {
            var e = (CameraCutTrack)track;
            e.ActorID = stream.ReadGuid();
            if (version <= 3)
            {
                // [Deprecated on 03.09.2021 expires on 03.09.2023]
                var m = e.TrackMedia;
                m.StartFrame = stream.ReadInt32();
                m.DurationFrames = stream.ReadInt32();
            }
            else
            {
                var count = stream.ReadInt32();
                while (e.Media.Count > count)
                    e.RemoveMedia(e.Media.Last());
                while (e.Media.Count < count)
                    e.AddMedia(new CameraCutMedia());
                for (int i = 0; i < count; i++)
                {
                    var m = (CameraCutMedia)e.Media[i];
                    m.StartFrame = stream.ReadInt32();
                    m.DurationFrames = stream.ReadInt32();
                    m.UpdateThumbnails();
                }
            }
        }

        private static void SaveTrack(Track track, BinaryWriter stream)
        {
            var e = (CameraCutTrack)track;
            stream.WriteGuid(ref e.ActorID);
            var count = e.Media.Count;
            stream.Write(count);
            for (int i = 0; i < count; i++)
            {
                var m = e.Media[i];
                stream.Write(m.StartFrame);
                stream.Write(m.DurationFrames);
            }
        }

        private Image _pilotCamera;

        /// <summary>
        /// Gets the camera object instance (it might be missing).
        /// </summary>
        public Camera Camera => Actor as Camera;

        private CameraCutMedia TrackMedia
        {
            get
            {
                CameraCutMedia media;
                if (Media.Count == 0)
                {
                    media = new CameraCutMedia
                    {
                        StartFrame = 0,
                        DurationFrames = Timeline != null ? (int)(Timeline.FramesPerSecond * 2) : 60,
                    };
                    AddMedia(media);
                }
                else
                {
                    media = (CameraCutMedia)Media[0];
                }
                return media;
            }
        }

        /// <inheritdoc />
        public CameraCutTrack(ref TrackCreateOptions options)
        : base(ref options, false)
        {
            MinMediaCount = 1;
            Height = CameraCutThumbnailRenderer.Height + 8;

            // Pilot Camera button
            const float buttonSize = 18;
            var icons = Editor.Instance.Icons;
            _pilotCamera = new Image
            {
                TooltipText = "Starts piloting camera (in scene edit window)",
                AutoFocus = true,
                AnchorPreset = AnchorPresets.MiddleRight,
                IsScrollable = false,
                Color = Style.Current.ForegroundGrey,
                Margin = new Margin(1),
                Brush = new SpriteBrush(icons.CameraFill32),
                Offsets = new Margin(-buttonSize - 2 + _selectActor.Offsets.Left, buttonSize, buttonSize * -0.5f, buttonSize),
                Parent = this,
            };
            _pilotCamera.Clicked += OnClickedPilotCamera;
        }

        private void OnClickedPilotCamera(Image image, MouseButton button)
        {
            if (button == MouseButton.Left)
            {
                var camera = Camera;
                if (camera)
                {
                    Expand();

                    var editWin = Editor.Instance.Windows.EditWin;
                    editWin.PilotActor(camera);

                    var time = Timeline.CurrentTime;
                    var hasPositionTrack = false;
                    var hasOrientationTrack = false;
                    foreach (var subTrack in SubTracks)
                    {
                        if (subTrack is MemberTrack memberTrack)
                        {
                            object value = memberTrack.Evaluate(time);
                            if (value != null)
                            {
                                if (memberTrack.MemberName == "Position" && value is Vector3 asPosition)
                                {
                                    editWin.Viewport.ViewPosition = asPosition;
                                    hasPositionTrack = true;
                                }
                                else if (memberTrack.MemberName == "Orientation" && value is Quaternion asRotation)
                                {
                                    editWin.Viewport.ViewOrientation = asRotation;
                                    hasOrientationTrack = true;
                                }
                            }
                        }
                    }
                    if (!hasPositionTrack)
                        AddPropertyTrack(camera.GetType().GetProperty("Position"));
                    if (!hasOrientationTrack)
                        AddPropertyTrack(camera.GetType().GetProperty("Orientation"));
                }
            }
        }

        private void UpdateThumbnails()
        {
            foreach (CameraCutMedia media in Media)
                media.UpdateThumbnails();
        }

        /// <inheritdoc />
        protected override void OnObjectExistenceChanged(object obj)
        {
            base.OnObjectExistenceChanged(obj);

            UpdateThumbnails();
        }

        /// <inheritdoc />
        protected override bool IsActorValid(Actor actor)
        {
            return base.IsActorValid(actor) && actor is Camera;
        }

        /// <inheritdoc />
        protected override void OnActorChanged()
        {
            base.OnActorChanged();

            UpdateThumbnails();
        }

        /// <inheritdoc />
        public override void OnSpawned()
        {
            // Ensure to have valid media added
            // ReSharper disable once UnusedVariable
            var media = TrackMedia;

            base.OnSpawned();
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _pilotCamera = null;

            base.OnDestroy();
        }
    }
}
