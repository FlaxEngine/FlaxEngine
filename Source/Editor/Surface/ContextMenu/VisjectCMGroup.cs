// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

using System;
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
        /// The archetype.
        /// </summary>
        public readonly GroupArchetype Archetype;

        /// <summary>
        /// A computed score for the context menu order.
        /// </summary>
        public float SortScore;

        /// <summary>
        /// Initializes a new instance of the <see cref="VisjectCMGroup"/> class.
        /// </summary>
        /// <param name="cm">The context menu.</param>
        /// <param name="archetype">The group archetype.</param>
        public VisjectCMGroup(VisjectCM cm, GroupArchetype archetype)
        {
            ContextMenu = cm;
            Archetype = archetype;
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
                    item.UpdateFilter(null);
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
        public void UpdateFilter(string filterText)
        {
            Profiler.BeginEvent("VisjectCMGroup.UpdateFilter");

            // Update items
            bool isAnyVisible = false;
            for (int i = 0; i < _children.Count; i++)
            {
                if (_children[i] is VisjectCMItem item)
                {
                    item.UpdateFilter(filterText);
                    isAnyVisible |= item.Visible;
                }
            }

            // Update header title
            if (QueryFilterHelper.Match(filterText, HeaderText))
            {
                for (int i = 0; i < _children.Count; i++)
                {
                    if (_children[i] is VisjectCMItem item)
                    {
                        item.Visible = true;
                    }
                }
                isAnyVisible = true;
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
                    order = string.Compare(Archetype.Name, otherGroup.Archetype.Name, StringComparison.InvariantCulture);
                }
                return order;
            }
            return base.Compare(other);
        }
    }
}
