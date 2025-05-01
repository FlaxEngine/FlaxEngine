// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.Surface.Elements;
using FlaxEditor.Utilities;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.Surface.ContextMenu
{
    /// <summary>
    /// Drop panel for group of <see cref="VisjectCMItem"/>. It represents <see cref="GroupArchetype"/>.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.DropPanel" />
    [HideInEditor]
    public sealed class VisjectCMGroup : DropPanel
    {
        /// <summary>
        /// The context menu.
        /// </summary>
        public readonly VisjectCM ContextMenu;

        /// <summary>
        /// The archetypes (one or more).
        /// </summary>
        public readonly List<GroupArchetype> Archetypes = new List<GroupArchetype>();

        /// <summary>
        /// A computed score for the context menu order.
        /// </summary>
        public float SortScore;

        /// <summary>
        /// The group name.
        /// </summary>
        public string Name;

        /// <summary>
        /// Initializes a new instance of the <see cref="VisjectCMGroup"/> class.
        /// </summary>
        /// <param name="cm">The context menu.</param>
        /// <param name="archetype">The group archetype.</param>
        public VisjectCMGroup(VisjectCM cm, GroupArchetype archetype)
        {
            Pivot = Float2.Zero;
            ContextMenu = cm;
            Archetypes.Add(archetype);
            Name = archetype.Name;
            EnableDropDownIcon = true;
            HeaderColor = Style.Current.Background;
            ArrowImageOpened = new SpriteBrush(Style.Current.ArrowDown);
            ArrowImageClosed = new SpriteBrush(Style.Current.ArrowRight);
            CloseAnimationTime = 0;
        }

        /// <summary>
        /// Resets the view.
        /// </summary>
        public void ResetView()
        {
            Profiler.BeginEvent("VisjectCMGroup.ResetView");

            // Remove filter
            SortScore = 0;
            for (int i = 0; i < _children.Count; i++)
            {
                if (_children[i] is VisjectCMItem item)
                {
                    item.UpdateFilter(null, null);
                    item.UpdateScore(null);
                }
            }
            SortChildren();
            if (ContextMenu.ShowExpanded)
                Open(false);
            else
                Close(false);
            Visible = true;

            Profiler.EndEvent();
        }

        /// <summary>
        /// Updates the filter.
        /// </summary>
        /// <param name="filterText">The filter text.</param>
        /// <param name="selectedBox">The optionally selected box to show hints for it.</param>
        public void UpdateFilter(string filterText, Box selectedBox)
        {
            Profiler.BeginEvent("VisjectCMGroup.UpdateFilter");

            // Check if a dot is inside the filter text and split the string accordingly.
            // Everything in front of the dot is for specifying a class/group name. And everything afterward is the actual item filter text
            if (!string.IsNullOrEmpty(filterText))
            {
                int dotIndex = filterText.IndexOf('.');
                if (dotIndex != -1)
                {
                    string filterGroupName = filterText.Substring(0, dotIndex);

                    // If the string in front of the dot has the length of 0 or doesn't start with a letter we skip the filtering to make spawning floats work
                    if (filterGroupName.Length > 0 && char.IsLetter(filterGroupName[0]))
                    {
                        // Early out and make the group invisible if it doesn't start with the specified string
                        if (!Name.StartsWith(filterGroupName, StringComparison.InvariantCultureIgnoreCase))
                        {
                            Visible = false;
                            Profiler.EndEvent();
                            return;
                        }
                        filterText = filterText.Substring(dotIndex + 1);
                    }
                }
            }

            // Update items
            bool isAnyVisible = false;
            bool groupHeaderMatches = QueryFilterHelper.Match(filterText, HeaderText);
            for (int i = 0; i < _children.Count; i++)
            {
                if (_children[i] is VisjectCMItem item)
                {
                    item.UpdateFilter(filterText, selectedBox, groupHeaderMatches);
                    isAnyVisible |= item.Visible;
                }
            }

            // Update itself
            if (isAnyVisible)
            {
                if (!string.IsNullOrEmpty(filterText))
                    Open(false);
                Visible = true;
            }
            else
            {
                // Hide group if none of the items matched the filter
                Visible = false;
            }

            Profiler.EndEvent();
        }

        internal void EvaluateVisibilityWithBox(Box selectedBox)
        {
            if (selectedBox == null)
            {
                for (int i = 0; i < _children.Count; i++)
                {
                    if (_children[i] is VisjectCMItem item)
                    {
                        item.Visible = true;
                    }
                }
                Visible = true;
                return;
            }

            Profiler.BeginEvent("VisjectCMGroup.EvaluateVisibilityWithBox");

            bool isAnyVisible = false;
            for (int i = 0; i < _children.Count; i++)
            {
                if (_children[i] is VisjectCMItem item)
                {
                    item.Visible = item.CanConnectTo(selectedBox);
                    isAnyVisible |= item.Visible;
                }
            }

            // Update itself
            if (isAnyVisible)
            {
                Visible = true;
            }
            else
            {
                // Hide group if none of the items matched the filter
                Visible = false;
            }

            Profiler.EndEvent();
        }

        /// <summary>
        /// Updates the sorting of the <see cref="VisjectCMItem"/>s of this <see cref="VisjectCMGroup"/>
        /// Also updates the <see cref="SortScore"/>
        /// </summary>
        /// <param name="selectedBox">The currently user-selected box</param>
        public void UpdateItemSort(Box selectedBox)
        {
            Profiler.BeginEvent("VisjectCMGroup.UpdateItemSort");

            SortScore = 0;
            for (int i = 0; i < _children.Count; i++)
            {
                if (_children[i] is VisjectCMItem item)
                {
                    item.UpdateScore(selectedBox);
                    if (item.SortScore > SortScore)
                        SortScore = item.SortScore;
                }
            }

            if (selectedBox != null)
            {
                if (string.Equals(Name, selectedBox.CurrentType.Name, StringComparison.InvariantCultureIgnoreCase))
                    SortScore += 10;
            }

            SortChildren();

            Profiler.EndEvent();
        }

        /// <inheritdoc/>
        public override int Compare(Control other)
        {
            if (other is VisjectCMGroup otherGroup)
            {
                int order = -1 * SortScore.CompareTo(otherGroup.SortScore);
                if (order == 0)
                {
                    order = string.Compare(Name, otherGroup.Name, StringComparison.InvariantCulture);
                }
                return order;
            }
            return base.Compare(other);
        }
    }
}
