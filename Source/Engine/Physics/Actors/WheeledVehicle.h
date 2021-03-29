// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Physics/Actors/RigidBody.h"
#include "Engine/Physics/Colliders/Collider.h"
#include "Engine/Scripting/ScriptingObjectReference.h"

class Physics;

/// <summary>
/// Representation of the car vehicle that uses wheels. Built on top of the RigidBody with collider representing its chassis shape and wheels.
/// </summary>
/// <seealso cref="RigidBody" />
API_CLASS() class FLAXENGINE_API WheeledVehicle : public RigidBody
{
    friend Physics;
DECLARE_SCENE_OBJECT(WheeledVehicle);
public:

    /// <summary>
    /// Vehicle driving mode types.
    /// </summary>
    API_ENUM() enum class DriveTypes
    {
        // Four-wheel drive. Any additional wheels are non-drivable. Optimized for 4-wheel cars.
        Drive4W,
        // N-wheel drive. Up to 20 drivable wheels. Suits generic wheels configurations.
        DriveNW,
        // Non-drivable vehicle.
        NoDrive,
    };

    /// <summary>
    /// Vehicle gearbox settings.
    /// </summary>
    API_STRUCT() struct GearboxSettings : ISerializable
    {
    DECLARE_SCRIPTING_TYPE_MINIMAL(GearboxSettings);
    API_AUTO_SERIALIZATION();

        /// <summary>
        /// If enabled the vehicle gears will be changes automatically, otherwise it's fully manual.
        /// </summary>
        API_FIELD() bool AutoGear = true;

        /// <summary>
        /// Time it takes to switch gear. Specified in seconds (s).
        /// </summary>
        API_FIELD() float SwitchTime = 0.5f;
    };

    /// <summary>
    /// Vehicle engine settings.
    /// </summary>
    API_STRUCT() struct EngineSettings : ISerializable
    {
    DECLARE_SCRIPTING_TYPE_MINIMAL(EngineSettings);
    API_AUTO_SERIALIZATION();

        /// <summary>
        /// If enabled the vehicle gears will be changes automatically, otherwise it's fully manual.
        /// </summary>
        API_FIELD() bool AutoGear = true;

        /// <summary>
        /// Moment of inertia of the engine around the axis of rotation. Specified in kilograms metres-squared (kg m^2).
        /// </summary>
        API_FIELD() float MOI = 1.0f;

        /// <summary>
        /// Maximum torque available to apply to the engine when the accelerator pedal is at maximum. Specified in kilograms metres-squared per second-squared (kg m^2 s^-2).
        /// </summary>
        API_FIELD() float MaxTorque = 500.0f;

        /// <summary>
        /// Maximum rotation speed of the engine (Revolutions Per Minute is the number of turns in one minute).
        /// </summary>
        API_FIELD() float MaxRotationSpeed = 6000.0f;
    };

    /// <summary>
    /// Vehicle wheel types.
    /// </summary>
    API_ENUM() enum class WheelTypes
    {
        // Left wheel of the front axle.
        FrontLeft,
        // Right wheel of the front axle.
        FrontRight,
        // Left wheel of the rear axle.
        RearLeft,
        // Right wheel of the rear axle.
        ReadRight,
        // Non-drivable wheel.
        NoDrive,
    };

    /// <summary>
    /// Vehicle wheel settings.
    /// </summary>
    API_STRUCT() struct Wheel : ISerializable
    {
    DECLARE_SCRIPTING_TYPE_MINIMAL(Wheel);
    API_AUTO_SERIALIZATION();

        /// <summary>
        /// Wheel placement type.
        /// </summary>
        API_FIELD() WheelTypes Type = WheelTypes::FrontLeft;

        /// <summary>
        /// Combined mass of the wheel and the tire in kg. Typically, a wheel has mass between 20Kg and 80Kg but can be lower and higher depending on the vehicle.
        /// </summary>
        API_FIELD() float Mass = 20.0f;

        /// <summary>
        /// Distance in metres between the center of the wheel and the outside rim of the tire. It is important that the value of the radius closely matches the radius of the render mesh of the wheel. Any mismatch will result in the wheels either hovering above the ground or intersecting the ground.
        /// </summary>
        API_FIELD() float Radius = 50.0f;

        /// <summary>
        /// Full width of the wheel in metres. This parameter has no bearing on the handling but is a very useful parameter to have when trying to render debug data relating to the wheel/tire/suspension.
        /// </summary>
        API_FIELD() float Width = 20.0f;

        /// <summary>
        /// Collider that represents the wheel shape and it's placement. Has to be attached as a child to the vehicle. Triangle mesh collider is not supported (use convex mesh or basic shapes).
        /// </summary>
        API_FIELD() ScriptingObjectReference<Collider> Collider;
    };

    /// <summary>
    /// Vehicle wheel dynamic simulation state container.
    /// </summary>
    API_STRUCT() struct WheelState
    {
    DECLARE_SCRIPTING_TYPE_MINIMAL(WheelState);

        /// <summary>
        /// True if suspension travel limits forbid the wheel from touching the drivable surface.
        /// </summary>
        API_FIELD() bool IsInAir = false;

        /// <summary>
        /// The wheel is not in the air then it's set to the collider of the driving surface under the corresponding vehicle wheel.
        /// </summary>
        API_FIELD() PhysicsColliderActor* TireContactCollider = nullptr;

        /// <summary>
        /// The wheel is not in the air then it's set to the point on the drivable surface hit by the tire.
        /// </summary>
        API_FIELD() Vector3 TireContactPoint = Vector3::Zero;

        /// <summary>
        /// The wheel is not in the air then it's set to the normal on the drivable surface hit by the tire.
        /// </summary>
        API_FIELD() Vector3 TireContactNormal = Vector3::Zero;

        /// <summary>
        /// The friction experienced by the tire for the combination of tire type and surface type after accounting.
        /// </summary>
        API_FIELD() float TireFriction = 0.0f;
    };

private:

    struct WheelData
    {
        Collider* Collider;
        Quaternion LocalOrientation;
        WheelState State;
    };

    void* _drive = nullptr;
    DriveTypes _driveType = DriveTypes::Drive4W, _driveTypeCurrent;
    Array<WheelData, FixedAllocation<20>> _wheelsData;
    float _throttle = 0.0f, _steering = 0.0f, _brake = 0.0f, _handBrake = 0.0f;
    Array<Wheel> _wheels;
    GearboxSettings _gearbox;
    EngineSettings _engine;

public:

    /// <summary>
    /// If checked, the negative throttle value will be used as brake and reverse to behave in a more arcade style where holding reverse also functions as brake. Disable it for more realistic driving controls.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(0), EditorDisplay(\"Vehicle\")")
    bool UseReverseAsBrake = true;

    /// <summary>
    /// Gets the vehicle driving model type.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(1), EditorDisplay(\"Vehicle\")") DriveTypes GetDriveType() const;

    /// <summary>
    /// Sets the vehicle driving model type.
    /// </summary>
    API_PROPERTY() void SetDriveType(DriveTypes value);

    /// <summary>
    /// Gets the vehicle wheels settings.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(2), EditorDisplay(\"Vehicle\")") const Array<Wheel>& GetWheels() const;

    /// <summary>
    /// Sets the vehicle wheels settings.
    /// </summary>
    API_PROPERTY() void SetWheels(const Array<Wheel>& value);

    /// <summary>
    /// Gets the vehicle engine settings.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(3), EditorDisplay(\"Vehicle\")") EngineSettings GetEngine() const;

    /// <summary>
    /// Sets the vehicle engine settings.
    /// </summary>
    API_PROPERTY() void SetEngine(const EngineSettings& value);

    /// <summary>
    /// Gets the vehicle gearbox settings.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(5), EditorDisplay(\"Vehicle\")") GearboxSettings GetGearbox() const;

    /// <summary>
    /// Sets the vehicle gearbox settings.
    /// </summary>
    API_PROPERTY() void SetGearbox(const GearboxSettings& value);

public:

    /// <summary>
    /// Sets the input for vehicle throttle. It is the analog accelerator pedal value in range (0,1) where 1 represents the pedal fully pressed and 0 represents the pedal in its rest state.
    /// </summary>
    /// <param name="value">The value (-1,1 range). When using UseReverseAsBrake it can be negative and will be used as brake and backward driving.</param>
    API_FUNCTION() void SetThrottle(float value);

    /// <summary>
    /// Sets the input for vehicle steering. Steer is the analog steer value in range (-1,1) where -1 represents the steering wheel at left lock and +1 represents the steering wheel at right lock.
    /// </summary>
    /// <param name="value">The value (-1,1 range).</param>
    API_FUNCTION() void SetSteering(float value);

    /// <summary>
    /// Sets the input for vehicle brakes. Brake is the analog brake pedal value in range (0,1) where 1 represents the pedal fully pressed and 0 represents the pedal in its rest state.
    /// </summary>
    /// <param name="value">The value (0,1 range).</param>
    API_FUNCTION() void SetBrake(float value);

    /// <summary>
    /// Sets the input for vehicle handbrake. Handbrake is the analog handbrake value in range (0,1) where 1 represents the handbrake fully engaged and 0 represents the handbrake in its rest state.
    /// </summary>
    /// <param name="value">The value (0,1 range).</param>
    API_FUNCTION() void SetHandbrake(float value);

    /// <summary>
    /// Clears all the vehicle control inputs to the default values (throttle, steering, breaks).
    /// </summary>
    API_FUNCTION() void ClearInput();

public:

    /// <summary>
    /// Gets the current forward vehicle movement speed (along forward vector of the actor transform).
    /// </summary>
    API_PROPERTY() float GetForwardSpeed() const;

    /// <summary>
    /// Gets the current sideways vehicle movement speed (along right vector of the actor transform).
    /// </summary>
    API_PROPERTY() float GetSidewaysSpeed() const;

    /// <summary>
    /// Gets the current engine rotation speed (Revolutions Per Minute is the number of turns in one minute).
    /// </summary>
    API_PROPERTY() float GetEngineRotationSpeed() const;

    /// <summary>
    /// Gets the current gear number. Neutral gears is 0, reverse gears is -1, forward gears are 1 and higher.
    /// </summary>
    API_PROPERTY(Attributes="HideInEditor") int32 GetCurrentGear() const;

    /// <summary>
    /// Sets the current gear number. The gear change is instant. Neutral gears is 0, reverse gears is -1, forward gears are 1 and higher.
    /// </summary>
    API_PROPERTY() void SetCurrentGear(int32 value);

    /// <summary>
    /// Gets the target gear number. Neutral gears is 0, reverse gears is -1, forward gears are 1 and higher.
    /// </summary>
    API_PROPERTY(Attributes="HideInEditor") int32 GetTargetGear() const;

    /// <summary>
    /// Sets the target gear number. Gearbox will change the current gear to the target. Neutral gears is 0, reverse gears is -1, forward gears are 1 and higher.
    /// </summary>
    API_PROPERTY() void SetTargetGear(int32 value);

    /// <summary>
    /// Gets the current state of the wheel.
    /// </summary>
    /// <param name="index">The index of the wheel.</param>
    /// <param name="result">The current state.</param>
    API_FUNCTION() void GetWheelState(int32 index, API_PARAM(Out) WheelState& result);

private:

    void Setup();
#if USE_EDITOR
    void DrawPhysicsDebug(RenderView& view);
#endif

public:

    // [Vehicle]
#if USE_EDITOR
    void OnDebugDrawSelected() override;
#endif
    void Serialize(SerializeStream& stream, const void* otherObj) override;
    void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) override;
    void OnColliderChanged(Collider* c) override;

protected:

    // [Vehicle]
    void BeginPlay(SceneBeginData* data) override;
    void EndPlay() override;
};
