// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

namespace FlaxEngine
{
    /// <summary>
    /// Physical materials are used to define the response of a physical object when interacting dynamically with the world.
    /// </summary>
    public sealed class PhysicalMaterial
    {
        /// <summary>
        /// The friction value of surface, controls how easily things can slide on this surface.
        /// </summary>
        [EditorOrder(0), Limit(0), EditorDisplay("Physical Material"), Tooltip("The friction value of surface, controls how easily things can slide on this surface.")]
        public float Friction = 0.7f;

        /// <summary>
        /// The friction combine mode, controls how friction is computed for multiple materials.
        /// </summary>
        [EditorOrder(1), EditorDisplay("Physical Material"), Tooltip("The friction combine mode, controls how friction is computed for multiple materials.")]
        public PhysicsCombineMode FrictionCombineMode = PhysicsCombineMode.Average;

        /// <summary>
        /// If set we will use the FrictionCombineMode of this material, instead of the FrictionCombineMode found in the Physics settings. 
        /// </summary>
        [HideInEditor]
        public bool OverrideFrictionCombineMode = false;

        /// <summary>
        /// The restitution or 'bounciness' of this surface, between 0 (no bounce) and 1 (outgoing velocity is same as incoming).
        /// </summary>
        [EditorOrder(3), Range(0, 1), EditorDisplay("Physical Material"), Tooltip("The restitution or \'bounciness\' of this surface, between 0 (no bounce) and 1 (outgoing velocity is same as incoming).")]
        public float Restitution = 0.3f;

        /// <summary>
        /// The restitution combine mode, controls how restitution is computed for multiple materials.
        /// </summary>
        [EditorOrder(4), EditorDisplay("Physical Material"), Tooltip("The restitution combine mode, controls how restitution is computed for multiple materials.")]
        public PhysicsCombineMode RestitutionCombineMode = PhysicsCombineMode.Average;

        /// <summary>
        /// If set we will use the RestitutionCombineMode of this material, instead of the RestitutionCombineMode found in the Physics settings.
        /// </summary>
        [HideInEditor]
        public bool OverrideRestitutionCombineMode = false;
    }
}
