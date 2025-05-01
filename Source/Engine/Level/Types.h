// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Config.h"
#include "Engine/Core/Types/BaseTypes.h"

// Level system types
class SceneObject;
class Actor;
class Scene;
class Prefab;
class JsonAsset;
class Level;
class PrefabManager;
class PrefabInstanceData;
class Script;
class ManagedScript;
class SceneInfo;
class Light;
class LightWithShadow;
class ModelInstanceActor;

// Actor types
class StaticModel;
class Camera;
class EmptyActor;
class DirectionalLight;
class PointLight;
class Skybox;
class EnvironmentProbe;
class BoxBrush;
class Scene;
class Sky;
class RigidBody;
class SpotLight;
class PostFxVolume;
class BoxCollider;
class SphereCollider;
class CapsuleCollider;
class CharacterController;
class FixedJoint;
class DistanceJoint;
class HingeJoint;
class SliderJoint;
class SphericalJoint;
class D6Joint;
class MeshCollider;
class SkyLight;
class ExponentialHeightFog;
class TextRender;
class AudioSource;
class AudioListener;
class AnimatedModel;
class BoneSocket;
class Decal;

// Default extension for JSON scene files
#define DEFAULT_SCENE_EXTENSION TEXT("scene")
#define DEFAULT_SCENE_EXTENSION_DOT TEXT(".scene")

// Default extension for JSON prefab files
#define DEFAULT_PREFAB_EXTENSION TEXT("prefab")
#define DEFAULT_PREFAB_EXTENSION_DOT TEXT(".prefab")

// Default extension for JSON resource files
#define DEFAULT_JSON_EXTENSION TEXT("json")
#define DEFAULT_JSON_EXTENSION_DOT TEXT(".json")

/// <summary>
/// Static flags for the actor object.
/// </summary>
API_ENUM(Attributes="Flags") enum class StaticFlags
{
    /// <summary>
    /// Non-static object.
    /// </summary>
    None = 0,

    /// <summary>
    /// Object is considered to be static for reflection probes offline caching.
    /// </summary>
    ReflectionProbe = 1 << 0,

    /// <summary>
    /// Object is considered to be static for static lightmaps.
    /// </summary>
    Lightmap = 1 << 1,

    /// <summary>
    /// Object is considered to have static transformation in space (no dynamic physics and any movement at runtime).
    /// </summary>
    Transform = 1 << 2,

    /// <summary>
    /// Object is considered to affect navigation (static occluder or walkable surface).
    /// </summary>
    Navigation = 1 << 3,

    /// <summary>
    /// Object is considered to have static shadowing (casting and receiving).
    /// </summary>
    Shadow = 1 << 4,

    /// <summary>
    /// Object is fully static in the scene.
    /// </summary>
    FullyStatic = ReflectionProbe | Lightmap | Transform | Navigation | Shadow,

    /// <summary>
    /// Maximum value of the enum (force to int).
    /// </summary>
    API_ENUM(Attributes="HideInEditor")
    MAX = 1 << 31,
};

DECLARE_ENUM_OPERATORS(StaticFlags);

/// <summary>
/// Object hide state description flags. Control object appearance.
/// </summary>
API_ENUM(Attributes="Flags") enum class HideFlags
{
    /// <summary>
    /// The default state.
    /// </summary>
    None = 0,

    /// <summary>
    /// The object will not be visible in the hierarchy.
    /// </summary>
    HideInHierarchy = 1,

    /// <summary>
    /// The object will not be saved.
    /// </summary>
    DontSave = 2,

    /// <summary>
    /// The object will not selectable in the editor viewport.
    /// </summary>
    DontSelect = 4,

    /// <summary>
    /// The fully hidden object flags mask.
    /// </summary>
    FullyHidden = HideInHierarchy | DontSave | DontSelect,
};

DECLARE_ENUM_OPERATORS(HideFlags);
