// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEngine
{
    public partial class DirectionalLight
    {
        bool ShowCascade1 => CascadeCount >= 1 && PartitionMode == PartitionMode.Manual;
        bool ShowCascade2 => CascadeCount >= 2 && PartitionMode == PartitionMode.Manual;
        bool ShowCascade3 => CascadeCount >= 3 && PartitionMode == PartitionMode.Manual;
        bool ShowCascade4 => CascadeCount >= 4 && PartitionMode == PartitionMode.Manual;
    }
}

namespace FlaxEditor.SceneGraph.Actors
{
    /// <summary>
    /// Scene tree node for <see cref="DirectionalLight"/> actor type.
    /// </summary>
    /// <seealso cref="ActorNodeWithIcon" />
    [HideInEditor]
    public sealed class DirectionalLightNode : ActorNodeWithIcon
    {
        /// <inheritdoc />
        public DirectionalLightNode(Actor actor)
        : base(actor)
        {
        }

        /// <inheritdoc />
        public override void OnDebugDraw(ViewportDebugDrawData data)
        {
            base.OnDebugDraw(data);

            var transform = Actor.Transform;
            DebugDraw.DrawWireArrow(transform.Translation, transform.Orientation, 1.0f, Color.Red, 0.0f, false);
        }
    }
}
