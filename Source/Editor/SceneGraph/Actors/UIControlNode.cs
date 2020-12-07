// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

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
    }
}
