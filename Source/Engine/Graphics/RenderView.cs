// Copyright (c) Wojciech Figat. All rights reserved.

namespace FlaxEngine
{
    partial struct RenderView
    {
        /// <summary>
        /// Initializes this view with default options.
        /// </summary>
        public void Init()
        {
            MaxShadowsQuality = Quality.Ultra;
            ModelLODDistanceFactor = 1.0f;
            ModelLODDistanceFactorSqrt = 1.0f;
#pragma warning disable 0612
            ShadowModelLODDistanceFactor = 1.0f;
#pragma warning restore 0612
            Flags = ViewFlags.DefaultGame;
            Mode = ViewMode.Default;
        }

        /// <summary>
        /// Updates the cached data for the view (inverse matrices, etc.).
        /// </summary>
        public void UpdateCachedData()
        {
            Matrix.Invert(ref View, out IV);
            Matrix.Invert(ref Projection, out IP);
            Matrix.Multiply(ref View, ref Projection, out var viewProjection);
            Frustum = new BoundingFrustum(ref viewProjection);
            Matrix.Invert(ref viewProjection, out IVP);
            CullingFrustum = Frustum;
            NonJitteredProjection = Projection;
        }

        /// <summary>
        /// Initializes render view data.
        /// </summary>
        /// <param name="view">The view.</param>
        /// <param name="projection">The projection.</param>
        public void SetUp(ref Matrix view, ref Matrix projection)
        {
            Projection = projection;
            NonJitteredProjection = projection;
            View = view;

            UpdateCachedData();
        }

        /// <summary>
        /// Set up view for projector rendering.
        /// </summary>
        /// <param name="nearPlane">Near plane</param>
        /// <param name="farPlane">Far plane</param>
        /// <param name="position">Camera's position</param>
        /// <param name="direction">Camera's direction vector</param>
        /// <param name="up">Camera's up vector</param>
        /// <param name="angle">Camera's FOV angle (in degrees)</param>
        public void SetProjector(float nearPlane, float farPlane, Float3 position, Float3 direction, Float3 up, float angle)
        {
            // Copy data
            Near = nearPlane;
            Far = farPlane;
            Position = position;

            // Create projection matrix
            Matrix.PerspectiveFov(angle * Mathf.DegreesToRadians, 1.0f, nearPlane, farPlane, out Projection);
            NonJitteredProjection = Projection;
            TemporalAAJitter = Float4.Zero;

            // Create view matrix
            Direction = direction;
            var target = Position + Direction;
            Matrix.LookAt(ref Position, ref target, ref up, out View);

            UpdateCachedData();
        }

        /// <summary>
        /// Copies render view data from the camera.
        /// </summary>
        /// <param name="camera">The camera.</param>
        public void CopyFrom(Camera camera)
        {
            var viewport = camera.Viewport;
            CopyFrom(camera, ref viewport);
        }

        /// <summary>
        /// Copies render view data from the camera.
        /// </summary>
        /// <param name="camera">The camera.</param>
        /// <param name="viewport">The custom viewport to use for view/projeection matrices override.</param>
        public void CopyFrom(Camera camera, ref Viewport viewport)
        {
            Vector3 cameraPos = camera.Position;
            LargeWorlds.UpdateOrigin(ref Origin, cameraPos);
            Position = cameraPos - Origin;
            Direction = camera.Direction;
            Near = camera.NearPlane;
            Far = camera.FarPlane;
            camera.GetMatrices(out View, out Projection, ref viewport, ref Origin);
            NonJitteredProjection = Projection;
            TemporalAAJitter = Float4.Zero;
            RenderLayersMask = camera.RenderLayersMask;
            Flags = camera.RenderFlags;
            Mode = camera.RenderMode;

            UpdateCachedData();
        }

        /// <summary>
        /// Calculates the world matrix for the given transformation instance rendering.
        /// </summary>
        /// <param name="transform">The object transformation.</param>
        /// <param name="world">The output matrix.</param>
        public void GetWorldMatrix(ref Transform transform, out Matrix world)
        {
            Float3 translation = transform.Translation - Origin;
            Matrix.Transformation(ref transform.Scale, ref transform.Orientation, ref translation, out world);
        }
    }
}
