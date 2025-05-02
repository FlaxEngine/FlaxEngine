// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.SceneGraph.Actors
{
    /// <summary>
    /// Scene tree node for <see cref="Joint"/> actor type.
    /// </summary>
    [HideInEditor]
    public sealed class JointNode : ActorNode
    {
        /// <inheritdoc />
        public JointNode(Actor actor)
        : base(actor)
        {
        }

        /// <inheritdoc />
        public override void PostConvert(ActorNode source)
        {
            base.PostConvert(source);

            if (source.Actor is Joint other)
            {
                // Preserve basic properties when changing joint type
                var joint = (Joint)Actor;
                joint.Target = other.Target;
                joint.TargetAnchor = other.TargetAnchor;
                joint.TargetAnchorRotation = other.TargetAnchorRotation;
                joint.EnableCollision = other.EnableCollision;
            }
        }
    }
}
