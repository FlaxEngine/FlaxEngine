using System;

namespace FlaxEngine
{
    partial class BoxCollider
    {
        /// <inheritdoc />
        public override void OnActorSpawned()
        {
            base.OnActorSpawned();
            Vector3 parentScale = Parent.Scale;
            Vector3 parentSize = Parent.Box.Size;
            Vector3 parentCenter = Parent.Box.Center - Parent.Position;

            Size = parentSize / parentScale;
            Center = parentCenter / parentScale;

            // Undo Rotation
            Orientation *= Quaternion.Invert(Orientation);
        }
    }
}
