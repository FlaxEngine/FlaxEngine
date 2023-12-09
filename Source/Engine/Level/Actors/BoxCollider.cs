using System;

namespace FlaxEngine
{
    partial class BoxCollider
    {
        /// <inheritdoc />
        public override void OnActorSpawned()
        {
            base.OnActorSpawned();
            if (Parent is StaticModel model)
            {
                Vector3 modelScale = model.Scale;
                Vector3 modelSize = model.Box.Size;
                Vector3 modelCenter = model.Box.Center - model.Position;
                Vector3 colliderSize = modelSize / modelScale;
                Vector3 colliderCenter = modelCenter / modelScale;

                Size = colliderSize;
                Center = colliderCenter;

                // Undo Rotation
                Orientation *= Quaternion.Invert(Orientation);
            }
        }
    }
}
