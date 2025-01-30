// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Physics/Actors/RigidBody.h"
#include "Engine/Physics/Colliders/Collider.h"
#include "Engine/Scripting/ScriptingObjectReference.h"

/// <summary>
/// Representation of the car vehicle that uses wheels. Built on top of the RigidBody with collider representing its chassis shape and wheels.
/// </summary>
/// <seealso cref="RigidBody" />
API_CLASS(Attributes="ActorContextMenu(\"New/Physics/Wheeled Vehicle\"), ActorToolbox(\"Physics\")") class FLAXENGINE_API WheeledVehicle : public RigidBody
{
    friend class PhysicsBackend;
    friend struct ScenePhysX;
    DECLARE_SCENE_OBJECT(WheeledVehicle);

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
        // Tank Drive. Can have more than 4 wheel. Not use steer, control acceleration for each tank track.
        Tank,
    };

    /// <summary>
    /// Vehicle driving types. Used only on tanks to specify the drive mode.   
    /// </summary>
    API_ENUM() enum class DriveModes
    {
        // Drive turning the vehicle using only one track.
        Standard,
        // Drive turning the vehicle using all tracks inverse direction.
        Special,
    };

    /// <summary>
    /// Storage the relationship between speed and steer.
    /// </summary>
    API_STRUCT() struct SteerControl : ISerializable
    {
        DECLARE_SCRIPTING_TYPE_MINIMAL(SteerControl);
        API_AUTO_SERIALIZATION();

        /// <summary>
        /// The vehicle speed.
        /// </summary>
        API_FIELD(Attributes = "Limit(0)") float Speed = 1000;

        /// <summary>
        /// The target max steer of the speed.
        /// </summary>
        API_FIELD(Attributes = "Limit(0, 1)") float Steer = 1;

        SteerControl() = default;

        /// <summary>
        /// Create a Steer/Speed relationship structure.
        /// <param name="speed">The vehicle speed.</param>
        /// <param name="steer">The target max steer of the speed.</param>
        /// </summary>
        SteerControl(float speed, float steer)
        {
            Speed = speed;
            Steer = steer;
        }
    };

    /// <summary>
    /// Vehicle engine settings.
    /// </summary>
    API_STRUCT() struct EngineSettings : ISerializable
    {
        DECLARE_SCRIPTING_TYPE_MINIMAL(EngineSettings);
        API_AUTO_SERIALIZATION();

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
    /// Vehicle differential types.
    /// </summary>
    API_ENUM() enum class DifferentialTypes
    {
        // Limited slip differential for car with 4 driven wheels.
        LimitedSlip4W,
        // Limited slip differential for car with front-wheel drive.
        LimitedSlipFrontDrive,
        // Limited slip differential for car with rear-wheel drive.
        LimitedSlipRearDrive,
        // Open differential for car with 4 driven wheels.
        Open4W,
        // Open differential for car with front-wheel drive.
        OpenFrontDrive,
        // Open differential for car with rear-wheel drive.
        OpenRearDrive,
    };

    /// <summary>
    /// Vehicle drive control settings.
    /// </summary>
    API_STRUCT() struct DriveControlSettings : ISerializable
    {
        DECLARE_SCRIPTING_TYPE_MINIMAL(DriveControlSettings);
        API_AUTO_SERIALIZATION();

        /// <summary>
        /// Gets or sets the drive mode, used by vehicles specify the way of the tracks control.
        /// </summary>
        API_FIELD(Attributes="EditorOrder(0), EditorDisplay(\"Tanks\")") WheeledVehicle::DriveModes DriveMode = WheeledVehicle::DriveModes::Standard;

        /// <summary>
        /// Acceleration input sensitive.
        /// </summary>
        API_FIELD(Attributes="EditorOrder(10)") float RiseRateAcceleration = 6.0f;

        /// <summary>
        /// Deceleration input sensitive.
        /// </summary>
        API_FIELD(Attributes="EditorOrder(11)") float FallRateAcceleration = 10.0f;

        /// <summary>
        /// Brake input sensitive.
        /// </summary>
        API_FIELD(Attributes="EditorOrder(12)") float RiseRateBrake = 6.0f;

        /// <summary>
        /// Release brake sensitive.
        /// </summary>
        API_FIELD(Attributes="EditorOrder(13)") float FallRateBrake = 10.0f;

        /// <summary>
        /// Brake input sensitive.
        /// </summary>
        API_FIELD(Attributes="EditorOrder(14)") float RiseRateHandBrake = 12.0f;

        /// <summary>
        /// Release handbrake sensitive.
        /// </summary>
        API_FIELD(Attributes="EditorOrder(15)") float FallRateHandBrake = 12.0f;

        /// <summary>
        /// Steer input sensitive.
        /// </summary>
        API_FIELD(Attributes="EditorOrder(16)") float RiseRateSteer = 2.5f;

        /// <summary>
        /// Release steer input sensitive.
        /// </summary>
        API_FIELD(Attributes="EditorOrder(17)") float FallRateSteer = 5.0f;

        /// <summary>
        /// Vehicle control relationship between speed and steer. The higher is the speed, decrease steer to make vehicle more maneuverable (limited only 4 relationships).
        /// </summary>
        API_FIELD() Array<WheeledVehicle::SteerControl> SteerVsSpeed = Array<WheeledVehicle::SteerControl>
        {
            SteerControl(800, 1.0f),
            SteerControl(1500, 0.7f),
            SteerControl(2500, 0.5f),
            SteerControl(5000, 0.2f),
        };
    };

    /// <summary>
    /// Vehicle differential settings.
    /// </summary>
    API_STRUCT() struct DifferentialSettings : ISerializable
    {
        DECLARE_SCRIPTING_TYPE_MINIMAL(DifferentialSettings);
        API_AUTO_SERIALIZATION();

        /// <summary>
        /// Type of differential.
        /// </summary>
        API_FIELD() DifferentialTypes Type = DifferentialTypes::LimitedSlip4W;

        /// <summary>
        /// Ratio of torque split between front and rear (higher then 0.5 means more to front, smaller than 0.5 means more to rear). Only applied to LimitedSlip4W and Open4W.
        /// </summary>
        API_FIELD(Attributes="Limit(0, 1)") float FrontRearSplit = 0.45f;

        /// <summary>
        /// Ratio of torque split between front-left and front-right (higher then 0.5 means more to front-left, smaller than 0.5 means more to front-right). Only applied to LimitedSlip4W and Open4W and LimitedSlipFrontDrive.
        /// </summary>
        API_FIELD(Attributes="Limit(0, 1)") float FrontLeftRightSplit = 0.5f;

        /// <summary>
        /// Ratio of torque split between rear-left and rear-right (higher then 0.5 means more to rear-left, smaller than 0.5 means more to rear-right). Only applied to LimitedSlip4W and Open4W and LimitedSlipRearDrive.
        /// </summary>
        API_FIELD(Attributes="Limit(0, 1)") float RearLeftRightSplit = 0.5f;

        /// <summary>
        /// Maximum allowed ratio of average front wheel rotation speed and rear wheel rotation speeds. The differential will divert more torque to the slower wheels when the bias is exceeded. Only applied to LimitedSlip4W.
        /// </summary>
        API_FIELD(Attributes="Limit(1)") float CentreBias = 1.3f;

        /// <summary>
        /// Maximum allowed ratio of front-left and front-right wheel rotation speeds. The differential will divert more torque to the slower wheel when the bias is exceeded. Only applied to LimitedSlip4W and LimitedSlipFrontDrive.
        /// </summary>
        API_FIELD(Attributes="Limit(1)") float FrontBias = 1.3f;

        /// <summary>
        /// Maximum allowed ratio of rear-left and rear-right wheel rotation speeds. The differential will divert more torque to the slower wheel when the bias is exceeded. Only applied to LimitedSlip4W and LimitedSlipRearDrive.
        /// </summary>
        API_FIELD(Attributes="Limit(1)") float RearBias = 1.3f;
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
        /// Number of gears to move to forward
        /// </summary>
        API_FIELD(Attributes = "Limit(1, 30)") int32 ForwardGearsRatios = 5;

        /// <summary>
        /// Time it takes to switch gear. Specified in seconds (s).
        /// </summary>
        API_FIELD(Attributes="Limit(0)") float SwitchTime = 0.5f;

        /// <summary>
        /// Strength of clutch. A stronger clutch more strongly couples the engine to the wheels, while a clutch of strength zero completely decouples the engine from the wheels.
        /// Stronger clutches more quickly bring the wheels and engine into equilibrium, while weaker clutches take longer, resulting in periods of clutch slip and delays in power transmission from the engine to the wheels.
        /// Specified in kilograms metres-squared per second (kg m^2 s^-1).
        /// </summary>
        API_FIELD(Attributes="Limit(0)") float ClutchStrength = 10.0f;
    };

    /// <summary>
    /// Vehicle wheel settings.
    /// </summary>
    API_STRUCT() struct Wheel : ISerializable
    {
        DECLARE_SCRIPTING_TYPE_MINIMAL(Wheel);
        API_AUTO_SERIALIZATION();

        /// <summary>
        /// Combined mass of the wheel and the tire in kg. Typically, a wheel has mass between 20Kg and 80Kg but can be lower and higher depending on the vehicle.
        /// </summary>
        API_FIELD(Attributes="EditorOrder(1)") float Mass = 20.0f;

        /// <summary>
        /// Distance in metres between the center of the wheel and the outside rim of the tire. It is important that the value of the radius closely matches the radius of the render mesh of the wheel. Any mismatch will result in the wheels either hovering above the ground or intersecting the ground.
        /// </summary>
        API_FIELD(Attributes="EditorOrder(2)") float Radius = 50.0f;

        /// <summary>
        /// Full width of the wheel in metres. This parameter has no bearing on the handling but is a very useful parameter to have when trying to render debug data relating to the wheel/tire/suspension.
        /// </summary>
        API_FIELD(Attributes="EditorOrder(3)") float Width = 20.0f;

        /// <summary>
        /// Max steer angle that can be achieved by the wheel (in degrees, -180 to 180).
        /// </summary>
        API_FIELD(Attributes="Limit(-180, 180), EditorDisplay(\"Steering\"), EditorOrder(10)") float MaxSteerAngle = 0.0f;

        /// <summary>
        /// Damping rate applied to wheel. Specified in kilograms metres-squared per second (kg m^2 s^-1).
        /// </summary>
        API_FIELD(Attributes="Limit(0), EditorDisplay(\"Steering\"), EditorOrder(11)") float DampingRate = 0.25f;

        /// <summary>
        /// Max brake torque that can be applied to wheel. Specified in kilograms metres-squared per second-squared (kg m^2 s^-2)
        /// </summary>
        API_FIELD(Attributes="Limit(0), EditorDisplay(\"Steering\"), EditorOrder(12)") float MaxBrakeTorque = 1500.0f;

        /// <summary>
        /// Max handbrake torque that can be applied to wheel. Specified in kilograms metres-squared per second-squared (kg m^2 s^-2)
        /// </summary>
        API_FIELD(Attributes="Limit(0), EditorDisplay(\"Steering\"), EditorOrder(13)") float MaxHandBrakeTorque = 2000.0f;

        /// <summary>
        /// Collider that represents the wheel shape and it's placement. Has to be attached as a child to the vehicle. Triangle mesh collider is not supported (use convex mesh or basic shapes).
        /// </summary>
        API_FIELD(Attributes="EditorOrder(4)") ScriptingObjectReference<Collider> Collider;

        /// <summary>
        /// Spring sprung mass force multiplier.
        /// </summary>
        API_FIELD(Attributes="Limit(0.01f), EditorDisplay(\"Suspension\"), EditorOrder(19)") float SprungMassMultiplier = 1.0f;

        /// <summary>
        /// Spring damper rate of suspension unit.
        /// </summary>
        API_FIELD(Attributes="Limit(0), EditorDisplay(\"Suspension\"), EditorOrder(20)") float SuspensionDampingRate = 1.0f;

        /// <summary>
        /// The maximum offset for the suspension that wheel can go above resting location.
        /// </summary>
        API_FIELD(Attributes="Limit(0), EditorDisplay(\"Suspension\"), EditorOrder(21)") float SuspensionMaxRaise = 10.0f;

        /// <summary>
        /// The maximum offset for the suspension that wheel can go below resting location.
        /// </summary>
        API_FIELD(Attributes="Limit(0), EditorDisplay(\"Suspension\"), EditorOrder(22)") float SuspensionMaxDrop = 10.0f;

        /// <summary>
        /// The vertical offset from where suspension forces are applied (relative to the vehicle center of mass). The suspension force is applies on the vertical axis going though the wheel center.
        /// </summary>
        API_FIELD(Attributes="EditorDisplay(\"Suspension\"), EditorOrder(23)") float SuspensionForceOffset = 0.0f;

        /// <summary>
        /// The tire lateral stiffness to have given lateral slip.
        /// </summary>
        API_FIELD(Attributes="EditorDisplay(\"Tire\"), EditorOrder(30)") float TireLateralStiffness = 17.0f;

        /// <summary>
        /// The maximum tire load (normalized) at which tire cannot provide more lateral stiffness (no matter how much extra load is applied to it).
        /// </summary>
        API_FIELD(Attributes="EditorDisplay(\"Tire\"), EditorOrder(31)") float TireLateralMax = 2.0f;

        /// <summary>
        /// The tire longitudinal stiffness to have given longitudinal slip.
        /// </summary>
        API_FIELD(Attributes="EditorDisplay(\"Tire\"), EditorOrder(32)") float TireLongitudinalStiffness = 1000.0f;

        /// <summary>
        /// The tire friction scale (scales the drivable surface friction under the tire).
        /// </summary>
        API_FIELD(Attributes="EditorDisplay(\"Tire\"), EditorOrder(33)") float TireFrictionScale = 1.0f;
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

        /// <summary>
        /// The steer angle (in degrees) of the wheel about the "up" vector accounting for input steer and toe and, if applicable, Ackermann steer correction.
        /// </summary>
        API_FIELD() float SteerAngle = 0.0f;

        /// <summary>
        /// The rotation angle (in degrees) about the rolling axis for the specified wheel.
        /// </summary>
        API_FIELD() float RotationAngle = 0.0f;

        /// <summary>
        /// The compression of the suspension spring. Offsets the wheel location.
        /// </summary>
        API_FIELD() float SuspensionOffset = 0.0f;

#if USE_EDITOR
        /// <summary>
        /// The start location of the suspension raycast start (Editor only for debugging).
        /// </summary>
        API_FIELD() Vector3 SuspensionTraceStart = Vector3::Zero;

        /// <summary>
        /// The start location of the suspension raycast end (Editor only for debugging).
        /// </summary>
        API_FIELD() Vector3 SuspensionTraceEnd = Vector3::Zero;
#endif
    };

    /// <summary>
    /// Vehicle axle anti roll bar.
    /// </summary>
    API_STRUCT() struct AntiRollBar : ISerializable
    {
        DECLARE_SCRIPTING_TYPE_MINIMAL(AntiRollBar);
        API_AUTO_SERIALIZATION();

        /// <summary>
        /// The specific axle with wheels to apply anti roll.
        /// </summary>
        API_FIELD() int32 Axle;

        /// <summary>
        /// The anti roll stiffness.
        /// </summary>
        API_FIELD() float Stiffness;
    };

private:
    struct WheelData
    {
        Collider* Collider;
        Quaternion LocalOrientation;
        WheelState State;
    };

    void* _vehicle = nullptr;
    DriveTypes _driveType = DriveTypes::Drive4W, _driveTypeCurrent;
    Array<WheelData, FixedAllocation<20>> _wheelsData;
    float _throttle = 0.0f, _steering = 0.0f, _brake = 0.0f, _handBrake = 0.0f, _tankLeftThrottle = 0.0f, _tankRightThrottle = 0.0f, _tankLeftBrake = 0.0f, _tankRightBrake = 0.0f;
    Array<Wheel> _wheels;
    Array<AntiRollBar> _antiRollBars;
    DriveControlSettings _driveControl;
    EngineSettings _engine;
    DifferentialSettings _differential;
    GearboxSettings _gearbox;
    bool _fixInvalidForwardDir = false; // [Deprecated on 13.06.2023, expires on 13.06.2025]
    bool _useWheelsUpdates = true;

public:
    /// <summary>
    /// If checked, the negative throttle value will be used as brake and reverse to behave in a more arcade style where holding reverse also functions as brake. Disable it for more realistic driving controls.
    /// </summary>
    API_FIELD(Attributes="EditorOrder(0), EditorDisplay(\"Vehicle\")")
    bool UseReverseAsBrake = true;

    /// <summary>
    /// If checked, the vehicle driving and steering inputs will be used as analog values (from gamepad), otherwise will be used as digital input (from keyboard).
    /// </summary>
    API_FIELD(Attributes="EditorOrder(1), EditorDisplay(\"Vehicle\")")
    bool UseAnalogSteering = false;

    /// <summary>
    /// Gets the vehicle driving model type.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(2), EditorDisplay(\"Vehicle\")") DriveTypes GetDriveType() const;

    /// <summary>
    /// Sets the vehicle driving model type.
    /// </summary>
    API_PROPERTY() void SetDriveType(DriveTypes value);

    /// <summary>
    /// Gets the vehicle wheels settings.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(4), EditorDisplay(\"Vehicle\")") const Array<Wheel>& GetWheels() const;

    /// <summary>
    /// Gets the vehicle drive control settings.
    /// </summary>
    API_PROPERTY(Attributes = "EditorOrder(5), EditorDisplay(\"Vehicle\")") DriveControlSettings GetDriveControl() const;

    /// <summary>
    /// Sets the vehicle drive control settings.
    /// </summary>
    API_PROPERTY() void SetDriveControl(DriveControlSettings value);

    /// <summary>
    /// Sets the vehicle wheels settings.
    /// </summary>
    API_PROPERTY() void SetWheels(const Array<Wheel>& value);

    /// <summary>
    /// Gets the vehicle engine settings.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(6), EditorDisplay(\"Vehicle\")") EngineSettings GetEngine() const;

    /// <summary>
    /// Sets the vehicle engine settings.
    /// </summary>
    API_PROPERTY() void SetEngine(const EngineSettings& value);

    /// <summary>
    /// Gets the vehicle differential settings.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(7), EditorDisplay(\"Vehicle\")") DifferentialSettings GetDifferential() const;

    /// <summary>
    /// Sets the vehicle differential settings.
    /// </summary>
    API_PROPERTY() void SetDifferential(const DifferentialSettings& value);

    /// <summary>
    /// Gets the vehicle gearbox settings.
    /// </summary>
    API_PROPERTY(Attributes="EditorOrder(8), EditorDisplay(\"Vehicle\")") GearboxSettings GetGearbox() const;

    /// <summary>
    /// Sets the vehicle gearbox settings.
    /// </summary>
    API_PROPERTY() void SetGearbox(const GearboxSettings& value);

    // <summary>
    /// Sets axles anti roll bars to increase vehicle stability.
    /// </summary>
    API_PROPERTY() void SetAntiRollBars(const Array<AntiRollBar>& value);

    // <summary>
    /// Gets axles anti roll bars.
    /// </summary>
    API_PROPERTY() const Array<AntiRollBar>& GetAntiRollBars() const;

public:
    /// <summary>
    /// Sets the input for vehicle throttle. It is the analog accelerator pedal value in range (0,1) where 1 represents the pedal fully pressed and 0 represents the pedal in its rest state.
    /// </summary>
    /// <param name="value">The value (-1,1 range). When using UseReverseAsBrake it can be negative and will be used as brake and backward driving.</param>
    API_FUNCTION() void SetThrottle(float value);

    /// <summary>
    /// Get the vehicle throttle. It is the analog accelerator pedal value in range (0,1) where 1 represents the pedal fully pressed and 0 represents the pedal in its rest state.
    /// </summary>
    /// <returns>The vehicle throttle.</returns>
    API_FUNCTION() float GetThrottle();

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
    /// Sets the input for tank left track throttle. It is the analog accelerator pedal value in range (-1,1) where 1 represents the pedal fully pressed to move to forward, 0 to represents the 
    /// pedal in its rest state and -1 represents the pedal fully pressed to move to backward. The track direction will be inverted if the vehicle current gear is rear.
    /// </summary>
    /// <param name="value">The value (-1,1 range).</param>
    API_FUNCTION() void SetTankLeftThrottle(float value);

    /// <summary>
    /// Sets the input for tank right track throttle. It is the analog accelerator pedal value in range (-1,1) where 1 represents the pedal fully pressed to move to forward, 0 to represents the
    /// pedal in its rest state and -1 represents the pedal fully pressed to move to backward. The track direction will be inverted if the vehicle current gear is rear.
    /// </summary>
    /// <param name="value">The value (-1,1 range).</param>
    API_FUNCTION() void SetTankRightThrottle(float value);

    /// <summary>
    /// Sets the input for tank brakes the left track. Brake is the analog brake pedal value in range (0,1) where 1 represents the pedal fully pressed and 0 represents the pedal in its rest state.
    /// </summary>
    /// <param name="value">The value (0,1 range).</param>
    API_FUNCTION() void SetTankLeftBrake(float value);

    /// <summary>
    /// Sets the input for tank brakes the right track. Brake is the analog brake pedal value in range (0,1) where 1 represents the pedal fully pressed and 0 represents the pedal in its rest state.
    /// </summary>
    /// <param name="value">The value (0,1 range).</param>
    API_FUNCTION() void SetTankRightBrake(float value);

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

    /// <summary>
    /// Rebuilds the vehicle. Call it after modifying vehicle settings (eg. engine options).
    /// </summary>
    API_FUNCTION() void Setup();

private:
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
    void OnActiveInTreeChanged() override;

protected:
    void OnPhysicsSceneChanged(PhysicsScene* previous) override;

    // [Vehicle]
    void OnTransformChanged() override;
    void BeginPlay(SceneBeginData* data) override;
    void EndPlay() override;
};
