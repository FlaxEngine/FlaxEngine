// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using FlaxEditor.GUI.Drag;
using FlaxEditor.GUI.Timeline.Undo;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI.Timeline
{
    /// <summary>
    /// The Timeline track that contains a header and custom timeline events/media.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    [HideInEditor]
    public class Track : ContainerControl
    {
        /// <summary>
        /// The default prefix for drag data used for tracks dragging.
        /// </summary>
        public const string DragPrefix = "TRACK!?";

        /// <summary>
        /// The default node offset in y.
        /// </summary>
        public const float DefaultNodeOffsetY = 1.0f;

        /// <summary>
        /// The default drag insert position margin.
        /// </summary>
        public const float DefaultDragInsertPositionMargin = 4.0f;

        /// <summary>
        /// The header height.
        /// </summary>
        public const float HeaderHeight = 22.0f;

        private Timeline _timeline;
        private int _trackIndexCached = -1;
        private TrackFlags _flags;
        private Track _parentTrack;
        internal float _xOffset;
        private Margin _margin = new Margin(2.0f);
        private readonly List<Media> _media = new List<Media>();
        private readonly List<Track> _subTracks = new List<Track>();
        private bool _opened;
        private bool _isMouseDown;
        private bool _mouseOverArrow;
        private Float2 _mouseDownPos;

        /// <summary>
        /// The mute track checkbox.
        /// </summary>
        protected CheckBox _muteCheckbox;

        private DragTracks _dragTracks;
        private DragHandlers _dragHandlers;
        private DragItemPositioning _dragOverMode;
        private bool _isDragOverHeader;

        /// <summary>
        /// Gets the parent timeline.
        /// </summary>
        public Timeline Timeline => _timeline;

        /// <summary>
        /// Gets the parent track (null if this track is one of the root tracks in timeline).
        /// </summary>
        public Track ParentTrack
        {
            get => _parentTrack;
            set
            {
                if (_parentTrack != value)
                {
                    _parentTrack?.RemoveSubTrack(this);
                    value?.AdSubTrack(this);
                }
            }
        }

        /// <summary>
        /// Gets or sets the index of the track (in timeline track list).
        /// </summary>
        public int TrackIndex
        {
            get
            {
                if (_timeline == null)
                    return _trackIndexCached;
                int result = -1;
                for (int i = 0; i < _timeline.Tracks.Count; i++)
                {
                    if (_timeline.Tracks[i] == this)
                    {
                        result = i;
                        break;
                    }
                }
                return result;
            }
            set
            {
                if (_timeline != null)
                    _timeline.ChangeTrackIndex(this, value);
                else
                    _trackIndexCached = value;
            }
        }

        /// <summary>
        /// Gets the collection of the media events added to this track (read-only list).
        /// </summary>
        public IReadOnlyList<Media> Media => _media;

        /// <summary>
        /// Occurs when collection of the media events gets changed.
        /// </summary>
        public event Action<Track> MediaChanged;

        /// <summary>
        /// Gets the collection of the child tracks added to this track (read-only list).
        /// </summary>
        public IReadOnlyList<Track> SubTracks => _subTracks;

        /// <summary>
        /// Occurs when collection of the sub tracks gets changed.
        /// </summary>
        public event Action<Track> SubTracksChanged;

        /// <summary>
        /// The track text.
        /// </summary>
        public string Name;

        /// <summary>
        /// The track custom title text (name is used if title is null).
        /// </summary>
        public string Title;

        /// <summary>
        /// The track title color.
        /// </summary>
        public Color TitleTintColor = Color.White;

        /// <summary>
        /// The track icon.
        /// </summary>
        public SpriteHandle Icon;

        /// <summary>
        /// The icon color (tint).
        /// </summary>
        public Color IconColor = Color.White;

        /// <summary>
        /// The track color.
        /// </summary>
        public Color Color = Color.White;

        /// <summary>
        /// The track flags.
        /// </summary>
        public TrackFlags Flags
        {
            get => _flags;
            set
            {
                if (_flags == value)
                    return;
                _flags = value;
                _muteCheckbox.Checked = (Flags & TrackFlags.Mute) == 0;
                Timeline?.MarkAsEdited();
            }
        }

        /// <summary>
        /// Controls <see cref="TrackFlags.Mute"/> flag.
        /// </summary>
        public bool Mute
        {
            get => (Flags & TrackFlags.Mute) != 0;
            set => Flags = value ? (Flags | TrackFlags.Mute) : (Flags & ~TrackFlags.Mute);
        }

        /// <summary>
        /// Controls <see cref="TrackFlags.Loop"/> flag.
        /// </summary>
        public bool Loop
        {
            get => (Flags & TrackFlags.Loop) != 0;
            set => Flags = value ? (Flags | TrackFlags.Loop) : (Flags & ~TrackFlags.Loop);
        }

        /// <summary>
        /// The minimum amount of media items for this track.
        /// </summary>
        public int MinMediaCount = 0;

        /// <summary>
        /// The maximum amount of media items for this track.
        /// </summary>
        public int MaxMediaCount = 1024;

        /// <summary>
        /// The track archetype.
        /// </summary>
        public TrackArchetype Archetype;

        internal bool DrawDisabled;

        /// <summary>
        /// Gets a value indicating whether this track is expanded and all of its parents are also expanded.
        /// </summary>
        public bool IsExpandedAll => (ParentTrack == null || ParentTrack.IsExpandedAll) && (!CanExpand || IsExpanded);

        /// <summary>
        /// Gets a value indicating whether this all of this track parents are expanded.
        /// </summary>
        public bool HasParentsExpanded => (ParentTrack == null || ParentTrack.IsExpandedAll);

        /// <summary>
        /// Gets a value indicating whether this track has any sub-tracks.
        /// </summary>
        public bool HasSubTracks => _subTracks.Count > 0;

        /// <summary>
        /// Gets or sets a value indicating whether this track is expanded.
        /// </summary>
        public bool IsExpanded
        {
            get => _opened;
            set
            {
                if (value)
                    Expand();
                else
                    Collapse();
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether this track is collapsed.
        /// </summary>
        public bool IsCollapsed
        {
            get => !_opened;
            set
            {
                if (value)
                    Collapse();
                else
                    Expand();
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Track"/> class.
        /// </summary>
        /// <param name="options">The track initial options.</param>
        public Track(ref TrackCreateOptions options)
        {
            AutoFocus = false;
            Offsets = new Margin(0, 100, 0, HeaderHeight);

            Archetype = options.Archetype;
            Name = options.Archetype.Name;
            Icon = options.Archetype.Icon;
            _flags = options.Flags;

            // Mute checkbox
            const float buttonSize = 14;
            _muteCheckbox = new CheckBox
            {
                TooltipText = "Mute track",
                AutoFocus = true,
                Checked = (Flags & TrackFlags.Mute) == 0,
                AnchorPreset = AnchorPresets.MiddleRight,
                Offsets = new Margin(-buttonSize - 2, buttonSize, buttonSize * -0.5f, buttonSize),
                IsScrollable = false,
                Parent = this,
            };
            _muteCheckbox.StateChanged += OnMuteButtonStateChanged;
        }

        private void OnMuteButtonStateChanged(CheckBox checkBox)
        {
            if (Mute == !checkBox.Checked)
                return;
            using (new TrackUndoBlock(this))
                Mute = !checkBox.Checked;
        }

        /// <summary>
        /// Gets the arrow rectangle.
        /// </summary>
        protected Rectangle ArrowRect => new Rectangle(_xOffset + 2 + _margin.Left, (HeaderHeight - 12) * 0.5f, 12, 12);

        /// <summary>
        /// Called when parent timeline gets changed.
        /// </summary>
        /// <param name="timeline">The timeline.</param>
        public virtual void OnTimelineChanged(Timeline timeline)
        {
            _timeline = timeline;
            if (_trackIndexCached != -1)
                _timeline.ChangeTrackIndex(this, _trackIndexCached);

            for (var i = 0; i < _media.Count; i++)
            {
                _media[i].OnTimelineChanged(this);
            }

            UnlockChildrenRecursive();
        }

        /// <summary>
        /// Called when timeline zoom gets changed.
        /// </summary>
        public virtual void OnTimelineZoomChanged()
        {
            for (var i = 0; i < _media.Count; i++)
            {
                _media[i].OnTimelineZoomChanged();
            }
        }

        /// <summary>
        /// Called when timeline FPS gets changed.
        /// </summary>
        /// <param name="before">The before value.</param>
        /// <param name="after">The after value.</param>
        public virtual void OnTimelineFpsChanged(float before, float after)
        {
            for (var i = 0; i < _media.Count; i++)
            {
                _media[i].OnTimelineFpsChanged(before, after);
            }
        }

        /// <summary>
        /// Called when timeline current frame gets changed.
        /// </summary>
        /// <param name="frame">The frame.</param>
        public virtual void OnTimelineCurrentFrameChanged(int frame)
        {
        }

        /// <summary>
        /// Called when undo action gets reverted or applied for this track data. Can be used to update UI.
        /// </summary>
        public virtual void OnUndo()
        {
            for (var i = 0; i < _media.Count; i++)
            {
                _media[i].OnUndo();
            }
        }

        /// <summary>
        /// Called when parent track gets changed.
        /// </summary>
        /// <param name="parent">The parent track.</param>
        public virtual void OnParentTrackChanged(Track parent)
        {
            _parentTrack = parent;
        }

        /// <summary>
        /// Determines whether the specified track contains is contained in this track sub track or any sub track children.
        /// </summary>
        /// <param name="track">The track.</param>
        /// <returns><c>true</c> if this track contains the specified track; otherwise, <c>false</c>.</returns>
        public bool ContainsTrack(Track track)
        {
            return _subTracks.Any(x => x == track || x.ContainsTrack(track));
        }

        /// <summary>
        /// Called when track gets loaded from the serialized timeline data.
        /// </summary>
        public virtual void OnLoaded()
        {
        }

        /// <summary>
        /// Called when tracks gets spawned by the user.
        /// </summary>
        public virtual void OnSpawned()
        {
        }

        /// <summary>
        /// Called when tracks gets removed by the user.
        /// </summary>
        public virtual void OnDeleted()
        {
            for (var i = 0; i < _media.Count; i++)
            {
                _media[i].OnDeleted();
            }

            Dispose();
        }

        /// <summary>
        /// Called when track gets duplicated.
        /// </summary>
        /// <param name="clone">The cloned track instance.</param>
        public virtual void OnDuplicated(Track clone)
        {
        }

        /// <summary>
        /// Arranges the track and all its media. Called when timeline performs layout for the contents.
        /// </summary>
        public virtual void OnTimelineArrange()
        {
            if (ParentTrack == null)
            {
                _xOffset = 0;
                Visible = true;
            }
            else
            {
                _xOffset = ParentTrack._xOffset + 12.0f;
                Visible = ParentTrack.Visible && ParentTrack.IsExpanded;
            }

            for (int j = 0; j < Media.Count; j++)
            {
                var media = Media[j];

                media.Visible = Visible;
                media.Bounds = new Rectangle(media.X, Y + 2, media.Width, Height - 4);
            }
        }

        /// <summary>
        /// Gets the frame of the next keyframe (if found).
        /// </summary>
        /// <param name="time">The start time.</param>
        /// <param name="result">The result value.</param>
        /// <returns>True if found next keyframe, otherwise false.</returns>
        public virtual bool GetNextKeyframeFrame(float time, out int result)
        {
            result = 0;
            return false;
        }

        /// <summary>
        /// Gets the frame of the previous keyframe (if found).
        /// </summary>
        /// <param name="time">The start time.</param>
        /// <param name="result">The result value.</param>
        /// <returns>True if found previous keyframe, otherwise false.</returns>
        public virtual bool GetPreviousKeyframeFrame(float time, out int result)
        {
            result = 0;
            return false;
        }

        /// <summary>
        /// Adds the media.
        /// </summary>
        /// <param name="media">The media.</param>
        public virtual void AddMedia(Media media)
        {
            _media.Add(media);
            media.OnTimelineChanged(this);

            OnMediaChanged();

            media.UnlockChildrenRecursive();
            media.PerformLayout();
        }

        /// <summary>
        /// Removes the media.
        /// </summary>
        /// <param name="media">The media.</param>
        public virtual void RemoveMedia(Media media)
        {
            media.OnTimelineChanged(null);
            _media.Remove(media);

            OnMediaChanged();
        }

        /// <summary>
        /// Adds the sub track.
        /// </summary>
        /// <param name="track">The track.</param>
        public virtual void AdSubTrack(Track track)
        {
            _subTracks.Add(track);
            track.OnParentTrackChanged(this);

            OnSubTracksChanged();
        }

        /// <summary>
        /// Removes the sub track.
        /// </summary>
        /// <param name="track">The track.</param>
        public virtual void RemoveSubTrack(Track track)
        {
            track.OnParentTrackChanged(null);
            _subTracks.Remove(track);

            OnSubTracksChanged();
        }

        /// <summary>
        /// Called when collection of the media items gets changed.
        /// </summary>
        protected virtual void OnMediaChanged()
        {
            MediaChanged?.Invoke(this);
            _timeline?.ArrangeTracks();
        }

        /// <summary>
        /// Called when collection of the sub tracks gets changed.
        /// </summary>
        protected virtual void OnSubTracksChanged()
        {
            SubTracksChanged?.Invoke(this);
        }

        sealed class DragTracks : DragNames<DragEventArgs>
        {
            private Track _track;

            public DragTracks(Track track)
            : base(Track.DragPrefix, track.ValidateTrackDrag)
            {
                _track = track;
            }

            public List<Track> Tracks => Objects.ConvertAll(x => _track.Timeline.Tracks.FirstOrDefault(y => y.Name == x));

            /// <inheritdoc />
            public override DragDropEffect Effect
            {
                get
                {
                    var result = base.Effect;

                    // Reject drag over if one of the tracks cannot be moved to this track
                    if (result != DragDropEffect.None)
                    {
                        var tracks = Tracks;
                        for (int i = 0; i < tracks.Count; i++)
                        {
                            var track = tracks[i];
                            if (track == null)
                                continue;

                            bool blockDrag = false;
                            switch (_track._dragOverMode)
                            {
                            case DragItemPositioning.At:
                                blockDrag = !_track.CanAddChildTrack(track);
                                break;
                            case DragItemPositioning.Above:
                            case DragItemPositioning.Below:
                                var parent = _track.ParentTrack;
                                blockDrag = parent != null && !parent.CanAddChildTrack(track);
                                break;
                            }
                            if (blockDrag)
                                return DragDropEffect.None;
                        }
                    }

                    return result;
                }
            }
        }

        /// <summary>
        /// Called when drag and drop enters the track header area.
        /// </summary>
        /// <param name="data">The data.</param>
        /// <returns>Drag action response.</returns>
        protected virtual DragDropEffect OnDragEnterHeader(DragData data)
        {
            if (_dragHandlers == null)
                _dragHandlers = new DragHandlers();

            // Check if drop tracks
            if (_dragTracks == null)
            {
                _dragTracks = new DragTracks(this);
                _dragHandlers.Add(_dragTracks);
            }
            if (_dragTracks.OnDragEnter(data))
                return _dragTracks.Effect;

            return DragDropEffect.None;
        }

        private bool ValidateTrackDrag(string name)
        {
            var track = _timeline.Tracks.FirstOrDefault(x => x.Name == name);

            // Reject dragging parents and itself
            return track != null && track != this && !track.ContainsTrack(this);
        }

        /// <summary>
        /// Called when drag and drop moves over the track header area.
        /// </summary>
        /// <param name="data">The data.</param>
        /// <returns>Drag action response.</returns>
        protected virtual DragDropEffect OnDragMoveHeader(DragData data)
        {
            return _dragHandlers.Effect;
        }

        /// <summary>
        /// Called when drag and drop performs over the track header area.
        /// </summary>
        /// <param name="data">The data.</param>
        /// <returns>Drag action response.</returns>
        protected virtual DragDropEffect OnDragDropHeader(DragData data)
        {
            var result = DragDropEffect.None;

            // Use drag positioning to change target parent and index
            Track newParent;
            int newOrder;
            if (_dragOverMode == DragItemPositioning.Above)
            {
                newParent = _parentTrack;
                newOrder = TrackIndex;
            }
            else if (_dragOverMode == DragItemPositioning.Below)
            {
                newParent = _parentTrack;
                newOrder = TrackIndex + 1;
            }
            else
            {
                newParent = this;
                newOrder = (_subTracks.Count > 0 ? _subTracks.Last().TrackIndex : TrackIndex) + 1;
            }

            // Drag tracks
            if (_dragTracks != null && _dragTracks.HasValidDrag)
            {
                var tracks = _dragTracks.Tracks;
                if (tracks.Count == 1 && tracks[0] != null)
                {
                    var track = tracks[0];
                    _timeline.Undo?.AddAction(new ReorderTrackAction(_timeline, track, track.TrackIndex, newOrder, track.ParentTrack?.Name, newParent?.Name));
                    track.ParentTrack = newParent;
                    track.TrackIndex = newOrder;
                }
                else
                {
                    var actions = new List<IUndoAction>(tracks.Count);
                    for (int i = 0; i < tracks.Count; i++)
                    {
                        var track = tracks[i];
                        if (track != null)
                        {
                            actions.Add(new ReorderTrackAction(_timeline, track, track.TrackIndex, newOrder, track.ParentTrack?.Name, newParent?.Name));
                            track.ParentTrack = newParent;
                            track.TrackIndex = newOrder;
                        }
                    }
                    _timeline.Undo?.AddAction(new MultiUndoAction(actions));
                }
                _timeline.OnTracksOrderChanged();
                _timeline.MarkAsEdited();

                result = DragDropEffect.Move;
            }

            // Clear cache
            _dragHandlers.OnDragDrop(null);

            // Expand if dropped sth
            if (result != DragDropEffect.None)
            {
                Expand();
            }

            return result;
        }

        /// <summary>
        /// Called when drag and drop leaves the track header area.
        /// </summary>
        protected virtual void OnDragLeaveHeader()
        {
            _dragHandlers.OnDragLeave();
        }

        /// <summary>
        /// Begins the drag drop operation.
        /// </summary>
        protected virtual void DoDragDrop()
        {
            DragData data;

            // Check if this node is selected
            if (_timeline.SelectedTracks.Contains(this))
            {
                // Get selected tracks
                var names = new List<string>();
                for (var i = 0; i < _timeline.SelectedTracks.Count; i++)
                {
                    var track = _timeline.SelectedTracks[i];
                    if (track.CanDrag)
                        names.Add(track.Name);
                }
                data = DragNames.GetDragData(DragPrefix, names);
            }
            else
            {
                data = DragNames.GetDragData(DragPrefix, Name);
            }

            // Start drag operation
            DoDragDrop(data);
        }

        /// <summary>
        /// Called when expanded state gets changed.
        /// </summary>
        protected virtual void OnExpandedChanged()
        {
            _timeline?.ArrangeTracks();
        }

        /// <summary>
        /// Gets a value indicating whether user can drag this track.
        /// </summary>
        public virtual bool CanDrag => true;

        /// <summary>
        /// Gets a value indicating whether user can rename this track.
        /// </summary>
        public virtual bool CanRename => true;

        /// <summary>
        /// Gets a value indicating whether user can copy and paste this track.
        /// </summary>
        public virtual bool CanCopyPaste => true;

        /// <summary>
        /// Gets a value indicating whether user can expand the track contents of the inner hierarchy.
        /// </summary>
        public virtual bool CanExpand => SubTracks.Count > 0;

        /// <summary>
        /// Determines whether this track can get the child track.
        /// </summary>
        /// <param name="track">The track.</param>
        /// <returns>True if can add this track, otherwise false.</returns>
        protected virtual bool CanAddChildTrack(Track track)
        {
            return false;
        }

        /// <summary>
        /// Updates the drag over mode based on the given mouse location.
        /// </summary>
        /// <param name="location">The location.</param>
        private void UpdateDragPositioning(ref Float2 location)
        {
            // Check collision with drag areas
            if (new Rectangle(0, 0 - DefaultDragInsertPositionMargin - DefaultNodeOffsetY, Width, DefaultDragInsertPositionMargin * 2.0f).Contains(location))
                _dragOverMode = DragItemPositioning.Above;
            else if (IsCollapsed && new Rectangle(0, Height - DefaultDragInsertPositionMargin, Width, DefaultDragInsertPositionMargin * 2.0f).Contains(location))
                _dragOverMode = DragItemPositioning.Below;
            else
                _dragOverMode = DragItemPositioning.At;

            // Update DraggedOverTrack
            var timeline = Timeline;
            if (_dragOverMode == DragItemPositioning.None)
            {
                if (timeline != null && timeline.DraggedOverTrack == this)
                    timeline.DraggedOverTrack = null;
            }
            else if (timeline != null)
                timeline.DraggedOverTrack = this;
        }

        private void ClearDragPositioning()
        {
            _dragOverMode = DragItemPositioning.None;
            var timeline = Timeline;
            if (timeline != null && timeline.DraggedOverTrack == this)
                timeline.DraggedOverTrack = null;
        }

        /// <summary>
        /// Tests the header hit.
        /// </summary>
        /// <param name="location">The location.</param>
        /// <returns>True if hits it.</returns>
        protected virtual bool TestHeaderHit(ref Float2 location)
        {
            return new Rectangle(0, 0, Width, HeaderHeight).Contains(ref location);
        }

        /// <summary>
        /// Starts the track renaming action.
        /// </summary>
        public void StartRenaming()
        {
            _timeline.Select(this, false);

            // Start renaming the track
            var dialog = RenamePopup.Show(this, new Rectangle(0, 0, Width, HeaderHeight), Name, false);
            dialog.Validate += OnRenameValidate;
            dialog.Renamed += OnRenamed;
        }

        private bool OnRenameValidate(RenamePopup popup, string value)
        {
            return _timeline.IsTrackNameValid(value);
        }

        private void OnRenamed(RenamePopup renamePopup)
        {
            var oldName = Name;
            OnRename(renamePopup.Text);
            _timeline.Undo?.AddAction(new RenameTrackAction(_timeline, this, oldName, Name));
        }

        /// <summary>
        /// Called when track should be renamed.
        /// </summary>
        /// <param name="newName">The new name.</param>
        protected virtual void OnRename(string newName)
        {
            Name = newName;
            _timeline?.MarkAsEdited();
        }

        /// <summary>
        /// Renames the track to the specified name and handles duplicated track names (adds number after the given name to make it unique).
        /// </summary>
        /// <param name="name">The base name.</param>
        public void Rename(string name)
        {
            if (_timeline != null)
                name = _timeline.GetValidTrackName(name);
            OnRename(name);
        }

        /// <summary>
        /// Deletes this track.
        /// </summary>
        public void Delete()
        {
            _timeline.Delete(this);
        }

        /// <summary>
        /// Expand track.
        /// </summary>
        public void Expand()
        {
            ExpandAllParents();

            if (_opened)
                return;

            _opened = true;

            OnExpandedChanged();
        }

        /// <summary>
        /// Collapse track.
        /// </summary>
        public void Collapse()
        {
            if (!_opened)
                return;

            _opened = false;

            OnExpandedChanged();
        }

        /// <summary>
        /// Expand track and all the children.
        /// </summary>
        public void ExpandAll()
        {
            bool wasLayoutLocked = IsLayoutLocked;
            IsLayoutLocked = true;

            Expand();

            for (int i = 0; i < SubTracks.Count; i++)
            {
                if (SubTracks[i].CanExpand)
                    SubTracks[i].ExpandAll();
            }

            IsLayoutLocked = wasLayoutLocked;
            PerformLayout();
        }

        /// <summary>
        /// Collapse track and all the children.
        /// </summary>
        public void CollapseAll()
        {
            bool wasLayoutLocked = IsLayoutLocked;
            IsLayoutLocked = true;

            Collapse();

            for (int i = 0; i < SubTracks.Count; i++)
            {
                if (SubTracks[i].CanExpand)
                    SubTracks[i].CollapseAll();
            }

            IsLayoutLocked = wasLayoutLocked;
            PerformLayout();
        }

        /// <summary>
        /// Ensure that all track parents are expanded.
        /// </summary>
        public void ExpandAllParents()
        {
            _parentTrack?.Expand();
        }

        /// <inheritdoc />
        public override void Draw()
        {
            // Cache data
            var style = Style.Current;
            bool isSelected = _timeline.SelectedTracks.Contains(this);
            bool isFocused = _timeline.ContainsFocus;
            var left = _xOffset + 16; // offset + arrow
            var height = HeaderHeight;
            var bounds = new Rectangle(Float2.Zero, Size);
            var textRect = new Rectangle(left, 0, bounds.Width - left, height);
            _margin.ShrinkRectangle(ref textRect);
            var textColor = style.Foreground * TitleTintColor;
            var backgroundColorSelected = style.BackgroundSelected;
            var backgroundColorHighlighted = style.BackgroundHighlighted;
            var backgroundColorSelectedUnfocused = style.LightBackground;
            var isMouseOver = IsMouseOver;

            // Draw background
            if (isSelected || isMouseOver)
            {
                Render2D.FillRectangle(bounds, (isSelected && isFocused) ? backgroundColorSelected : (isMouseOver ? backgroundColorHighlighted : backgroundColorSelectedUnfocused));
            }

            // Draw arrow
            if (CanExpand)
            {
                Render2D.DrawSprite(_opened ? style.ArrowDown : style.ArrowRight, ArrowRect, isMouseOver ? style.Foreground : style.ForegroundGrey);
            }

            // Draw icon
            if (Icon.IsValid)
            {
                Render2D.DrawSprite(Icon, new Rectangle(textRect.Left, (height - 16) * 0.5f, 16, 16), IconColor);
                textRect.X += 18.0f;
                textRect.Width -= 18.0f;
            }

            // Draw text
            Render2D.DrawText(style.FontSmall, Title ?? Name, textRect, textColor, TextAlignment.Near, TextAlignment.Center);

            // Disabled overlay
            DrawDisabled = Mute || (ParentTrack != null && ParentTrack.DrawDisabled);
            if (DrawDisabled)
            {
                Render2D.FillRectangle(bounds, new Color(0, 0, 0, 100));
            }

            // Draw drag and drop effect
            if (IsDragOver && _timeline.DraggedOverTrack == this)
            {
                switch (_dragOverMode)
                {
                case DragItemPositioning.At:
                    Render2D.FillRectangle(textRect, style.Selection);
                    Render2D.DrawRectangle(textRect, style.SelectionBorder);
                    break;
                case DragItemPositioning.Above:
                    Render2D.DrawRectangle(new Rectangle(textRect.X, textRect.Top - DefaultDragInsertPositionMargin * 0.5f - DefaultNodeOffsetY - _margin.Top, textRect.Width, DefaultDragInsertPositionMargin), style.SelectionBorder);
                    break;
                case DragItemPositioning.Below:
                    Render2D.DrawRectangle(new Rectangle(textRect.X, textRect.Bottom + _margin.Bottom - DefaultDragInsertPositionMargin * 0.5f, textRect.Width, DefaultDragInsertPositionMargin), style.SelectionBorder);
                    break;
                }
            }

            base.Draw();
        }


        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            // Base
            if (base.OnMouseDown(location, button))
                return true;

            // Check if mouse hits bar and track isn't a root
            if (IsMouseOver)
            {
                // Check if left button goes down
                if (button == MouseButton.Left)
                {
                    _isMouseDown = true;
                    _mouseDownPos = location;
                }

                // Handled
                Focus();
                return true;
            }

            // Handled
            Focus();
            return true;
        }

        /// <summary>
        /// Called when showing timeline context menu to the user. Can be used to add custom buttons.
        /// </summary>
        /// <param name="menu">The menu.</param>
        /// <param name="time">The time (in seconds) at which context menu is shown (user clicked on a timeline).</param>
        public virtual void OnTimelineContextMenu(ContextMenu.ContextMenu menu, float time)
        {
        }

        /// <summary>
        /// Called when context menu is being prepared to show. Can be used to add custom options.
        /// </summary>
        /// <param name="menu">The menu.</param>
        protected virtual void OnContextMenu(ContextMenu.ContextMenu menu)
        {
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            // Base
            if (base.OnMouseUp(location, button))
                return true;

            // Check if mouse hits bar
            if (button == MouseButton.Right)
            {
                // Show context menu
                var menu = new ContextMenu.ContextMenu();
                if (CanRename)
                    menu.AddButton("Rename", "F2", StartRenaming);
                if (CanCopyPaste)
                    menu.AddButton("Duplicate", "Ctrl+D", () => Timeline.DuplicateSelectedTracks());
                menu.AddButton("Delete", "Del", Delete);
                if (CanExpand)
                {
                    menu.AddSeparator();
                    menu.AddButton("Expand All", ExpandAll);
                    menu.AddButton("Collapse All", CollapseAll);
                }
                if (SubTracks.Count > 1)
                {
                    menu.AddButton("Sort All", () => _timeline.SortTrack(this, _subTracks.Sort));
                }
                OnContextMenu(menu);
                menu.Show(this, location);
            }
            else if (button == MouseButton.Left)
            {
                // Clear flag
                _isMouseDown = false;
            }

            // Prevent from selecting track when user is just clicking at an arrow
            if (!_mouseOverArrow)
            {
                var window = Root;
                if (window.GetKey(KeyboardKeys.Control))
                {
                    // Add/Remove
                    if (_timeline.SelectedTracks.Contains(this))
                        _timeline.Deselect(this);
                    else
                        _timeline.Select(this, true);
                }
                else
                {
                    // Select
                    _timeline.Select(this, false);
                }
            }

            // Check if mouse hits arrow
            if (CanExpand && _mouseOverArrow)
            {
                // Toggle open state
                if (_opened)
                    Collapse();
                else
                    Expand();
            }

            // Handled
            Focus();
            return true;
        }

        /// <inheritdoc />
        public override void OnMouseMove(Float2 location)
        {
            base.OnMouseMove(location);

            // Cache flag
            _mouseOverArrow = CanExpand && ArrowRect.Contains(location);

            // Check if start drag and drop
            if (_isMouseDown && Float2.Distance(_mouseDownPos, location) > 10.0f)
            {
                // Clear flag
                _isMouseDown = false;

                // Start
                DoDragDrop();
            }
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            base.OnMouseLeave();

            // Clear flags
            _mouseOverArrow = false;

            // Check if start drag and drop
            if (_isMouseDown)
            {
                // Clear flag
                _isMouseDown = false;

                // Start
                DoDragDrop();
            }

            // Hack fix for drag problems
            if (_isDragOverHeader)
            {
                _dragOverMode = DragItemPositioning.None;
                _isDragOverHeader = false;
                OnDragLeave();
            }
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragEnter(ref Float2 location, DragData data)
        {
            var result = base.OnDragEnter(ref location, data);

            // Check if no children handled that event
            _dragOverMode = DragItemPositioning.None;
            if (result == DragDropEffect.None)
            {
                UpdateDragPositioning(ref location);

                // Check if mouse is over header
                _isDragOverHeader = TestHeaderHit(ref location);
                if (_isDragOverHeader)
                {
                    if (Timeline != null)
                        Timeline.DraggedOverTrack = this;

                    // Expand node if mouse goes over arrow
                    if (_children.Count > 0 && ArrowRect.Contains(location))
                        Expand();

                    result = OnDragEnterHeader(data);
                }

                if (result == DragDropEffect.None)
                    _dragOverMode = DragItemPositioning.None;
            }

            return result;
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragMove(ref Float2 location, DragData data)
        {
            var result = base.OnDragMove(ref location, data);

            // Check if no children handled that event
            ClearDragPositioning();
            if (result == DragDropEffect.None)
            {
                UpdateDragPositioning(ref location);

                // Check if mouse is over header
                bool isDragOverHeader = TestHeaderHit(ref location);
                if (isDragOverHeader)
                {
                    // Expand node if mouse goes over arrow
                    if (_children.Count > 0 && ArrowRect.Contains(location))
                        Expand();

                    if (!_isDragOverHeader)
                        result = OnDragEnterHeader(data);
                    else
                        result = OnDragMoveHeader(data);
                }
                else if (_isDragOverHeader)
                {
                    OnDragLeaveHeader();
                }
                _isDragOverHeader = isDragOverHeader;

                if (result == DragDropEffect.None)
                    _dragOverMode = DragItemPositioning.None;
            }

            return result;
        }

        /// <inheritdoc />
        public override DragDropEffect OnDragDrop(ref Float2 location, DragData data)
        {
            var result = base.OnDragDrop(ref location, data);

            // Check if no children handled that event
            if (result == DragDropEffect.None)
            {
                UpdateDragPositioning(ref location);

                // Check if mouse is over header
                if (TestHeaderHit(ref location))
                {
                    result = OnDragDropHeader(data);
                }
            }

            // Clear cache
            _isDragOverHeader = false;
            ClearDragPositioning();

            return result;
        }

        /// <inheritdoc />
        public override void OnDragLeave()
        {
            // Clear cache
            if (_isDragOverHeader)
            {
                _isDragOverHeader = false;
                OnDragLeaveHeader();
            }
            ClearDragPositioning();

            base.OnDragLeave();
        }

        /// <inheritdoc />
        public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            if (base.OnMouseDoubleClick(location, button))
                return true;

            if (CanRename && TestHeaderHit(ref location))
            {
                StartRenaming();
                return true;
            }

            return false;
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            if (IsFocused)
            {
                Track toSelect = null;
                switch (key)
                {
                case KeyboardKeys.F2:
                    if (CanRename)
                        StartRenaming();
                    return true;
                case KeyboardKeys.Delete:
                    _timeline.DeleteSelectedTracks();
                    return true;
                case KeyboardKeys.D:
                    if (Root.GetKey(KeyboardKeys.Control) && CanCopyPaste)
                    {
                        _timeline.DuplicateSelectedTracks();
                        return true;
                    }
                    break;
                case KeyboardKeys.ArrowLeft:
                    if (CanExpand && IsExpanded)
                    {
                        Collapse();
                        return true;
                    }
                    else
                    {
                        toSelect = ParentTrack;
                    }
                    break;
                case KeyboardKeys.ArrowRight:
                    if (CanExpand)
                    {
                        if (IsExpanded && HasSubTracks)
                        {
                            toSelect = SubTracks[0];
                        }
                        else
                        {
                            Expand();
                            return true;
                        }
                    }
                    break;
                case KeyboardKeys.ArrowUp:
                {
                    int index = IndexInParent;
                    while (index != 0)
                    {
                        toSelect = Parent.GetChild(--index) as Track;
                        if (toSelect != null && toSelect.HasParentsExpanded)
                            break;
                        toSelect = null;
                    }
                    break;
                }
                case KeyboardKeys.ArrowDown:
                {
                    int index = IndexInParent;
                    while (index < Parent.ChildrenCount - 1)
                    {
                        toSelect = Parent.GetChild(++index) as Track;
                        if (toSelect != null && toSelect.HasParentsExpanded)
                            break;
                        toSelect = null;
                    }
                    break;
                }
                }
                if (toSelect != null)
                {
                    Timeline.Select(toSelect);
                    ((Panel)Timeline.TracksPanel.Parent).ScrollViewTo(toSelect);
                    toSelect.Focus();
                    return true;
                }
            }

            // Base
            if (_opened)
                return base.OnKeyDown(key);
            return false;
        }

        /// <inheritdoc />
        public override void OnKeyUp(KeyboardKeys key)
        {
            // Base
            if (_opened)
                base.OnKeyUp(key);
        }

        /// <inheritdoc />
        public override int Compare(Control other)
        {
            if (other is Track otherTrack)
            {
                var name = Title ?? Name;
                var otherName = otherTrack.Title ?? otherTrack.Name;
                return string.Compare(name, otherName, StringComparison.InvariantCulture);
            }
            return base.Compare(other);
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            // Cleanup
            Archetype = new TrackArchetype();
            MediaChanged = null;
            _timeline = null;
            _muteCheckbox = null;

            base.OnDestroy();
        }

        /// <summary>
        /// Loads the name using UTF8 encoding by reading name length (as 32bit int) followed by the contents and null-terminated character.
        /// </summary>
        /// <param name="stream">The input stream.</param>
        /// <returns>The name.</returns>
        protected static string LoadName(BinaryReader stream)
        {
            var length = stream.ReadInt32();
            var data = stream.ReadBytes(length);
            var value = Encoding.UTF8.GetString(data, 0, length);
            if (stream.ReadChar() != 0)
                throw new Exception("Invalid track data.");
            return value;
        }

        /// <summary>
        /// Saves the name using UTF8 encoding by writing name length (as 32bit int) followed by the contents and null-terminated character.
        /// </summary>
        /// <param name="stream">The output stream.</param>
        /// <param name="value">The value to write (can be null).</param>
        protected static void SaveName(BinaryWriter stream, string value)
        {
            value = value ?? string.Empty;
            var data = Encoding.UTF8.GetBytes(value);
            if (data.Length != value.Length)
                throw new Exception(string.Format("The name bytes data has different size as UTF8 bytes. Type {0}.", value));

            stream.Write(data.Length);
            stream.Write(data);
            stream.Write('\0');
        }
    }
}
