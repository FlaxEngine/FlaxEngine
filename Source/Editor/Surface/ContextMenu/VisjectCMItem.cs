// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using FlaxEditor.Scripting;
using FlaxEditor.Surface.Elements;
using FlaxEditor.Utilities;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Surface.ContextMenu
{
    /// <summary>
    /// Single item used for <see cref="VisjectCM"/>. Represents type of the <see cref="NodeArchetype"/> that can be spawned.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.Control" />
    [HideInEditor]
    public sealed class VisjectCMItem : Control
    {
        private bool _isMouseDown;
        private List<Rectangle> _highlights;
        private GroupArchetype _groupArchetype;
        private NodeArchetype _archetype;
        private bool _isStartsWithMatch;
        private bool _isFullMatch;

        /// <summary>
        /// Gets the item's group
        /// </summary>
        public VisjectCMGroup Group { get; }

        /// <summary>
        /// Gets the group archetype.
        /// </summary>
        public GroupArchetype GroupArchetype => _groupArchetype;

        /// <summary>
        /// Gets the node archetype.
        /// </summary>
        public NodeArchetype NodeArchetype => _archetype;

        /// <summary>
        /// Gets and sets the data of the node.
        /// </summary>
        public object[] Data;

        /// <summary>
        /// A computed score for the context menu order
        /// </summary>
        public float SortScore;

        /// <summary>
        /// Initializes a new instance of the <see cref="VisjectCMItem"/> class.
        /// </summary>
        /// <param name="group">The group.</param>
        /// <param name="groupArchetype">The group archetype.</param>
        /// <param name="archetype">The archetype.</param>
        public VisjectCMItem(VisjectCMGroup group, GroupArchetype groupArchetype, NodeArchetype archetype)
        : base(0, 0, 120, 14)
        {
            Group = group;
            _groupArchetype = groupArchetype;
            _archetype = archetype;
            TooltipText = GetTooltip();
        }

        /// <summary>
        /// Updates the <see cref="SortScore"/>
        /// </summary>
        /// <param name="selectedBox">The currently user-selected box</param>
        public void UpdateScore(Box selectedBox)
        {
            SortScore = 0;

            if (!Visible)
                return;

            SortScore += _archetype.SortScore;
            if (selectedBox != null && CanConnectTo(selectedBox))
                SortScore += 1;
            if (Data != null)
                SortScore += 1;
            if (_highlights is { Count: > 0 })
                SortScore += 1;
            if (_isStartsWithMatch)
                SortScore += 2;
            if (_isFullMatch)
                SortScore += 5;
        }

        private void GetTextRectangle(out Rectangle textRect)
        {
            textRect = new Rectangle(22, 0, Width - 24, Height);
        }

        /// <summary>
        /// Checks if this context menu item can be connected to a given box, before a node is actually spawned.
        /// </summary>
        /// <param name="startBox">The connected box</param>
        /// <returns>True if the connected box is compatible with this item</returns>
        public bool CanConnectTo(Box startBox)
        {
            // Is compatible if box is null for reset reasons
            if (startBox == null)
                return true;

            if (_archetype == null)
                return false;

            bool isCompatible = false;
            if (startBox.IsOutput && _archetype.IsInputCompatible != null)
            {
                isCompatible |= _archetype.IsInputCompatible.Invoke(_archetype, startBox.CurrentType, _archetype.ConnectionsHints, startBox.ParentNode.Context);
            }
            else if (!startBox.IsOutput && _archetype.IsOutputCompatible != null)
            {
                isCompatible |= _archetype.IsOutputCompatible.Invoke(_archetype, startBox.CurrentType, startBox.ParentNode.Archetype.ConnectionsHints, startBox.ParentNode.Context);
            }
            else if (_archetype.Elements != null)
            {
                // Check compatibility based on the defined elements in the archetype. This handles all the default groups and items
                isCompatible = CheckElementsCompatibility(startBox);
            }

            return isCompatible;
        }

        private bool CheckElementsCompatibility(Box startBox)
        {
            bool isCompatible = false;
            foreach (NodeElementArchetype element in _archetype.Elements)
            {
                // Ignore all elements that aren't inputs or outputs (e.g. input fields)
                if (element.Type != NodeElementType.Output && element.Type != NodeElementType.Input)
                    continue;

                // Ignore elements with the same direction as the box
                if ((startBox.IsOutput && element.Type == NodeElementType.Output) || (!startBox.IsOutput && element.Type == NodeElementType.Input))
                    continue;

                ScriptType fromType;
                ScriptType toType;
                ConnectionsHint hint;
                if (startBox.IsOutput)
                {
                    fromType = element.ConnectionsType;
                    toType = startBox.CurrentType;
                    hint = _archetype.ConnectionsHints;
                }
                else
                {
                    fromType = startBox.CurrentType;
                    toType = element.ConnectionsType;
                    hint = startBox.ParentNode.Archetype.ConnectionsHints;
                }

                isCompatible |= VisjectSurface.FullCastCheck(fromType, toType, hint);
            }

            return isCompatible;
        }

        /// <summary>
        /// Updates the filter.
        /// </summary>
        /// <param name="filterText">The filter text.</param>
        /// <param name="selectedBox">The optionally selected box to show hints for it.</param>
        /// <param name="groupHeaderMatches">True if item's group header got a filter match and item should stay visible.</param>
        public void UpdateFilter(string filterText, Box selectedBox, bool groupHeaderMatches = false)
        {
            // When dragging connection out of a box, validate if the box is compatible with this item's type
            if (selectedBox != null)
            {
                Visible = CanConnectTo(selectedBox);
                if (!Visible)
                {
                    _highlights?.Clear();
                    return;
                }
            }

            _isStartsWithMatch = _isFullMatch = false;
            if (string.IsNullOrEmpty(filterText))
            {
                // Clear filter
                _highlights?.Clear();
                Visible = true;
                return;
            }

            GetTextRectangle(out var textRect);

            // Check archetype title
            if (QueryFilterHelper.Match(filterText, _archetype.Title, out var ranges))
            {
                // Update highlights
                if (_highlights == null)
                    _highlights = new List<Rectangle>(ranges.Length);
                else
                    _highlights.Clear();
                var style = Style.Current;
                var font = style.FontSmall;
                for (int i = 0; i < ranges.Length; i++)
                {
                    var start = font.GetCharPosition(_archetype.Title, ranges[i].StartIndex);
                    var end = font.GetCharPosition(_archetype.Title, ranges[i].EndIndex);
                    _highlights.Add(new Rectangle(start.X + textRect.X, 0, end.X - start.X, Height));

                    if (ranges[i].StartIndex <= 0)
                    {
                        _isStartsWithMatch = true;
                        if (ranges[i].Length == _archetype.Title.Length)
                            _isFullMatch = true;
                    }
                }
                Visible = true;
                return;
            }

            // Check archetype synonyms
            if (_archetype.AlternativeTitles != null && _archetype.AlternativeTitles.Any(altTitle => QueryFilterHelper.Match(filterText, altTitle, out ranges)))
            {
                // Update highlights
                if (_highlights == null)
                    _highlights = new List<Rectangle>(1);
                else
                    _highlights.Clear();
                var style = Style.Current;
                var font = style.FontSmall;
                var start = font.GetCharPosition(_archetype.Title, 0);
                var end = font.GetCharPosition(_archetype.Title, _archetype.Title.Length - 1);
                _highlights.Add(new Rectangle(start.X + textRect.X, 0, end.X - start.X, Height));

                for (int i = 0; i < ranges.Length; i++)
                {
                    if (ranges[i].StartIndex <= 0)
                    {
                        _isStartsWithMatch = true;
                    }
                }

                Visible = true;
                return;
            }

            // Check archetype data (if it exists)
            if (NodeArchetype.TryParseText != null && NodeArchetype.TryParseText(filterText, out var data))
            {
                // Update highlights
                if (_highlights == null)
                    _highlights = new List<Rectangle>(1);
                else
                    _highlights.Clear();
                var style = Style.Current;
                var font = style.FontSmall;
                var start = font.GetCharPosition(_archetype.Title, 0);
                var end = font.GetCharPosition(_archetype.Title, _archetype.Title.Length - 1);
                _highlights.Add(new Rectangle(start.X + textRect.X, 0, end.X - start.X, Height));
                Visible = true;
                Data = data;
                return;
            }

            _highlights?.Clear();

            // Hide
            if (!groupHeaderMatches)
                Visible = false;
        }

        /// <inheritdoc />
        public override void Draw()
        {
            var style = Style.Current;
            var rect = new Rectangle(Float2.Zero, Size);
            GetTextRectangle(out var textRect);
            var showScoreHit = SortScore > 0.1f;

            // Overlay
            if (IsMouseOver)
                Render2D.FillRectangle(rect, style.BackgroundHighlighted);

            if (Group.ContextMenu.SelectedItem == this)
                Render2D.FillRectangle(rect, style.BackgroundSelected);

            // Shift text with highlights if using score mark
            if (showScoreHit)
            {
                Render2D.PushTransform(Matrix3x3.Translation2D(12, 0));
            }

            // Draw all highlights
            if (_highlights != null)
            {
                var color = style.ProgressNormal * 0.6f;
                for (int i = 0; i < _highlights.Count; i++)
                    Render2D.FillRectangle(_highlights[i], color);
            }

            // Draw name
            Render2D.DrawText(style.FontSmall, _archetype.Title, textRect, Enabled ? style.Foreground : style.ForegroundDisabled, TextAlignment.Near, TextAlignment.Center);
            if (_archetype.SubTitle != null)
            {
                var titleLength = style.FontSmall.MeasureText(_archetype.Title).X;
                var subTitleRect = new Rectangle(textRect.X + titleLength, textRect.Y, textRect.Width - titleLength, textRect.Height);
                Render2D.DrawText(style.FontSmall, _archetype.SubTitle, subTitleRect, style.ForegroundDisabled, TextAlignment.Near, TextAlignment.Center);
            }

            // Reset transform and draw score mark
            if (showScoreHit)
            {
                Render2D.PopTransform();
                Render2D.DrawText(style.FontSmall, "> ", textRect, Enabled ? style.Foreground : style.ForegroundDisabled, TextAlignment.Near, TextAlignment.Center);
            }
        }

        /// <summary>
        /// Callback when selected by the visject CM
        /// </summary>
        public void OnSelect()
        {
            Group.ContextMenu.SetDescriptionPanelArchetype(_archetype);
        }

        private string GetTooltip()
        {
            StringBuilder sb = new StringBuilder();
            sb.Append(string.IsNullOrEmpty(_archetype.Signature) ? _archetype.Title : _archetype.Signature);
            if (!string.IsNullOrEmpty(_archetype.Description))
                sb.Append("\n" + _archetype.Description);
            return sb.ToString();
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left)
            {
                _isMouseDown = true;
            }

            return base.OnMouseDown(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left && _isMouseDown)
            {
                _isMouseDown = false;
                Group.ContextMenu.OnClickItem(this);
            }

            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override void OnMouseEnter(Float2 location)
        {
            Group.ContextMenu.SetDescriptionPanelArchetype(_archetype);

            base.OnMouseEnter(location);
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            _isMouseDown = false;

            base.OnMouseLeave();
        }

        /// <inheritdoc />
        public override int Compare(Control other)
        {
            if (other is VisjectCMItem otherItem)
            {
                int order = -1 * SortScore.CompareTo(otherItem.SortScore);
                if (order == 0)
                {
                    order = string.Compare(_archetype.Title, otherItem._archetype.Title, StringComparison.Ordinal);
                }
                return order;
            }
            return base.Compare(other);
        }
    }
}
