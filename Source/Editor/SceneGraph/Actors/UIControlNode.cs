// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.SceneGraph.Actors
{
    /// <summary>
    /// Scene tree node for <see cref="UIControl"/> actor type.
    /// </summary>
    /// <seealso cref="ActorNode" />
    [HideInEditor]
    public sealed class UIControlNode : ActorNode
    {
        /// <inheritdoc />
        public UIControlNode(Actor actor)
        : base(actor)
        {
        }

        /// <inheritdoc />
        public override void OnDebugDraw(ViewportDebugDrawData data)
        {
            base.OnDebugDraw(data);

            if (Actor is UIControl uiControl)
                DebugDraw.DrawWireBox(uiControl.Bounds, Color.BlueViolet);
        }

        /// <inheritdoc />
        public override void PostPaste()
        {
            base.PostPaste();

            var control = ((UIControl)Actor).Control;
            if (control != null)
            {
                if (control.Parent != null)
                    control.Parent.PerformLayout();
                else
                    control.PerformLayout();
            }
        }

        /// <inheritdoc />
        public override bool CanSelectInViewport
        {
            get
            {
                // Check if control and skip if canvas is 2D
                if (Actor is not UIControl uiControl)
                    return false;
                UICanvas canvas = null;
                var controlParent = uiControl.Parent;
                while (controlParent != null && controlParent is not Scene)
                {
                    if (controlParent is UICanvas uiCanvas)
                    {
                        canvas = uiCanvas;
                        break;
                    }
                    controlParent = controlParent.Parent;
                }
                if (canvas != null)
                {
                    if (canvas.Is2D)
                        return false;
                }
                return base.CanSelectInViewport;
            }
        }
    }
}
