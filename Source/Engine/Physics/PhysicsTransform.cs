namespace FlaxEngine
{
    /// <summary>
    /// Describes transformation in a 3D space.
    /// </summary>
    public partial struct PhysicsTransform
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="PhysicsTransform" /> struct.
        /// </summary>
        public PhysicsTransform()
        {
            Translation = Vector3.Zero;
            Orientation = Quaternion.Identity;
        }
        /// <summary>
        /// Initializes a new instance of the <see cref="PhysicsTransform"/> struct.
        /// </summary>
        /// <param name="InTranslation">The in translation.</param>
        public PhysicsTransform(Vector3 InTranslation)
        {
            Translation = InTranslation;
            Orientation = Quaternion.Identity;
        }
        /// <summary>
        /// Initializes a new instance of the <see cref="PhysicsTransform"/> struct.
        /// </summary>
        /// <param name="InTranslation">The in translation.</param>
        /// <param name="InOrientation">The in orientation.</param>
        public PhysicsTransform(Vector3 InTranslation, Quaternion InOrientation)
        {
            Translation = InTranslation;
            Orientation = InOrientation;
        }
        /// <summary>
        /// Initializes a new instance of the <see cref="PhysicsTransform"/> struct.
        /// </summary>
        /// <param name="InTransform">The in transform.</param>
        public PhysicsTransform(Transform InTransform)
        {
            Translation = InTransform.Translation;
            Orientation = InTransform.Orientation;
        }

        /// <summary>
        /// World PhysicsTransform to local PhysicsTransform.
        /// </summary>
        /// <param name="InWorld">The in world.</param>
        /// <param name="InOtherWorld">The in other world.</param>
        /// <returns>The local PhysicsTransform</returns>
        public static PhysicsTransform WorldToLocal(PhysicsTransform InWorld, PhysicsTransform InOtherWorld)
        {
            var inv = InWorld.Orientation.Conjugated();
            var T = (InOtherWorld.Translation - InWorld.Translation) * inv;
            var Q = inv * InOtherWorld.Orientation;
            return new PhysicsTransform(T, Q);
        }

        /// <summary>
        /// Local PhysicsTransform to world PhysicsTransform.
        /// </summary>
        /// <param name="InWorld">The in world.</param>
        /// <param name="InLocal">The in local.</param>
        /// <returns>The world PhysicsTransform</returns>
        public static PhysicsTransform LocalToWorld(PhysicsTransform InWorld, PhysicsTransform InLocal)
        {
            var T = (InLocal.Translation * InWorld.Orientation) + InWorld.Translation;
            var Q = InWorld.Orientation * InLocal.Orientation;
            return new PhysicsTransform(T, Q);
        }

        /// <summary>
        /// Converts to transform.
        /// </summary>
        /// <param name="InScale">The in scale.</param>
        /// <returns>The transform with the new scale.</returns>
        public readonly Transform ToTransform(Float3 InScale)
        {
            return new Transform(Translation, Orientation, InScale);
        }
    }
}
