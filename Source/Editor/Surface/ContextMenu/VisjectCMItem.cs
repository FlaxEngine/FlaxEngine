// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using FlaxEditor.Scripting;
using FlaxEditor.Surface.Elements;
using FlaxEditor.Utilities;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Utilities;

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
        : base(0, 0, 120, 12)
        {
            Group = group;
            _groupArchetype = groupArchetype;
            _archetype = archetype;
            TooltipText = _archetype.Description;
        }

        /// <summary>
        /// Updates the <see cref="SortScore"/>
        /// </summary>
        /// <param name="selectedBox">The currently user-selected box</param>
        public void UpdateScore(Box selectedBox)
        {
            SortScore = 0;

            if (!(_highlights?.Count > 0))
                return;
            if (!Visible)
                return;

            if (selectedBox != null && CanConnectTo(selectedBox))
                SortScore += 1;
            if (Data != null)
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
            {
                Visible = true;
                return true;   
            }

            if (_archetype == null)
            {
                Visible = false;
                return false;
            }

            bool isCompatible = false;
            
            // Check compatibility based on the defined elements in the archetype. This handles all the default groups and items
            if (_archetype.Elements != null)
            {
                isCompatible = CheckElementsCompatibility(startBox);
            }

            // Check compatibility based on the archetype tag or name. This handles custom groups and items, mainly function nodes for visual scripting
            if (_archetype.NodeTypeHint == NodeTypeHint.FunctionNode)
            {
                ScriptMemberInfo memberInfo = ScriptMemberInfo.Null;

                // Check if the archetype tag already has a member info otherwise try to fetch it via the archetype type and name
                // Only really the InvokeMethod Nodes have a member info in their tag
                if (_archetype.Tag is ScriptMemberInfo info)
                {
                    memberInfo = info;
                }
                else if(_archetype.DefaultValues is { Length: > 1 })
                {
                    var eventName = (string)_archetype.DefaultValues[1];
                    var eventType = TypeUtils.GetType((string)_archetype.DefaultValues[0]);
                    memberInfo = eventType.GetMember(eventName, MemberTypes.Event, BindingFlags.Public | BindingFlags.Static | BindingFlags.Instance);
                }

                if (memberInfo != ScriptMemberInfo.Null)
                {
                    if(memberInfo.IsEvent)
                        isCompatible = false;

                    // Box was dragged from an impulse port and the member info can be invoked so it is compatible
                    if (startBox.CurrentType.IsVoid && memberInfo.ValueType.IsVoid)
                    {
                        isCompatible = true;   
                    }
                    else
                    {
                        // When the startBox is output we only need to check the input parameters
                        if (startBox.IsOutput)
                        {
                            var parameters = memberInfo.GetParameters();
                            ScriptType outType = startBox.CurrentType;
                            
                            // non static members have an instance input parameter
                            if (!memberInfo.IsStatic)
                            {
                                var scriptType = memberInfo.DeclaringType;
                                isCompatible |= CanCastToType(scriptType, outType, _archetype.ConnectionsHints);
                            }

                            // We ignore event members here since they only have output parameters, which are currently not declared as such
                            // TODO: Fix bug where event member parameters 'IsOut' is set to false and not true
                            if (!memberInfo.IsEvent)
                            {
                                for (int i = 0; i < parameters.Length; i++)
                                {
                                    ScriptType inType = parameters[i].Type;
                                    isCompatible |= CanCastToType(inType, outType, _archetype.ConnectionsHints);
                                }
                            }
                        }
                        else
                        {
                            // When the startBox is input we only have to check the output type of the method
                            ScriptType inType = startBox.CurrentType;
                            ScriptType outType = memberInfo.ValueType;

                            isCompatible |= CanCastToType(inType, outType, _archetype.ConnectionsHints);
                        }
                    }
                }
            }

            Visible = isCompatible;
            return isCompatible;
        }

        private bool CheckElementsCompatibility(Box startBox)
        {
            bool isCompatible = false;
            foreach (NodeElementArchetype element in _archetype.Elements)
            {
                if(element.Type != NodeElementType.Output && element.Type != NodeElementType.Input)
                    continue;
                
                if ((startBox.IsOutput && element.Type == NodeElementType.Output) || (!startBox.IsOutput && element.Type == NodeElementType.Input))
                    continue;
                
                ScriptType inType;
                ScriptType outType;
                ConnectionsHint hint;
                if (startBox.IsOutput)
                {
                    inType = element.ConnectionsType;
                    outType = startBox.CurrentType;
                    hint = _archetype.ConnectionsHints;
                }
                else
                {
                    inType = startBox.CurrentType;
                    outType = element.ConnectionsType;
                    hint = startBox.ParentNode.Archetype.ConnectionsHints;
                }
                
                isCompatible |= CanCastToType(inType, outType, hint);
            }

            return isCompatible;
        }
        
        private bool CanCastToType(ScriptType currentType, ScriptType type, ConnectionsHint hint)
        {
            if (VisjectSurface.CanUseDirectCastStatic(type, currentType, false))
                return true;
            
            if(VisjectSurface.IsTypeCompatible(currentType, type, hint))
                return true;
            
            return CanCast(type, currentType);
        }
        
        private static bool CanCast(ScriptType oB, ScriptType iB)
        {
            if (oB == iB)
                return true;
            if (oB == ScriptType.Null || iB == ScriptType.Null)
                return false;
            return (oB.Type != typeof(void) && oB.Type != typeof(FlaxEngine.Object)) &&
                   (iB.Type != typeof(void) && iB.Type != typeof(FlaxEngine.Object)) &&
                   oB.IsAssignableFrom(iB);
        }
        
        /// <summary>
        /// Updates the filter.
        /// </summary>
        /// <param name="filterText">The filter text.</param>
        public void UpdateFilter(string filterText, Box selectedBox)
        {
            if (selectedBox != null)
            {
                if (!CanConnectTo(selectedBox))
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
            }
            else
            {
                GetTextRectangle(out var textRect);
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
                }
                else if (_archetype.AlternativeTitles?.Any(altTitle => string.Equals(filterText, altTitle, StringComparison.CurrentCultureIgnoreCase)) == true)
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
                    _isFullMatch = true;
                    Visible = true;
                }
                else if (NodeArchetype.TryParseText != null && NodeArchetype.TryParseText(filterText, out var data))
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
                }
                else
                {
                    // Hide
                    _highlights?.Clear();
                    Visible = false;
                }
            }
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
