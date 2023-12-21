//writen by https://github.com/NoriteSC

#pragma once
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Quaternion.h"
#include "Engine/Core/Math/Transform.h"
#include "Engine/Core/Math/Color.h"
#include "Engine/Scripting/ScriptingObject.h"

API_CLASS() class FLAXENGINE_API IKBone : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE(IKBone);
public:    
    /// <summary>
    /// 
    /// </summary>
    API_STRUCT() struct Limits
    {
        DECLARE_SCRIPTING_TYPE_STRUCTURE(Limits);
    public:        
        /// <summary>
        /// The active
        /// </summary>
        API_FIELD() bool Active;
        /// <summary>
        /// Determines the minimum angle.
        /// </summary>
        API_FIELD() float Min;

        /// <summary>
        /// Determines the maximum angle.
        /// </summary>
        API_FIELD() float Max;
    };
protected:

    /// <summary>
    /// The head (start location)
    /// </summary>
    Vector3 Head;

    /// <summary>
    /// The taill (end Location)
    /// </summary>
    Vector3 Taill;

    /// <summary>
    /// The lenght of the bone
    /// </summary>
    float Lenght;

    /// <summary>
    /// The roll of the bone
    /// </summary>
    float Roll;

    /// <summary>
    /// The oritentacion of the parent
    /// </summary>
    Quaternion ParentOritentacion; //[ToDo] use ponter ? for it the ShaderPointer will be use full in this case but o well std:: is not allowed in this engine

    /// <summary>
    /// The oritentacion
    /// </summary>
    Quaternion Oritentacion;

    /// <summary>
    /// Debug color for a bone
    /// </summary>
    Color DebugColor;
public:

    /// <summary>
    /// The x constrain
    /// </summary>
    API_FIELD() Limits X;

    /// <summary>
    /// The y constrain
    /// </summary>
    API_FIELD() Limits Y;

    /// <summary>
    /// The z constrain
    /// </summary>
    API_FIELD() Limits Z;

    /// <summary>
    /// Snaps the head to location.
    /// </summary>
    /// <param name="newLocation">The new location.</param>
    API_FUNCTION() void SnapHeadTo(const Vector3& newLocation);

    /// <summary>
    /// Snaps the taill to location.
    /// </summary>
    /// <param name="newLocation">The new location.</param>
    API_FUNCTION() void SnapTaillTo(const Vector3& newLocation);

    /// <summary>
    /// Sets the orientation constrained version.
    /// </summary>
    /// <param name="Oritentacion">The oritentacion.</param>
    API_FUNCTION() void SetOrientation(const Quaternion& Oritentacion);

    /// <summary>
    /// Sets the orientation unconstrained.
    /// </summary>
    /// <param name="Oritentacion">The oritentacion.</param>
    API_FUNCTION() void SetOrientationUnconstrained(const Quaternion& Oritentacion);

    /// <summary>
    /// Gets the direction.
    /// </summary>
    /// <returns></returns>
    API_FUNCTION() Vector3 GetDirection();

#if USE_EDITOR

    /// <summary>
    /// Debugs the draw.
    /// </summary>
    API_FUNCTION() void Draw();

    /// <summary>
    /// Draws the octahedral bone.
    /// </summary>
    /// <param name="Head">The head.</param>
    /// <param name="Taill">The taill.</param>
    /// <param name="Roll">The roll.</param>
    /// <param name="Color">The color.</param>
    API_FUNCTION() static void DrawOctahedralBone(const Vector3& Head, const Vector3& Taill, float Roll, const Color& Color);
#else
    API_FUNCTION() void DebugDraw() {};
    API_FUNCTION() static void DrawOctahedralBone(const Vector3& Head, const Vector3& Taill, float Roll, const Color& Color) {};
#endif

    /// <summary>
    /// Gets the transform.
    /// </summary>
    /// <returns>world space transform</returns>
    API_FUNCTION() Transform GetTransform() { return Transform(Head, Oritentacion, Vector3::One); }

protected:
    friend class IKSolver;
};
