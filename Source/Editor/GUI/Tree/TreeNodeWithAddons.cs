// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;
using FlaxEngine.GUI;
using System.Collections.Generic;

namespace FlaxEditor.GUI.Tree
{
    /// <summary>
    /// Tree node control with in-built checkbox.
    /// </summary>
    [HideInEditor]
    public class TreeNodeWithAddons : TreeNode
    {
        /// <summary>
        /// The additional controls (eg. added to the header).
        /// </summary>
        public List<Control> Addons = new List<Control>();

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            foreach (var child in Addons)
            {
                Render2D.PushTransform(ref child._cachedTransform);
                child.Draw();
                Render2D.PopTransform();
            }
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            foreach (var child in Addons)
            {
                if (child.Visible && child.Enabled)
                {
                    if (IntersectsChildContent(child, location, out var childLocation))
                    {
                        if (child.OnMouseDown(childLocation, button))
                            return true;
                    }
                }
            }

            return base.OnMouseDown(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            foreach (var child in Addons)
            {
                if (child.Visible && child.Enabled)
                {
                    if (IntersectsChildContent(child, location, out var childLocation))
                    {
                        if (child.OnMouseUp(childLocation, button))
                            return true;
                    }
                }
            }

            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            foreach (var child in Addons)
            {
                if (child.Visible && child.Enabled)
                {
                    if (IntersectsChildContent(child, location, out var childLocation))
                    {
                        if (child.OnMouseDoubleClick(childLocation, button))
                            return true;
                    }
                }
            }

            return base.OnMouseDoubleClick(location, button);
        }

        /// <inheritdoc />
        public override void OnMouseMove(Float2 location)
        {
            base.OnMouseMove(location);

            if (IsCollapsed)
            {
                foreach (var child in Addons)
                {
                    if (child.Visible && child.Enabled)
                    {
                        if (IntersectsChildContent(child, location, out var childLocation))
                        {
                            if (child.IsMouseOver)
                            {
                                // Move
                                child.OnMouseMove(childLocation);
                            }
                            else
                            {
                                // Enter
                                child.OnMouseEnter(childLocation);
                            }
                        }
                        else if (child.IsMouseOver)
                        {
                            // Leave
                            child.OnMouseLeave();
                        }
                    }
                }
            }
        }
    }
}
