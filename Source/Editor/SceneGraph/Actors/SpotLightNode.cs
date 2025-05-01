// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.SceneGraph.Actors
{
    /// <summary>
    /// Scene tree node for <see cref="SpotLight"/> actor type.
    /// </summary>
    /// <seealso cref="ActorNodeWithIcon" />
    [HideInEditor]
    public sealed class SpotLightNode : ActorNodeWithIcon
    {
        /// <inheritdoc />
        public SpotLightNode(Actor actor)
        : base(actor)
        {
        }

        /// <inheritdoc />
        public override void OnDebugDraw(ViewportDebugDrawData data)
        {
            base.OnDebugDraw(data);

            var transform = Actor.Transform;
            DebugDraw.DrawWireArrow(transform.Translation, transform.Orientation, 0.3f, 0.15f, Color.Red, 0.0f, false);
        }
    }
}
