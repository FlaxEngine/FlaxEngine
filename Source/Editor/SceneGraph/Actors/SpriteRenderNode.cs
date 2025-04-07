// Copyright (c) Wojciech Figat. All rights reserved.

#if USE_LARGE_WORLDS
using Real = System.Double;
#else
using Real = System.Single;
#endif

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
        public override bool RayCastSelf(ref RayCastData ray, out Real distance, out Vector3 normal)
        {
            SpriteRender sprite = (SpriteRender)Actor;
            Float3 viewPosition = (Float3)ray.View.Position; // TODO: large-worlds
            Float3 viewDirection = ray.View.Direction;
            Matrix m1, m2, m3, world;
            var size = sprite.Size;
            Matrix.Scaling(size.X, size.Y, 1.0f, out m1);
            var transform = sprite.Transform;
            if (sprite.FaceCamera)
            {
                var up = Float3.Up;
                Float3 translation = transform.Translation;
                Matrix.Billboard(ref translation, ref viewPosition, ref up, ref viewDirection, out m2);
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
            world.Decompose(out bounds.Transformation);

            normal = -ray.Ray.Direction;
            return bounds.Intersects(ref ray.Ray, out distance);
        }

        /// <inheritdoc />
        public override void PostSpawn()
        {
            base.PostSpawn();

            if (Actor.HasPrefabLink)
            {
                return;
            }

            // Setup for default values
            var sprite = (SpriteRender)Actor;
            sprite.Material = FlaxEngine.Content.LoadAsyncInternal<MaterialBase>(EditorAssets.DefaultSpriteMaterial);
            sprite.Image = FlaxEngine.Content.LoadAsyncInternal<Texture>(EditorAssets.FlaxIconTexture);
        }
    }
}
