// Copyright (c) Wojciech Figat. All rights reserved.

namespace FlaxEngine
{
    partial struct SpringParameters
    {
        /// <summary>
        /// Constructs a spring.
        /// </summary>
        /// <param name="stiffness">Spring strength. Force proportional to the position error.</param>
        /// <param name="damping">Damping strength. Force proportional to the velocity error.</param>
        public SpringParameters(float stiffness, float damping)
        {
            Stiffness = stiffness;
            Damping = damping;
        }
    }

    partial struct LimitLinearRange
    {
        /// <summary>
        /// Constructs a hard limit. Once the limit is reached the movement of the attached bodies will come to a stop.
        /// </summary>
        /// <param name="lower">The lower distance of the limit. Must be less than upper.</param>
        /// <param name="upper">The upper distance of the limit. Must be more than lower.</param>
        /// <param name="contactDist">Distance from the limit at which it becomes active. Allows the solver to activate earlier than the limit is reached to avoid breaking the limit. Specify -1 for the default.</param>
        public LimitLinearRange(float lower, float upper, float contactDist = -1.0f)
        {
            ContactDist = contactDist;
            Restitution = 0.0f;
            Spring = new SpringParameters();
            Lower = lower;
            Upper = upper;
        }

        /// <summary>
        /// Constructs a soft limit. Once the limit is reached the bodies will bounce back according to the restitution parameter and will be pulled back towards the limit by the provided spring.
        /// </summary>
        /// <param name="lower">The lower distance of the limit. Must be less than upper.</param>
        /// <param name="upper">The upper distance of the limit. Must be more than lower.</param>
        /// <param name="spring">The spring that controls how are the bodies pulled back towards the limit when they breach it.</param>
        /// <param name="restitution">Controls how do objects react when the limit is reached, values closer to zero specify non-elastic collision, while those closer to one specify more elastic (i.e bouncy) collision. Must be in [0, 1] range.</param>
        public LimitLinearRange(float lower, float upper, SpringParameters spring, float restitution = 0.0f)
        {
            ContactDist = -1.0f;
            Restitution = restitution;
            Spring = spring;
            Lower = lower;
            Upper = upper;
        }
    }

    partial struct LimitLinear
    {
        /// <summary>
        /// Constructs a hard limit. Once the limit is reached the movement of the attached bodies will come to a stop.
        /// </summary>
        /// <param name="extent">The distance at which the limit becomes active.</param>
        /// <param name="contactDist">The distance from the limit at which it becomes active. Allows the solver to activate earlier than the limit is reached to avoid breaking the limit. Specify -1 for the default.</param>
        public LimitLinear(float extent, float contactDist = -1.0f)
        {
            ContactDist = contactDist;
            Restitution = 0.0f;
            Spring = new SpringParameters();
            Extent = extent;
        }

        /// <summary>
        /// Constructs a soft limit. Once the limit is reached the bodies will bounce back according to the restitution parameter and will be pulled back towards the limit by the provided spring.
        /// </summary>
        /// <param name="extent">The distance at which the limit becomes active.</param>
        /// <param name="spring">The spring that controls how are the bodies pulled back towards the limit when they breach it.</param>
        /// <param name="restitution">Controls how do objects react when the limit is reached, values closer to zero specify non-elastic collision, while those closer to one specify more elastic (i.e bouncy) collision. Must be in [0, 1] range.</param>
        public LimitLinear(float extent, SpringParameters spring, float restitution = 0.0f)
        {
            ContactDist = -1.0f;
            Restitution = restitution;
            Spring = spring;
            Extent = extent;
        }
    }

    partial struct LimitAngularRange
    {
        /// <summary>
        /// Constructs a hard limit. Once the limit is reached the movement of the attached bodies will come to a stop.
        /// </summary>
        /// <param name="lower">The lower angle of the limit (in degrees). Must be less than upper.</param>
        /// <param name="upper">The upper angle of the limit (in degrees). Must be more than lower.</param>
        /// <param name="contactDist">Distance from the limit at which it becomes active. Allows the solver to activate earlier than the limit is reached to avoid breaking the limit. Specify -1 for the default.</param>
        public LimitAngularRange(float lower, float upper, float contactDist = -1.0f)
        {
            ContactDist = contactDist;
            Restitution = 0.0f;
            Spring = new SpringParameters();
            Lower = lower;
            Upper = upper;
        }

        /// <summary>
        /// Constructs a soft limit. Once the limit is reached the bodies will bounce back according to the restitution parameter and will be pulled back towards the limit by the provided spring.
        /// </summary>
        /// <param name="lower">The lower angle of the limit. Must be less than upper.</param>
        /// <param name="upper">The upper angle of the limit. Must be more than lower.</param>
        /// <param name="spring">The spring that controls how are the bodies pulled back towards the limit when they breach it.</param>
        /// <param name="restitution">Controls how do objects react when the limit is reached, values closer to zero specify non-elastic collision, while those closer to one specify more elastic (i.e bouncy) collision. Must be in [0, 1] range.</param>
        public LimitAngularRange(float lower, float upper, SpringParameters spring, float restitution = 0.0f)
        {
            ContactDist = -1.0f;
            Restitution = restitution;
            Spring = spring;
            Lower = lower;
            Upper = upper;
        }
    }

    partial struct LimitConeRange
    {
        /// <summary>
        /// Constructs a hard limit. Once the limit is reached the movement of the attached bodies will come to a stop.
        /// </summary>
        /// <param name="yLimitAngle">The Y angle of the cone (in degrees). Movement is constrained between 0 and this angle on the Y axis.</param>
        /// <param name="zLimitAngle">The Z angle of the cone (in degrees). Movement is constrained between 0 and this angle on the Z axis.</param>
        /// <param name="contactDist">Distance from the limit at which it becomes active. Allows the solver to activate earlier than the limit is reached to avoid breaking the limit. Specify -1 for the default.</param>
        public LimitConeRange(float yLimitAngle, float zLimitAngle, float contactDist = -1.0f)
        {
            ContactDist = contactDist;
            Restitution = 0.0f;
            Spring = new SpringParameters();
            YLimitAngle = yLimitAngle;
            ZLimitAngle = zLimitAngle;
        }

        /// <summary>
        /// Constructs a soft limit. Once the limit is reached the bodies will bounce back according to the restitution parameter and will be pulled back towards the limit by the provided spring.
        /// </summary>
        /// <param name="yLimitAngle">The Y angle of the cone (in degrees). Movement is constrained between 0 and this angle on the Y axis.</param>
        /// <param name="zLimitAngle">The Z angle of the cone (in degrees). Movement is constrained between 0 and this angle on the Z axis.</param>
        /// <param name="spring">The spring that controls how are the bodies pulled back towards the limit when they breach it.</param>
        /// <param name="restitution">Controls how do objects react when the limit is reached, values closer to zero specify non-elastic collision, while those closer to one specify more elastic (i.e bouncy) collision. Must be in [0, 1] range.</param>
        public LimitConeRange(float yLimitAngle, float zLimitAngle, SpringParameters spring, float restitution = 0.0f)
        {
            ContactDist = -1.0f;
            Restitution = restitution;
            Spring = spring;
            YLimitAngle = yLimitAngle;
            ZLimitAngle = zLimitAngle;
        }
    }
}
