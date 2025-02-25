// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "SceneObject.h"
#include "Tags.h"
#include "Engine/Core/Types/Span.h"
#include "Engine/Core/Math/Transform.h"
#include "Engine/Core/Math/BoundingBox.h"
#include "Engine/Core/Math/BoundingSphere.h"
#include "Engine/Scripting/ScriptingObject.h"
#include "Types.h"

struct RenderView;
struct RenderContext;
struct RenderContextBatch;
class GPUContext;
class MemoryWriteStream;
class PhysicsScene;
class SceneRendering;
class SceneRenderTask;

/// <summary>
/// Base class for all actor objects on the scene.
/// </summary>
API_CLASS(Abstract) class FLAXENGINE_API Actor : public SceneObject
{
    DECLARE_SCENE_OBJECT(Actor);
    friend Level;
    friend PrefabManager;
    friend Scene;
    friend SceneRendering;
    friend Prefab;
    friend PrefabInstanceData;
protected:
    uint16 _isActive : 1;
    uint16 _isActiveInHierarchy : 1;
    uint16 _isPrefabRoot : 1;
    uint16 _isEnabled : 1;
    uint16 _isHierarchyDirty : 1;
    uint16 _drawNoCulling : 1;
    uint16 _drawCategory : 4;
    byte _layer;
    StaticFlags _staticFlags;
    Transform _localTransform;
    Transform _transform;
    BoundingSphere _sphere;
    BoundingBox _box;
    String _name;
    Scene* _scene;
    PhysicsScene* _physicsScene;

private:
    // Disable copying
    Actor(Actor const&) = delete;
    Actor& operator=(Actor const&) = delete;

public:
    /// <summary>
    /// List with all child actors attached to the actor (readonly). All items are valid (not null).
    /// </summary>
    API_FIELD(ReadOnly)
    Array<Actor*> Children;

    /// <summary>
    /// List with all scripts attached to the actor (readonly). All items are valid (not null).
    /// </summary>
    API_FIELD(ReadOnly, Attributes="HideInEditor, NoAnimate, EditorOrder(-5), EditorDisplay(\"Scripts\", EditorDisplayAttribute.InlineStyle), Collection(ReadOnly = true, NotNullItems = true, CanReorderItems = false)")
    Array<Script*> Scripts;

    /// <summary>
    /// The hide flags.
    /// </summary>
    API_FIELD(Attributes="HideInEditor, NoSerialize")
    HideFlags HideFlags;

    /// <summary>
    /// Actor tags collection.
    /// </summary>
    API_FIELD(Attributes="NoAnimate, EditorDisplay(\"General\"), EditorOrder(-68)") Array<Tag> Tags;

public:
    /// <summary>
    /// Gets the object layer (index). Can be used for selective rendering or ignoring raycasts.
    /// </summary>
    API_PROPERTY(Attributes="NoAnimate, EditorDisplay(\"General\"), EditorOrder(-69), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.ActorLayerEditor\")")
    FORCE_INLINE int32 GetLayer() const
    {
        return _layer;
    }

    /// <summary>
    /// Gets the layer mask (with single bit set).
    /// </summary>
    FORCE_INLINE int32 GetLayerMask() const
    {
        return 1 << static_cast<int32>(_layer);
    }

    /// <summary>
    /// Sets the layer.
    /// </summary>
    /// <param name="layerIndex">The index of the layer.</param>
    API_PROPERTY() void SetLayer(int32 layerIndex);

    /// <summary>
    /// Sets the layer recursively for all underlying children.
    /// </summary>
    /// <param name="layerIndex">The index of the layer.</param>
    API_FUNCTION() void SetLayerRecursive(int32 layerIndex);

    /// <summary>
    /// Gets the name of the layer.
    /// </summary>
    API_PROPERTY(Attributes="HideInEditor, NoSerialize, NoAnimate")
    const String& GetLayerName() const;

    /// <summary>
    /// Sets the name of the layer.
    /// </summary>
    API_PROPERTY() void SetLayerName(const StringView& value);

    /// <summary>
    /// Sets the name of the layer recursively for actor and for all underlying child actors.
    /// </summary>
    API_FUNCTION() void SetLayerNameRecursive(const StringView& value);

    /// <summary>
    /// Determines whether this actor has any tag assigned.
    /// </summary>
    API_FUNCTION() bool HasTag() const;

    /// <summary>
    /// Determines whether this actor has given tag assigned (exact match).
    /// </summary>
    /// <param name="tag">The tag to check.</param>
    API_FUNCTION() bool HasTag(const Tag& tag) const;

    /// <summary>
    /// Determines whether this actor has given tag assigned (exact match).
    /// </summary>
    /// <param name="tag">The tag to check.</param>
    API_FUNCTION() bool HasTag(const StringView& tag) const;

    /// <summary>
    /// Adds a tag to the actor
    /// </summary>
    /// <param name="tag">The tag to add.</param>
    API_FUNCTION() void AddTag(const Tag& tag);

    /// <summary>
    /// Adds a tag to the actor and for all underlying child actors.
    /// </summary>
    /// <param name="tag">The tag to add.</param>
    API_FUNCTION() void AddTagRecursive(const Tag& tag);

    /// <summary>
    /// Removes a tag to the actor
    /// </summary>
    /// <param name="tag">The tag to remove.</param>
    API_FUNCTION() void RemoveTag(const Tag& tag);

    /// <summary>
    /// Gets the name of the tag.
    /// [Deprecated in v1.5]
    /// </summary>
    API_PROPERTY(Attributes="HideInEditor, NoSerialize, NoAnimate")
    DEPRECATED("Use HasTag instead") const String& GetTag() const;

    /// <summary>
    /// Sets the name of the tag.
    /// [Deprecated in v1.5]
    /// </summary>
    API_PROPERTY() DEPRECATED("Use AddTag instead") void SetTag(const StringView& value);

    /// <summary>
    /// Gets the actor name.
    /// </summary>
    API_PROPERTY(Attributes="NoAnimate, EditorDisplay(\"General\"), EditorOrder(-100)")
    FORCE_INLINE const String& GetName() const
    {
        return _name;
    }

    /// <summary>
    /// Sets the actor name.
    /// </summary>
    /// <param name="value">The value to set.</param>
    API_PROPERTY() void SetName(String&& value);

    /// <summary>
    /// Sets the actor name.
    /// </summary>
    /// <param name="value">The value to set.</param>
    void SetName(const StringView& value);

public:
    /// <summary>
    /// Gets the scene object which contains this actor.
    /// </summary>
    API_PROPERTY(Attributes="HideInEditor, NoSerialize")
    FORCE_INLINE Scene* GetScene() const
    {
        return _scene;
    }

    /// <summary>
    /// Sets a actor parent.
    /// </summary>
    /// <param name="value">New parent</param>
    /// <param name="worldPositionsStays">Should actor world positions remain the same after parent change?</param>
    /// <param name="canBreakPrefabLink">True if can break prefab link on changing the parent.</param>
    API_FUNCTION() void SetParent(Actor* value, bool worldPositionsStays, bool canBreakPrefabLink);

    /// <summary>
    /// Gets amount of child actors.
    /// </summary>
    API_PROPERTY(Attributes="HideInEditor, NoSerialize")
    FORCE_INLINE int32 GetChildrenCount() const
    {
        return Children.Count();
    }

    /// <summary>
    /// Gets the child actor at the given index.
    /// </summary>
    /// <param name="index">The child actor index.</param>
    /// <returns>The child actor (always valid).</returns>
    API_FUNCTION() Actor* GetChild(int32 index) const;

    /// <summary>
    /// Gets the child actor with the given name.
    /// </summary>
    /// <param name="name">The child actor name.</param>
    /// <returns>The child actor or null.</returns>
    API_FUNCTION() Actor* GetChild(const StringView& name) const;

    /// <summary>
    /// Gets the child actor of the given type.
    /// </summary>
    /// <param name="type">Type of the actor to search for. Includes any actors derived from the type. Supports interface types.</param>
    /// <returns>The child actor or null.</returns>
    API_FUNCTION() Actor* GetChild(API_PARAM(Attributes="TypeReference(typeof(Actor))") const MClass* type) const;

    /// <summary>
    /// Gets the child actor of the given type.
    /// </summary>
    /// <returns>The child actor or null.</returns>
    template<typename T>
    FORCE_INLINE T* GetChild() const
    {
        return (T*)GetChild(T::GetStaticClass());
    }

    /// <summary>
    /// Finds the child actor of the given type or creates a new one.
    /// </summary>
    /// <returns>The child actor.</returns>
    template<typename T>
    T* GetOrAddChild()
    {
        T* result = (T*)GetChild(T::GetStaticClass());
        if (!result)
        {
            result = New<T>();
            result->SetParent(this, false, false);
        }
        return result;
    }

    /// <summary>
    /// Gets the child actors of the given type.
    /// </summary>
    /// <param name="type">Type of the actor to search for. Includes any actors derived from the type. Supports interface types.</param>
    /// <returns>The child actors.</returns>
    API_FUNCTION() Array<Actor*> GetChildren(API_PARAM(Attributes="TypeReference(typeof(Actor))") const MClass* type) const;

    /// <summary>
    /// Gets the child actors of the given type.
    /// </summary>
    /// <returns>The child actors.</returns>
    template<typename T>
    Array<T*> GetChildren() const
    {
        const MClass* type = T::GetStaticClass();
        Array<T*> result;
        for (Actor* child : Children)
            if (IsSubClassOf(child, type))
                result.Add((T*)child);
        return result;
    }

    /// <summary>
    /// Destroys the children. Calls Object.Destroy on every child actor and unlinks them for this actor.
    /// </summary>
    /// <param name="timeLeft">The time left to destroy object (in seconds).</param>
    API_FUNCTION(Attributes="NoAnimate") void DestroyChildren(float timeLeft = 0.0f);

public:
    /// <summary>
    /// Gets amount of scripts.
    /// </summary>
    API_PROPERTY(Attributes="HideInEditor, NoSerialize")
    FORCE_INLINE int32 GetScriptsCount() const
    {
        return Scripts.Count();
    }

    /// <summary>
    /// Gets the script at the given index.
    /// </summary>
    /// <param name="index">The script index.</param>
    /// <returns>The script (always valid).</returns>
    API_FUNCTION() Script* GetScript(int32 index) const;

    /// <summary>
    /// Gets the script of the given type from this actor.
    /// </summary>
    /// <param name="type">Type of the script to search for. Includes any scripts derived from the type. Supports interface types.</param>
    /// <returns>The script or null.</returns>
    API_FUNCTION() Script* GetScript(API_PARAM(Attributes="TypeReference(typeof(Script))") const MClass* type) const;

    /// <summary>
    /// Gets the script of the given type from this actor.
    /// </summary>
    /// <returns>The script or null.</returns>
    template<typename T>
    FORCE_INLINE T* GetScript() const
    {
        return (T*)GetScript(T::GetStaticClass());
    }

    /// <summary>
    /// Gets the scripts of the given type from this actor.
    /// </summary>
    /// <param name="type">Type of the script to search for. Includes any scripts derived from the type. Supports interface types.</param>
    /// <returns>The scripts.</returns>
    API_FUNCTION() Array<Script*> GetScripts(API_PARAM(Attributes="TypeReference(typeof(Script))") const MClass* type) const;

    /// <summary>
    /// Gets the scripts of the given type from this actor.
    /// </summary>
    /// <returns>The scripts.</returns>
    template<typename T>
    Array<T*> GetScripts() const
    {
        const MClass* type = T::GetStaticClass();
        Array<T*> result;
        for (Script* script : Scripts)
            if (IsSubClassOf(script, type))
                result.Add((T*)script);
        return result;
    }

    /// <summary>
    /// Creates a new script of a specific type and adds it to the actor.
    /// </summary>
    /// <returns>The created script instance, null otherwise.</returns>
    template<typename T>
    T* AddScript()
    {
        auto script = New<T>();
        script->SetParent(this);
        return script;
    }

public:
    /// <summary>
    /// Gets value indicating if actor is active in the scene.
    /// </summary>
    API_PROPERTY(Attributes="EditorDisplay(\"General\"), DefaultValue(true), EditorOrder(-70)")
    FORCE_INLINE bool GetIsActive() const
    {
        return _isActive != 0;
    }

    /// <summary>
    /// Sets value indicating if actor is active in the scene.
    /// </summary>
    /// <param name="value">The value to set.</param>
    API_PROPERTY() virtual void SetIsActive(bool value);

    /// <summary>
    /// Gets value indicating if actor is active in the scene graph. It must be active as well as that of all it's parents.
    /// </summary>
    API_PROPERTY(Attributes="HideInEditor, NoSerialize")
    FORCE_INLINE bool IsActiveInHierarchy() const
    {
        return _isActiveInHierarchy != 0;
    }

    /// <summary>
    /// Gets value indicating if actor is in a scene.
    /// </summary>
    API_PROPERTY(Attributes="HideInEditor, NoSerialize")
    FORCE_INLINE bool HasScene() const
    {
        return _scene != nullptr;
    }

    /// <summary>
    /// Returns true if object is fully static on the scene, otherwise false.
    /// </summary>
    FORCE_INLINE bool IsStatic() const
    {
        return _staticFlags == StaticFlags::FullyStatic;
    }

    /// <summary>
    /// Returns true if object has static transform.
    /// </summary>
    FORCE_INLINE bool IsTransformStatic() const
    {
        return (_staticFlags & StaticFlags::Transform) != StaticFlags::None;
    }

    /// <summary>
    /// Gets the actor static flags.
    /// </summary>
    API_PROPERTY(Attributes="NoAnimate, EditorDisplay(\"General\"), EditorOrder(-80), CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.ActorStaticFlagsEditor\")")
    FORCE_INLINE StaticFlags GetStaticFlags() const
    {
        return _staticFlags;
    }

    /// <summary>
    /// Sets the actor static flags.
    /// </summary>
    /// <param name="value">The value to set.</param>
    API_PROPERTY() virtual void SetStaticFlags(StaticFlags value);

    /// <summary>
    /// Returns true if object has given flag(s) set.
    /// </summary>
    FORCE_INLINE bool HasStaticFlag(StaticFlags flag) const
    {
        return EnumHasAllFlags(_staticFlags, flag);
    }

    /// <summary>
    /// Adds the actor static flags.
    /// </summary>
    /// <param name="flags">The flags to add.</param>
    FORCE_INLINE void AddStaticFlags(StaticFlags flags)
    {
        SetStaticFlags(_staticFlags | flags);
    }

    /// <summary>
    /// Removes the actor static flags.
    /// </summary>
    /// <param name="flags">The flags to remove.</param>
    FORCE_INLINE void RemoveStaticFlags(StaticFlags flags)
    {
        SetStaticFlags(_staticFlags & ~flags);
    }

    /// <summary>
    /// Sets a single static flag to the desire value.
    /// </summary>
    /// <param name="flag">The flag to change.</param>
    /// <param name="value">The target value of the flag.</param>
    FORCE_INLINE void SetStaticFlag(StaticFlags flag, bool value)
    {
        SetStaticFlags((_staticFlags & ~flag) | (value ? flag : StaticFlags::None));
    }

public:
    /// <summary>
    /// Gets the actor's world transformation.
    /// </summary>
    API_PROPERTY(Attributes="HideInEditor, NoSerialize")
    FORCE_INLINE const Transform& GetTransform() const
    {
        return _transform;
    }

    /// <summary>
    /// Sets the actor's world transformation.
    /// </summary>
    /// <param name="value">The value to set.</param>
    API_PROPERTY() void SetTransform(const Transform& value);

    /// <summary>
    /// Gets the actor's world transform position.
    /// </summary>
    API_PROPERTY(Attributes="HideInEditor, NoSerialize")
    FORCE_INLINE Vector3 GetPosition() const
    {
        return _transform.Translation;
    }

    /// <summary>
    /// Sets the actor's world transform position.
    /// </summary>
    /// <param name="value">The value to set.</param>
    API_PROPERTY() void SetPosition(const Vector3& value);

    /// <summary>
    /// Gets actor orientation in 3D space
    /// </summary>
    API_PROPERTY(Attributes="HideInEditor, NoSerialize")
    FORCE_INLINE Quaternion GetOrientation() const
    {
        return _transform.Orientation;
    }

    /// <summary>
    /// Sets actor orientation in 3D space.
    /// </summary>
    /// <param name="value">The value to set.</param>
    API_PROPERTY() void SetOrientation(const Quaternion& value);

    /// <summary>
    /// Gets actor scale in 3D space.
    /// </summary>
    API_PROPERTY(Attributes="HideInEditor, NoSerialize")
    FORCE_INLINE Float3 GetScale() const
    {
        return _transform.Scale;
    }

    /// <summary>
    /// Sets actor scale in 3D space
    /// </summary>
    /// <param name="value">The value to set.</param>
    API_PROPERTY() void SetScale(const Float3& value);

    /// <summary>
    /// Gets actor rotation matrix.
    /// </summary>
    API_PROPERTY(Attributes="HideInEditor, NoSerialize")
    Matrix GetRotation() const;

    /// <summary>
    /// Sets actor rotation matrix.
    /// </summary>
    /// <param name="value">The value to set.</param>
    API_PROPERTY() void SetRotation(const Matrix& value);

public:
    /// <summary>
    /// Gets the random per-instance value (normalized to range 0-1).
    /// </summary>
    API_PROPERTY() FORCE_INLINE float GetPerInstanceRandom() const
    {
        return _id.C * (1.0f / (float)MAX_uint32);
    }

    /// <summary>
    /// Gets actor direction vector (forward vector).
    /// </summary>
    API_PROPERTY(Attributes="HideInEditor, NoSerialize") FORCE_INLINE Float3 GetDirection() const
    {
        return Float3::Transform(Float3::Forward, GetOrientation());
    }

    /// <summary>
    /// Sets actor direction vector (forward)
    /// </summary>
    /// <param name="value">The value to set.</param>
    API_PROPERTY() void SetDirection(const Float3& value);

public:
    /// <summary>
    /// Resets the actor local transform.
    /// </summary>
    void ResetLocalTransform();

    /// <summary>
    /// Gets local transform of the actor in parent actor space.
    /// </summary>
    API_PROPERTY(Attributes="HideInEditor, NoAnimate")
    FORCE_INLINE Transform GetLocalTransform() const
    {
        return _localTransform;
    }

    /// <summary>
    /// Sets local transform of the actor in parent actor space.
    /// </summary>
    /// <param name="value">The value to set.</param>
    API_PROPERTY() void SetLocalTransform(const Transform& value);

    /// <summary>
    /// Gets local position of the actor in parent actor space.
    /// </summary>
    API_PROPERTY(Attributes="EditorDisplay(\"Transform\", \"Position\"), VisibleIf(\"ShowTransform\"), DefaultValue(typeof(Vector3), \"0,0,0\"), EditorOrder(-30), NoSerialize, CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.ActorTransformEditor+PositionEditor\")")
    FORCE_INLINE Vector3 GetLocalPosition() const
    {
        return _localTransform.Translation;
    }

    /// <summary>
    /// Sets local position of the actor in parent actor space.
    /// </summary>
    /// <param name="value">The value to set.</param>
    API_PROPERTY() void SetLocalPosition(const Vector3& value);

    /// <summary>
    /// Gets local rotation of the actor in parent actor space.
    /// </summary>
    /// <code>Actor.LocalOrientation *= Quaternion.Euler(0, 10 * Time.DeltaTime, 0)</code>
    API_PROPERTY(Attributes="EditorDisplay(\"Transform\", \"Rotation\"), VisibleIf(\"ShowTransform\"), DefaultValue(typeof(Quaternion), \"0,0,0,1\"), EditorOrder(-20), NoSerialize, CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.ActorTransformEditor+OrientationEditor\")")
    FORCE_INLINE Quaternion GetLocalOrientation() const
    {
        return _localTransform.Orientation;
    }

    /// <summary>
    /// Sets local rotation of the actor in parent actor space.
    /// </summary>
    /// <param name="value">The value to set.</param>
    API_PROPERTY() void SetLocalOrientation(const Quaternion& value);

    /// <summary>
    /// Gets local scale vector of the actor in parent actor space.
    /// </summary>
    API_PROPERTY(Attributes="EditorDisplay(\"Transform\", \"Scale\"), VisibleIf(\"ShowTransform\"), DefaultValue(typeof(Float3), \"1,1,1\"), Limit(float.MinValue, float.MaxValue, 0.01f), EditorOrder(-10), NoSerialize, CustomEditorAlias(\"FlaxEditor.CustomEditors.Editors.ActorTransformEditor+ScaleEditor\")")
    FORCE_INLINE Float3 GetLocalScale() const
    {
        return _localTransform.Scale;
    }

    /// <summary>
    /// Sets local scale vector of the actor in parent actor space.
    /// </summary>
    /// <param name="value">The value to set.</param>
    API_PROPERTY() void SetLocalScale(const Float3& value);

    /// <summary>
    /// Moves the actor (also can rotate it) in world space.
    /// </summary>
    /// <param name="translation">The translation vector.</param>
    API_FUNCTION() FORCE_INLINE void AddMovement(const Vector3& translation)
    {
        AddMovement(translation, Quaternion::Identity);
    }

    /// <summary>
    /// Moves the actor (also can rotate it) in world space.
    /// </summary>
    /// <param name="translation">The translation vector.</param>
    /// <param name="rotation">The rotation quaternion.</param>
    API_FUNCTION() virtual void AddMovement(const Vector3& translation, const Quaternion& rotation);

    /// <summary>
    /// Gets the matrix that transforms a point from the world space to local space of the actor.
    /// </summary>
    /// <param name="worldToLocal">The world to local matrix.</param>
    API_FUNCTION() void GetWorldToLocalMatrix(API_PARAM(Out) Matrix& worldToLocal) const;

    /// <summary>
    /// Gets the matrix that transforms a point from the local space of the actor to world space.
    /// </summary>
    /// <param name="localToWorld">The world to local matrix.</param>
    API_FUNCTION() void GetLocalToWorldMatrix(API_PARAM(Out) Matrix& localToWorld) const;

    /// <summary>
    /// Gets the matrix that transforms a point from the world space to local space of the actor.
    /// </summary>
    /// <param name="worldToLocal">The world to local matrix.</param>
    void GetWorldToLocalMatrix(Double4x4& worldToLocal) const;

    /// <summary>
    /// Gets the matrix that transforms a point from the local space of the actor to world space.
    /// </summary>
    /// <param name="localToWorld">The world to local matrix.</param>
    void GetLocalToWorldMatrix(Double4x4& localToWorld) const;

public:
    /// <summary>
    /// Gets actor bounding sphere that defines 3D space intersecting with the actor (for determination of the visibility for actor).
    /// </summary>
    API_PROPERTY() FORCE_INLINE const BoundingSphere& GetSphere() const
    {
        return _sphere;
    }

    /// <summary>
    /// Gets actor bounding box that defines 3D space intersecting with the actor (for determination of the visibility for actor).
    /// </summary>
    API_PROPERTY() FORCE_INLINE const BoundingBox& GetBox() const
    {
        return _box;
    }

    /// <summary>
    /// Gets actor bounding box of the actor including all child actors (children included in recursive way)
    /// </summary>
    API_PROPERTY() BoundingBox GetBoxWithChildren() const;

#if USE_EDITOR
    /// <summary>
    /// Gets actor bounding box (single actor, no children included) for editor tools.
    /// </summary>
    API_PROPERTY() virtual BoundingBox GetEditorBox() const;

    /// <summary>
    /// Gets actor bounding box of the actor including all child actors for editor tools.
    /// </summary>
    API_PROPERTY() BoundingBox GetEditorBoxChildren() const;
#endif

    /// <summary>
    /// Returns true if actor has loaded content.
    /// </summary>
    API_PROPERTY() virtual bool HasContentLoaded() const;

    /// <summary>
    /// Calls UnregisterObject for all objects in the actor hierarchy.
    /// </summary>
    void UnregisterObjectHierarchy();

    /// <summary>
    /// Calls Initialize for all objects in the actor hierarchy.
    /// </summary>
    void InitializeHierarchy();

public:
    /// <summary>
    /// Draws this actor. Called by Scene Rendering service. This call is more optimized than generic Draw (eg. geometry is rendered during all pass types but other actors are drawn only during GBufferFill pass).
    /// </summary>
    /// <param name="renderContext">The rendering context.</param>
    virtual void Draw(RenderContext& renderContext);

    /// <summary>
    /// Draws this actor. Called by Scene Rendering service. This call is more optimized than generic Draw (eg. geometry is rendered during all pass types but other actors are drawn only during GBufferFill pass).
    /// </summary>
    /// <param name="renderContextBatch">The rendering context batch (eg, main view and shadow projections).</param>
    virtual void Draw(RenderContextBatch& renderContextBatch);

#if USE_EDITOR
    /// <summary>
    /// Draws debug shapes for the actor and all child scripts.
    /// </summary>
    API_FUNCTION() virtual void OnDebugDraw();

    /// <summary>
    /// Draws debug shapes for the selected actor and all child scripts.
    /// </summary>
    API_FUNCTION() virtual void OnDebugDrawSelected();
#endif

public:
    /// <summary>
    /// Changes the script order.
    /// </summary>
    /// <param name="script">The script.</param>
    /// <param name="newIndex">The new index.</param>
    void ChangeScriptOrder(Script* script, int32 newIndex);

    /// <summary>
    /// Gets the script by its identifier.
    /// </summary>
    /// <param name="id">The script identifier.</param>
    /// <returns>The script or null.</returns>
    Script* GetScriptByID(const Guid& id) const;

    /// <summary>
    /// Gets a value indicating whether this actor is a prefab instance root object.
    /// </summary>
    API_PROPERTY() bool IsPrefabRoot() const;

    /// <summary>
    /// Gets the root of the prefab this actor is attached to.
    /// </summary>
    /// <returns>The root prefab object, or null if this actor is not a prefab.</returns>
    API_FUNCTION() Actor* GetPrefabRoot();

public:
    /// <summary>
    /// Tries to find the actor with the given name in this actor hierarchy (checks this actor and all children hierarchy).
    /// </summary>
    /// <param name="name">The name of the actor.</param>
    /// <returns>Actor instance if found, null otherwise.</returns>
    API_FUNCTION() Actor* FindActor(const StringView& name) const;

    /// <summary>
    /// Tries to find the actor of the given type in this actor hierarchy (checks this actor and all children hierarchy).
    /// </summary>
    /// <param name="type">Type of the actor to search for. Includes any actors derived from the type. Supports interface types.</param>
    /// <param name="activeOnly">Finds only a active actor.</param>
    /// <returns>Actor instance if found, null otherwise.</returns>
    API_FUNCTION() Actor* FindActor(API_PARAM(Attributes="TypeReference(typeof(Actor))") const MClass* type, bool activeOnly = false) const;

    /// <summary>
    /// Tries to find the actor of the given type and name in this actor hierarchy (checks this actor and all children hierarchy).
    /// </summary>
    /// <param name="type">Type of the actor to search for. Includes any actors derived from the type. Supports interface types.</param>
    /// <param name="name">The name of the actor.</param>
    /// <returns>Actor instance if found, null otherwise.</returns>
    API_FUNCTION() Actor* FindActor(API_PARAM(Attributes="TypeReference(typeof(Actor))") const MClass* type, const StringView& name) const;

    /// <summary>
    /// Tries to find the actor of the given type and tag in this actor hierarchy.
    /// </summary>
    /// <param name="type">Type of the actor to search for. Includes any actors derived from the type. Supports interface types.</param>
    /// <param name="tag">The tag of the actor to search for.</param>
    /// <param name="activeOnly">Finds only an active actor.</param>
    /// <returns>Actor instance if found, null otherwise.</returns>
    API_FUNCTION() Actor* FindActor(API_PARAM(Attributes="TypeReference(typeof(Actor))") const MClass* type, const Tag& tag, bool activeOnly = false) const;

    /// <summary>
    /// Tries to find the actor of the given type in this actor hierarchy (checks this actor and all children hierarchy).
    /// </summary>
    /// <returns>Actor instance if found, null otherwise.</returns>
    template<typename T>
    FORCE_INLINE T* FindActor() const
    {
        return (T*)FindActor(T::GetStaticClass());
    }

    /// <summary>
    /// Tries to find the actor of the given type and name in this actor hierarchy (checks this actor and all children hierarchy).
    /// </summary>
    /// <param name="name">The name of the actor.</param>
    /// <returns>Actor instance if found, null otherwise.</returns>
    template<typename T>
    FORCE_INLINE T* FindActor(const StringView& name) const
    {
        return (T*)FindActor(T::GetStaticClass(), name);
    }

    /// <summary>
    /// Tries to find the actor of the given type and tag in this actor hierarchy (checks this actor and all children hierarchy).
    /// </summary>
    /// <param name="tag">The tag of the actor to search for.</param>
    /// <returns>Actor instance if found, null otherwise.</returns>
    template<typename T>
    FORCE_INLINE T* FindActor(const Tag& tag) const
    {
        return (T*)FindActor(T::GetStaticClass(), tag);
    }

    /// <summary>
    /// Tries to find the script of the given type in this actor hierarchy (checks this actor and all children hierarchy).
    /// </summary>
    /// <param name="type">Type of the actor to search for. Includes any actors derived from the type. Supports interface types.</param>
    /// <returns>Script instance if found, null otherwise.</returns>
    API_FUNCTION() Script* FindScript(API_PARAM(Attributes="TypeReference(typeof(Script))") const MClass* type) const;

    /// <summary>
    /// Tries to find the script of the given type in this actor hierarchy (checks this actor and all children hierarchy).
    /// </summary>
    /// <returns>Script instance if found, null otherwise.</returns>
    template<typename T>
    FORCE_INLINE T* FindScript() const
    {
        return (T*)FindScript(T::GetStaticClass());
    }

    /// <summary>
    /// Try to find actor in hierarchy structure.
    /// </summary>
    /// <param name="a">The actor to find.</param>
    /// <returns>Found actor or null.</returns>
    API_FUNCTION() bool HasActorInHierarchy(Actor* a) const;

    /// <summary>
    /// Try to find actor in child actors structure.
    /// </summary>
    /// <param name="a">The actor to find.</param>
    /// <returns>Found actor or null.</returns>
    API_FUNCTION() bool HasActorInChildren(Actor* a) const;

    /// <summary>
    /// Determines if there is an intersection between the current object and a ray.
    /// </summary>
    /// <param name="ray">The ray to test.</param>
    /// <param name="distance">When the method completes, contains the distance of the intersection (if any valid).</param>
    /// <param name="normal">When the method completes, contains the intersection surface normal vector (if any valid).</param>
    /// <returns>True whether the two objects intersected, otherwise false.</returns>
    API_FUNCTION() virtual bool IntersectsItself(const Ray& ray, API_PARAM(Out) Real& distance, API_PARAM(Out) Vector3& normal);

    /// <summary>
    /// Determines if there is an intersection between the current object or any it's child and a ray.
    /// </summary>
    /// <param name="ray">The ray to test.</param>
    /// <param name="distance">When the method completes, contains the distance of the intersection (if any valid).</param>
    /// <param name="normal">When the method completes, contains the intersection surface normal vector (if any valid).</param>
    /// <returns>The target hit actor that is the closest to the ray.</returns>
    API_FUNCTION() Actor* Intersects(const Ray& ray, API_PARAM(Out) Real& distance, API_PARAM(Out) Vector3& normal);

    /// <summary>
    /// Rotates actor to orient it towards the specified world position.
    /// </summary>
    /// <param name="worldPos">The world position to orient towards.</param>
    API_FUNCTION() void LookAt(const Vector3& worldPos);

    /// <summary>
    /// Rotates actor to orient it towards the specified world position with upwards direction.
    /// </summary>
    /// <param name="worldPos">The world position to orient towards.</param>
    /// <param name="worldUp">The up direction that constrains up axis orientation to a plane this vector lies on. This rule might be broken if forward and up direction are nearly parallel.</param>
    API_FUNCTION() void LookAt(const Vector3& worldPos, const Vector3& worldUp);

    /// <summary>
    /// Gets rotation of the actor oriented towards the specified world position.
    /// </summary>
    /// <param name="worldPos">The world position to orient towards.</param>
    API_FUNCTION() Quaternion LookingAt(const Vector3& worldPos) const;

    /// <summary>
    /// Gets rotation of the actor oriented towards the specified world position with upwards direction.
    /// </summary>
    /// <param name="worldPos">The world position to orient towards.</param>
    /// <param name="worldUp">The up direction that constrains up axis orientation to a plane this vector lies on. This rule might be broken if forward and up direction are nearly parallel.</param>
    API_FUNCTION() Quaternion LookingAt(const Vector3& worldPos, const Vector3& worldUp) const;

public:
    /// <summary>
    /// Execute custom action on actors tree.
    /// </summary>
    /// <param name="action">Actor to call on every actor in the tree. Returns true if keep calling deeper.</param>
    /// <param name="args">Custom arguments for the function</param>
    template<typename... Params>
    void TreeExecute(Function<bool(Actor*, Params ...)>& action, Params ... args)
    {
        if (action(this, args...))
        {
            for (int32 i = 0; i < Children.Count(); i++)
                Children.Get()[i]->TreeExecute<Params...>(action, args...);
        }
    }

    /// <summary>
    /// Execute custom action on actor children tree.
    /// </summary>
    /// <param name="action">Actor to call on every actor in the tree. Returns true if keep calling deeper.</param>
    /// <param name="args">Custom arguments for the function</param>
    template<typename... Params>
    void TreeExecuteChildren(Function<bool(Actor*, Params ...)>& action, Params ... args)
    {
        for (int32 i = 0; i < Children.Count(); i++)
            Children.Get()[i]->TreeExecute<Params...>(action, args...);
    }

public:
    /// <summary>
    /// Performs actors serialization to the raw bytes.
    /// </summary>
    /// <param name="actors">The actors to serialize.</param>
    /// <param name="output">The output data stream.</param>
    /// <returns>True if fails, otherwise false.</returns>
    static bool ToBytes(const Array<Actor*>& actors, MemoryWriteStream& output);

    /// <summary>
    /// Performs actors serialization to the raw bytes.
    /// </summary>
    /// <param name="actors">The actors to serialize.</param>
    /// <returns>The output data, empty if failed.</returns>
    API_FUNCTION() static Array<byte> ToBytes(const Array<Actor*>& actors);

    /// <summary>
    /// Performs actors deserialization from the raw bytes.
    /// </summary>
    /// <param name="data">The input data.</param>
    /// <param name="output">The output actors.</param>
    /// <param name="modifier">The custom serialization modifier.</param>
    /// <returns>True if fails, otherwise false.</returns>
    static bool FromBytes(const Span<byte>& data, Array<Actor*>& output, ISerializeModifier* modifier);

    /// <summary>
    /// Performs actors deserialization from the raw bytes.
    /// </summary>
    /// <param name="data">The input data.</param>
    /// <returns>The output actors.</returns>
    API_FUNCTION() static Array<Actor*> FromBytes(const Span<byte>& data);

    /// <summary>
    /// Performs actors deserialization from the raw bytes.
    /// </summary>
    /// <param name="data">The input data.</param>
    /// <param name="idsMapping">The serialized objects Ids mapping. Can be used to convert the spawned objects ids and references to them.</param>
    /// <returns>The output actors.</returns>
    API_FUNCTION() static Array<Actor*> FromBytes(const Span<byte>& data, const Dictionary<Guid, Guid, HeapAllocation>& idsMapping);

    /// <summary>
    /// Tries the get serialized objects ids from the raw bytes.
    /// </summary>
    /// <param name="data">The data.</param>
    /// <returns>The output array of serialized object ids.</returns>
    API_FUNCTION() static Array<Guid> TryGetSerializedObjectsIds(const Span<byte>& data);

    /// <summary>
    /// Serializes the actor object to the Json string. Serialized are only this actor properties but no child actors nor scripts. Serializes references to the other objects in a proper way using IDs.
    /// </summary>
    /// <returns>The Json container with serialized actor data.</returns>
    API_FUNCTION() String ToJson();

    /// <summary>
    /// Deserializes the actor object to the Json string. Deserialized are only this actor properties but no child actors nor scripts. 
    /// </summary>
    /// <param name="json">The serialized actor data (state).</param>
    API_FUNCTION() void FromJson(const StringAnsiView& json);

    /// <summary>
    /// Clones actor including all scripts and any child actors (whole scene tree). Objects are duplicated via serialization (any transient/non-saved state is ignored).
    /// </summary>
    API_FUNCTION() Actor* Clone();

public:
    /// <summary>
    /// Called when actor gets added to game systems. Occurs on BeginPlay event or when actor gets activated in hierarchy. Use this event to register object to other game system (eg. audio).
    /// </summary>
    API_FUNCTION() virtual void OnEnable();

    /// <summary>
    /// Called when actor gets removed from game systems. Occurs on EndPlay event or when actor gets inactivated in hierarchy. Use this event to unregister object from other game system (eg. audio).
    /// </summary>
    API_FUNCTION() virtual void OnDisable();

    /// <summary>
    /// Called when actor parent gets changed.
    /// </summary>
    API_FUNCTION() virtual void OnParentChanged();

    /// <summary>
    /// Called when actor transform gets changed.
    /// </summary>
    API_FUNCTION() virtual void OnTransformChanged();

    /// <summary>
    /// Called when actor active state gets changed.
    /// </summary>
    API_FUNCTION() virtual void OnActiveChanged();

    /// <summary>
    /// Called when actor active in tree state gets changed.
    /// </summary>
    API_FUNCTION() virtual void OnActiveInTreeChanged();

    /// <summary>
    /// Called when order in parent children array gets changed.
    /// </summary>
    API_FUNCTION() virtual void OnOrderInParentChanged();

    /// <summary>
    /// Called when actor static flag gets changed.
    /// </summary>
    API_FUNCTION() virtual void OnStaticFlagsChanged();

    /// <summary>
    /// Called when layer gets changed.
    /// </summary>
    API_FUNCTION() virtual void OnLayerChanged();

    /// <summary>
    /// Called when adding object to the game.
    /// </summary>
    API_FUNCTION(Attributes="NoAnimate") virtual void OnBeginPlay()
    {
    }

    /// <summary>
    /// Called when removing object from the game.
    /// </summary>
    API_FUNCTION(Attributes="NoAnimate") virtual void OnEndPlay()
    {
    }

    /// <summary>
    /// Gets the scene rendering object.
    /// </summary>
    /// <returns>The scene rendering interface.</returns>
    SceneRendering* GetSceneRendering() const;

public:
    /// <summary>
    /// Set the physics world the controller is part of.
    /// </summary>
    API_PROPERTY(Attributes="HideInEditor") void SetPhysicsScene(PhysicsScene* scene);

    /// <summary>
    /// Get the physics world the controller is part of.
    /// </summary>
    API_PROPERTY(Attributes="HideInEditor") PhysicsScene* GetPhysicsScene() const;

protected:
    virtual void OnPhysicsSceneChanged(PhysicsScene* previous)
    {
    }

private:
    void SetSceneInHierarchy(Scene* scene);
    void OnEnableInHierarchy();
    void OnDisableInHierarchy();

    // Helper methods used by templates GetChildren/GetScripts to prevent including MClass/Script here
    static bool IsSubClassOf(const Actor* object, const MClass* klass);
    static bool IsSubClassOf(const Script* object, const MClass* klass);

public:
    // [ScriptingObject]
    String ToString() const override;
    void OnDeleteObject() override;

    // [SceneObject]
    const Guid& GetSceneObjectId() const override;
    void SetParent(Actor* value, bool canBreakPrefabLink = true) override;
    int32 GetOrderInParent() const override;
    void SetOrderInParent(int32 index) override;
    void LinkPrefab(const Guid& prefabId, const Guid& prefabObjectId) override;
    void BreakPrefabLink() override;
    void Initialize() override;
    void BeginPlay(SceneBeginData* data) override;
    void EndPlay() override;
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
};
