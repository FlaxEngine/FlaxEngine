// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.SceneGraph.Actors
{
    /// <summary>
    /// Scene tree node for <see cref="SpriteRender"/> actor type.
    /// </summary>
    /// <seealso cref="ActorNode" />
    [HideInEditor]
    public sealed class SpriteRenderNode : ActorNode
    {
        /// <inheritdoc />
        public SpriteRenderNode(Actor actor)
        : base(actor)
        {
        }

        /// <inheritdoc />
        public override bool RayCastSelf(ref RayCastData ray, out float distance, out Vector3 normal)
        {
            SpriteRender sprite = (SpriteRender)Actor;
            Vector3 viewPosition = ray.View.Position;
            Vector3 viewDirection = ray.View.Direction;
            Matrix m1, m2, m3, world;
            var size = sprite.Size;
            Matrix.Scaling(size.X, size.Y, 1.0f, out m1);
            var transform = sprite.Transform;
            if (sprite.FaceCamera)
            {
                var up = Vector3.Up;
                Matrix.Billboard(ref transform.Translation, ref viewPosition, ref up, ref viewDirection, out m2);
                Matrix.Multiply(ref m1, ref m2, out m3);
                Matrix.Scaling(ref transform.Scale, out m1);
                Matrix.Multiply(ref m1, ref m3, out world);
            }
            else
            {
                transform.GetWorld(out m2);
                Matrix.Multiply(ref m1, ref m2, out world);
            }

            OrientedBoundingBox bounds;
            bounds.Extents = Vector3.Half;
            bounds.Transformation = world;

            normal = -ray.Ray.Direction;
            return bounds.Intersects(ref ray.Ray, out distance);
        }

        /// <inheritdoc />
        public override void PostSpawn()
        {
            base.PostSpawn();

            // Setup for default values
            var text = (SpriteRender)Actor;
            text.Material = FlaxEngine.Content.LoadAsyncInternal<MaterialBase>(EditorAssets.DefaultSpriteMaterial);
            text.Image = FlaxEngine.Content.LoadAsyncInternal<Texture>(EditorAssets.FlaxIconTexture);
        }
    }
}
