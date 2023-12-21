//writen by https://github.com/NoriteSC

#pragma once
#include "IKBone.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Level/Actor.h"

/// <summary>
/// utility class.
/// types of solvers
/// </summary>
API_CLASS(Static, Namespace = "FlaxEngine.Experimental.Animation.IK") class FLAXENGINE_API IKSolver
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(IKSolver)
public:

    /// <summary>
    /// Constructs the IKChain.
    /// </summary>
    /// <param name="Actors">The actors.</param>
    /// <returns></returns>
    API_FUNCTION() static Array<IKBone> ConstructChainFrom(const Span<Actor>& Actors);

    /// <summary>
    /// Constructs the IKChain.
    /// </summary>
    /// <param name="Transforms">The transforms.</param>
    /// <returns></returns>
    API_FUNCTION() static Array<IKBone> ConstructChainFrom(const Array<Transform>& Transforms);
    
    /// <summary>
    /// Constructs the IKChain.
    /// </summary>
    /// <param name="Locations">The locations.</param>
    /// <returns></returns>
    API_FUNCTION() static Array<IKBone> ConstructChainFrom(const Array<Vector3>& Locations);

    /// <summary>
    /// Forwards the and backward reaching inverse kinematic solver.
    /// can be used with min 2 bones or whole chain
    /// </summary>
    /// <param name="InOutBones">The bones.</param>
    /// <param name="MaxIturation">The maximum ituration.</param>
    /// <param name="Base">The base.</param>
    /// <param name="Target">The target.</param>
    /// <param name="PullTargetDirection">The pull target direction.</param>
    /// <param name="DebugDraw">if set to <c>true</c> [debug draw].</param>
    API_FUNCTION() static void ForwardAndBackwardReachingInverseKinematic
    (
        API_PARAM(Ref) Span<IKBone>& InOutBones,
        int MaxIturation,
        const Transform& Base,
        const Transform& Target,
        const Vector3& PullTargetDirection,
        bool DebugDraw = false
    );

    /// <summary>
    /// Forwards reaching inverse kinematic solver.
    /// can be used with min 2 bones or whole chain
    /// </summary>
    /// <param name="InOutBones">The bones.</param>
    /// <param name="MaxIturation">The maximum ituration.</param>
    /// <param name="Target">The target.</param>
    /// <param name="PullTargetDirection">The pull target direction.</param>
    /// <param name="DebugDraw">if set to <c>true</c> [debug draw].</param>
    API_FUNCTION() static void ForwardReachingInverseKinematic
    (
        API_PARAM(Ref) Span<IKBone>& InOutBones,
        int MaxIturation,
        const Transform& Target,
        const Vector3& PullTargetDirection,
        bool DebugDraw = false
    );

    /// <summary>
    /// backward reaching inverse kinematic solver.
    /// can be used with min 2 bones or whole chain
    /// </summary>
    /// <param name="InOutBones">The bones.</param>
    /// <param name="MaxIturation">The maximum ituration.</param>
    /// <param name="Target">The target.</param>
    /// <param name="PullTargetDirection">The pull target direction.</param>
    /// <param name="DebugDraw">if set to <c>true</c> [debug draw].</param>
    API_FUNCTION() static void BackwardReachingInverseKinematic
    (
        API_PARAM(Ref) Span<IKBone>& InOutBones,
        int MaxIturation,
        const Transform& Target,
        const Vector3& PullTargetDirection,
        bool DebugDraw = false
    );

protected:
    static void HandleBonesDrawAndZeroLenghtBones(Span<IKBone>& InOutBones, bool DebugDraw);
    static void FinalizeSolveInduration(Span<IKBone>& InOutBones, const Transform& Target);
};
