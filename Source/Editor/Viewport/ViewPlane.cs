namespace FlaxEditor.Viewport
{
    /// <summary>
    /// What plane axis are show
    /// </summary>
    public enum ViewPlaneAxis
    {
        /// <summary>
        /// There is no plane
        /// </summary>
        None = 0b000,
        /// <summary>
        /// Plane is on XY axis
        /// </summary>
        XY = 0b110,
        /// <summary>
        /// Plane is on XZ axis
        /// </summary>
        XZ = 0b101,
        /// <summary>
        /// Plane is on YZ axis
        /// </summary>
        YZ = 0b011
    }
}
