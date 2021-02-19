// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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
            ShadowModelLODDistanceFactor = 1.0f;
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
            Frustum = new BoundingFrustum(viewProjection);
            Matrix.Invert(ref viewProjection, out IVP);
            CullingFrustum = Frustum;
        }

        /// <summary>
        /// Initializes render view data.
        /// </summary>
        /// <param name="view">The view.</param>
        /// <param name="projection">The projection.</param>
        public void SetUp(ref Matrix view, ref Matrix projection)
        {
            Position = view.TranslationVector;
            Projection = projection;
            NonJitteredProjection = projection;
            TemporalAAJitter = Vector4.Zero;
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
        public void SetProjector(float nearPlane, float farPlane, Vector3 position, Vector3 direction, Vector3 up, float angle)
        {
            // Copy data
            Near = nearPlane;
            Far = farPlane;
            Position = position;

            // Create projection matrix
            Matrix.PerspectiveFov(angle * Mathf.DegreesToRadians, 1.0f, nearPlane, farPlane, out Projection);
            NonJitteredProjection = Projection;
            TemporalAAJitter = Vector4.Zero;

            // Create view matrix
            Direction = direction;
            Vector3 target = Position + Direction;
            Matrix.LookAt(ref Position, ref target, ref up, out View);

            UpdateCachedData();
        }

        /// <summary>
        /// Copies render view data from the camera.
        /// </summary>
        /// <param name="camera">The camera.</param>
        public void CopyFrom(Camera camera)
        {
            Position = camera.Position;
            Direction = camera.Direction;
            Near = camera.NearPlane;
            Far = camera.FarPlane;
            View = camera.View;
            Projection = camera.Projection;
            NonJitteredProjection = Projection;
            TemporalAAJitter = Vector4.Zero;
            RenderLayersMask = camera.RenderLayersMask;

            UpdateCachedData();
        }

        /// <summary>
        /// Copies render view data from the camera.
        /// </summary>
        /// <param name="camera">The camera.</param>
        /// <param name="customViewport">The custom viewport to use for view/projeection matrices override.</param>
        public void CopyFrom(Camera camera, ref Viewport customViewport)
        {
            Position = camera.Position;
            Direction = camera.Direction;
            Near = camera.NearPlane;
            Far = camera.FarPlane;
            camera.GetMatrices(out View, out Projection, ref customViewport);
            NonJitteredProjection = Projection;
            TemporalAAJitter = Vector4.Zero;
            RenderLayersMask = camera.RenderLayersMask;

            UpdateCachedData();
        }
    }
}
