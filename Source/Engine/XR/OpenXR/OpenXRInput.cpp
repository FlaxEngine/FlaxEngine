// Copyright (c) Wojciech Figat. All rights reserved.

#if XR_API_OPENXR

#include "OpenXRInput.h"
#include "OpenXRInstance.h"
#include "OpenXRSession.h"
#include "OpenXRConvert.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Input/Input.h"
#include "Engine/Input/Gamepad.h"
#include "Engine/Platform/StringUtils.h"
#include "Engine/Profiler/ProfilerCPU.h"

// Trigger and grip value above which the gamepad button is pressed
#define OPENXR_PRESS_THRESHOLD 0.5f

// Thumbstick value above which the gamepad stick direction button is pressed
#define OPENXR_THUMBSTICK_THRESHOLD 0.25f

namespace
{
    enum class ActionType
    {
        GripPose,
        AimPose,
        Trigger,
        Squeeze,
        Thumbstick,
        ThumbstickClick,
        Bumper,
        ButtonA,
        ButtonB,
        ButtonX,
        ButtonY,
        DPadUp,
        DPadDown,
        DPadLeft,
        DPadRight,
        Menu,
        View,
        Haptic,
        MAX
    };

    struct ActionDesc
    {
        const Char* Name;
        const Char* LocalizedName;
        XrActionType Type;
    };

    struct ActionBinding
    {
        ActionType Action;

        // Path relative to both hands (eg. input/trigger/value) or absolute path (eg. /user/hand/left/input/x/click)
        const char* Path;
    };

    struct ProfileBindings
    {
        const char* Profile;
        const ActionBinding* Bindings;
        int32 BindingsCount;
        bool* IsSupported;
    };

    struct HandState
    {
        XrPath Path = XR_NULL_PATH;
        XrSpace GripSpace = XR_NULL_HANDLE;
        XrSpace AimSpace = XR_NULL_HANDLE;
        Transform GripPose;
        Transform AimPose;
        bool IsGripValid = false;
        bool IsAimValid = false;
        float Trigger = 0.0f;
        float Squeeze = 0.0f;
        Float2 Thumbstick = Float2::Zero;
        bool ThumbstickClick = false;
        bool Bumper = false;
        XrHandTrackerEXT HandTracker = XR_NULL_HANDLE;
        Transform JointPoses[(int32)XRHandJoint::MAX];
        float JointRadii[(int32)XRHandJoint::MAX];
        uint32 ValidJoints = 0;
        XRHandTrackingSource Source = XRHandTrackingSource::Unknown;
    };
    static_assert((int32)XRHandJoint::MAX == XR_HAND_JOINT_COUNT_EXT, "Invalid hand joints count.");

    const ActionDesc Actions[] =
    {
        { TEXT("grip_pose"), TEXT("Grip Pose"), XR_ACTION_TYPE_POSE_INPUT },
        { TEXT("aim_pose"), TEXT("Aim Pose"), XR_ACTION_TYPE_POSE_INPUT },
        { TEXT("trigger"), TEXT("Trigger"), XR_ACTION_TYPE_FLOAT_INPUT },
        { TEXT("squeeze"), TEXT("Squeeze"), XR_ACTION_TYPE_FLOAT_INPUT },
        { TEXT("thumbstick"), TEXT("Thumbstick"), XR_ACTION_TYPE_VECTOR2F_INPUT },
        { TEXT("thumbstick_click"), TEXT("Thumbstick Click"), XR_ACTION_TYPE_BOOLEAN_INPUT },
        { TEXT("bumper"), TEXT("Bumper"), XR_ACTION_TYPE_BOOLEAN_INPUT },
        { TEXT("button_a"), TEXT("Button A"), XR_ACTION_TYPE_BOOLEAN_INPUT },
        { TEXT("button_b"), TEXT("Button B"), XR_ACTION_TYPE_BOOLEAN_INPUT },
        { TEXT("button_x"), TEXT("Button X"), XR_ACTION_TYPE_BOOLEAN_INPUT },
        { TEXT("button_y"), TEXT("Button Y"), XR_ACTION_TYPE_BOOLEAN_INPUT },
        { TEXT("dpad_up"), TEXT("D-Pad Up"), XR_ACTION_TYPE_BOOLEAN_INPUT },
        { TEXT("dpad_down"), TEXT("D-Pad Down"), XR_ACTION_TYPE_BOOLEAN_INPUT },
        { TEXT("dpad_left"), TEXT("D-Pad Left"), XR_ACTION_TYPE_BOOLEAN_INPUT },
        { TEXT("dpad_right"), TEXT("D-Pad Right"), XR_ACTION_TYPE_BOOLEAN_INPUT },
        { TEXT("menu"), TEXT("Menu Button"), XR_ACTION_TYPE_BOOLEAN_INPUT },
        { TEXT("view"), TEXT("View Button"), XR_ACTION_TYPE_BOOLEAN_INPUT },
        { TEXT("haptic"), TEXT("Haptic"), XR_ACTION_TYPE_VIBRATION_OUTPUT },
    };
    static_assert(ARRAY_COUNT(Actions) == (int32)ActionType::MAX, "Invalid actions count.");

    const ActionBinding SimpleControllerBindings[] =
    {
        { ActionType::GripPose, "input/grip/pose" },
        { ActionType::AimPose, "input/aim/pose" },
        { ActionType::Trigger, "input/select/click" },
        { ActionType::Menu, "input/menu/click" },
        { ActionType::Haptic, "output/haptic" },
    };

    const ActionBinding IndexControllerBindings[] =
    {
        { ActionType::GripPose, "input/grip/pose" },
        { ActionType::AimPose, "input/aim/pose" },
        { ActionType::Trigger, "input/trigger/value" },
        { ActionType::Squeeze, "input/squeeze/value" },
        { ActionType::Thumbstick, "input/thumbstick" },
        { ActionType::ThumbstickClick, "input/thumbstick/click" },
        { ActionType::ButtonX, "/user/hand/left/input/a/click" },
        { ActionType::ButtonY, "/user/hand/left/input/b/click" },
        { ActionType::ButtonA, "/user/hand/right/input/a/click" },
        { ActionType::ButtonB, "/user/hand/right/input/b/click" },
        { ActionType::Haptic, "output/haptic" },
    };

    const ActionBinding TouchControllerBindings[] =
    {
        { ActionType::GripPose, "input/grip/pose" },
        { ActionType::AimPose, "input/aim/pose" },
        { ActionType::Trigger, "input/trigger/value" },
        { ActionType::Squeeze, "input/squeeze/value" },
        { ActionType::Thumbstick, "input/thumbstick" },
        { ActionType::ThumbstickClick, "input/thumbstick/click" },
        { ActionType::ButtonX, "/user/hand/left/input/x/click" },
        { ActionType::ButtonY, "/user/hand/left/input/y/click" },
        { ActionType::Menu, "/user/hand/left/input/menu/click" },
        { ActionType::ButtonA, "/user/hand/right/input/a/click" },
        { ActionType::ButtonB, "/user/hand/right/input/b/click" },
        { ActionType::Haptic, "output/haptic" },
    };

    const ActionBinding ViveControllerBindings[] =
    {
        { ActionType::GripPose, "input/grip/pose" },
        { ActionType::AimPose, "input/aim/pose" },
        { ActionType::Trigger, "input/trigger/value" },
        { ActionType::Squeeze, "input/squeeze/click" },
        { ActionType::Thumbstick, "input/trackpad" },
        { ActionType::ThumbstickClick, "input/trackpad/click" },
        { ActionType::Menu, "input/menu/click" },
        { ActionType::Haptic, "output/haptic" },
    };

    const ActionBinding MotionControllerBindings[] =
    {
        { ActionType::GripPose, "input/grip/pose" },
        { ActionType::AimPose, "input/aim/pose" },
        { ActionType::Trigger, "input/trigger/value" },
        { ActionType::Squeeze, "input/squeeze/click" },
        { ActionType::Thumbstick, "input/thumbstick" },
        { ActionType::ThumbstickClick, "input/thumbstick/click" },
        { ActionType::Menu, "input/menu/click" },
        { ActionType::Haptic, "output/haptic" },
    };

    const ActionBinding FrameControllerBindings[] =
    {
        { ActionType::GripPose, "input/grip/pose" },
        { ActionType::AimPose, "input/aim/pose" },
        { ActionType::Trigger, "input/trigger/value" },
        { ActionType::Squeeze, "input/squeeze/value" },
        { ActionType::Thumbstick, "input/thumbstick" },
        { ActionType::ThumbstickClick, "input/thumbstick/click" },
        { ActionType::Bumper, "input/bumper/click" },
        { ActionType::DPadUp, "/user/hand/left/input/dpad_up/click" },
        { ActionType::DPadDown, "/user/hand/left/input/dpad_down/click" },
        { ActionType::DPadLeft, "/user/hand/left/input/dpad_left/click" },
        { ActionType::DPadRight, "/user/hand/left/input/dpad_right/click" },
        { ActionType::View, "/user/hand/left/input/view/click" },
        { ActionType::ButtonA, "/user/hand/right/input/a/click" },
        { ActionType::ButtonB, "/user/hand/right/input/b/click" },
        { ActionType::ButtonX, "/user/hand/right/input/x/click" },
        { ActionType::ButtonY, "/user/hand/right/input/y/click" },
        { ActionType::Menu, "/user/hand/right/input/menu/click" },
        { ActionType::Haptic, "output/haptic" },
    };

#define PROFILE_BINDINGS(profile, bindings, isSupported) { profile, bindings, ARRAY_COUNT(bindings), isSupported }
    const ProfileBindings Profiles[] =
    {
        PROFILE_BINDINGS("/interaction_profiles/khr/simple_controller", SimpleControllerBindings, nullptr),
        PROFILE_BINDINGS("/interaction_profiles/valve/index_controller", IndexControllerBindings, nullptr),
        PROFILE_BINDINGS("/interaction_profiles/oculus/touch_controller", TouchControllerBindings, nullptr),
        PROFILE_BINDINGS("/interaction_profiles/htc/vive_controller", ViveControllerBindings, nullptr),
        PROFILE_BINDINGS("/interaction_profiles/microsoft/motion_controller", MotionControllerBindings, nullptr),
        PROFILE_BINDINGS("/interaction_profiles/valve/frame_controller_valve", FrameControllerBindings, &OpenXRInstance::SupportsFrameController),
    };
#undef PROFILE_BINDINGS

    const char* HandPaths[] = { "/user/hand/left", "/user/hand/right" };

    XrActionSet ActionSet = XR_NULL_HANDLE;
    XrAction ActionHandles[(int32)ActionType::MAX] = {};
    XrSession Session = XR_NULL_HANDLE;
    XrSpace Space = XR_NULL_HANDLE;
    XrSpace HeadSpace = XR_NULL_HANDLE;
    Transform HeadPose;
    bool IsHeadValid = false;
    HandState Hands[2];
    bool Buttons[(int32)GamepadButton::MAX] = {};
    Gamepad* GamepadDevice = nullptr;
    PFN_xrCreateHandTrackerEXT CreateHandTrackerEXT = nullptr;
    PFN_xrDestroyHandTrackerEXT DestroyHandTrackerEXT = nullptr;
    PFN_xrLocateHandJointsEXT LocateHandJointsEXT = nullptr;

    class OpenXRGamepad : public Gamepad
    {
    public:
        OpenXRGamepad()
            : Gamepad(Guid(0x4f70656e, 0x58520000, 0, 0), TEXT("OpenXR Controllers"))
        {
        }

        ~OpenXRGamepad()
        {
            GamepadDevice = nullptr;
        }

    protected:
        // [Gamepad]
        bool UpdateState() override
        {
            // Disconnect when the session stops
            if (!OpenXRSession::IsRunning())
                return true;

            // Process buttons state
            const HandState& left = Hands[0];
            const HandState& right = Hands[1];
            Platform::MemoryCopy(_state.Buttons, Buttons, sizeof(Buttons));
            _state.Buttons[(int32)GamepadButton::LeftShoulder] = left.Squeeze > OPENXR_PRESS_THRESHOLD || left.Bumper;
            _state.Buttons[(int32)GamepadButton::RightShoulder] = right.Squeeze > OPENXR_PRESS_THRESHOLD || right.Bumper;
            _state.Buttons[(int32)GamepadButton::LeftThumb] = left.ThumbstickClick;
            _state.Buttons[(int32)GamepadButton::RightThumb] = right.ThumbstickClick;
            _state.Buttons[(int32)GamepadButton::LeftTrigger] = left.Trigger > OPENXR_PRESS_THRESHOLD;
            _state.Buttons[(int32)GamepadButton::RightTrigger] = right.Trigger > OPENXR_PRESS_THRESHOLD;
            _state.Buttons[(int32)GamepadButton::LeftStickUp] = left.Thumbstick.Y > OPENXR_THUMBSTICK_THRESHOLD;
            _state.Buttons[(int32)GamepadButton::LeftStickDown] = left.Thumbstick.Y < -OPENXR_THUMBSTICK_THRESHOLD;
            _state.Buttons[(int32)GamepadButton::LeftStickLeft] = left.Thumbstick.X < -OPENXR_THUMBSTICK_THRESHOLD;
            _state.Buttons[(int32)GamepadButton::LeftStickRight] = left.Thumbstick.X > OPENXR_THUMBSTICK_THRESHOLD;
            _state.Buttons[(int32)GamepadButton::RightStickUp] = right.Thumbstick.Y > OPENXR_THUMBSTICK_THRESHOLD;
            _state.Buttons[(int32)GamepadButton::RightStickDown] = right.Thumbstick.Y < -OPENXR_THUMBSTICK_THRESHOLD;
            _state.Buttons[(int32)GamepadButton::RightStickLeft] = right.Thumbstick.X < -OPENXR_THUMBSTICK_THRESHOLD;
            _state.Buttons[(int32)GamepadButton::RightStickRight] = right.Thumbstick.X > OPENXR_THUMBSTICK_THRESHOLD;

            // Process axes state
            _state.Axis[(int32)GamepadAxis::LeftStickX] = left.Thumbstick.X;
            _state.Axis[(int32)GamepadAxis::LeftStickY] = left.Thumbstick.Y;
            _state.Axis[(int32)GamepadAxis::RightStickX] = right.Thumbstick.X;
            _state.Axis[(int32)GamepadAxis::RightStickY] = right.Thumbstick.Y;
            _state.Axis[(int32)GamepadAxis::LeftTrigger] = left.Trigger;
            _state.Axis[(int32)GamepadAxis::RightTrigger] = right.Trigger;

            return false;
        }
    };

    XrAction GetAction(ActionType type)
    {
        return ActionHandles[(int32)type];
    }

    float GetFloat(ActionType type, XrPath subactionPath)
    {
        XrActionStateGetInfo getInfo = { XR_TYPE_ACTION_STATE_GET_INFO };
        getInfo.action = GetAction(type);
        getInfo.subactionPath = subactionPath;
        XrActionStateFloat state = { XR_TYPE_ACTION_STATE_FLOAT };
        return XR_SUCCEEDED(xrGetActionStateFloat(Session, &getInfo, &state)) && state.isActive ? state.currentState : 0.0f;
    }

    Float2 GetVector2(ActionType type, XrPath subactionPath)
    {
        XrActionStateGetInfo getInfo = { XR_TYPE_ACTION_STATE_GET_INFO };
        getInfo.action = GetAction(type);
        getInfo.subactionPath = subactionPath;
        XrActionStateVector2f state = { XR_TYPE_ACTION_STATE_VECTOR2F };
        return XR_SUCCEEDED(xrGetActionStateVector2f(Session, &getInfo, &state)) && state.isActive ? Float2(state.currentState.x, state.currentState.y) : Float2::Zero;
    }

    bool GetBool(ActionType type, XrPath subactionPath)
    {
        XrActionStateGetInfo getInfo = { XR_TYPE_ACTION_STATE_GET_INFO };
        getInfo.action = GetAction(type);
        getInfo.subactionPath = subactionPath;
        XrActionStateBoolean state = { XR_TYPE_ACTION_STATE_BOOLEAN };
        return XR_SUCCEEDED(xrGetActionStateBoolean(Session, &getInfo, &state)) && state.isActive && state.currentState;
    }

    bool LocateSpace(XrSpace space, XrTime time, Transform& pose)
    {
        if (space == XR_NULL_HANDLE)
            return false;
        XrSpaceLocation location = { XR_TYPE_SPACE_LOCATION };
        const XrResult result = xrLocateSpace(space, Space, time, &location);
        const XrSpaceLocationFlags validFlags = XR_SPACE_LOCATION_ORIENTATION_VALID_BIT | XR_SPACE_LOCATION_POSITION_VALID_BIT;
        if (XR_FAILED(result) || (location.locationFlags & validFlags) != validFlags)
            return false;
        pose = OpenXRConvert::ToTransform(location.pose);
        return true;
    }

    void ClearHandState(HandState& hand)
    {
        const XrPath path = hand.Path;
        hand = HandState();
        hand.Path = path;
    }

    void InitHandTracking(XrSession session)
    {
        if (CreateHandTrackerEXT == nullptr)
            return;

        // Check if headset supports hand tracking
        XrSystemHandTrackingPropertiesEXT handTrackingProperties = { XR_TYPE_SYSTEM_HAND_TRACKING_PROPERTIES_EXT };
        XrSystemProperties systemProperties = { XR_TYPE_SYSTEM_PROPERTIES };
        systemProperties.next = &handTrackingProperties;
        XrResult result = xrGetSystemProperties(OpenXRInstance::Instance, OpenXRInstance::SystemId, &systemProperties);
        if (XR_FAILED(result) || !handTrackingProperties.supportsHandTracking)
        {
            LOG(Info, "OpenXR hand tracking is not supported by the system");
            return;
        }

        // Create hand trackers (use both the hands tracking and the controllers to estimate the hand pose)
        XrHandTrackingDataSourceEXT dataSources[] = { XR_HAND_TRACKING_DATA_SOURCE_UNOBSTRUCTED_EXT, XR_HAND_TRACKING_DATA_SOURCE_CONTROLLER_EXT };
        XrHandTrackingDataSourceInfoEXT dataSourceInfo = { XR_TYPE_HAND_TRACKING_DATA_SOURCE_INFO_EXT };
        dataSourceInfo.requestedDataSourceCount = ARRAY_COUNT(dataSources);
        dataSourceInfo.requestedDataSources = dataSources;
        for (int32 i = 0; i < 2; i++)
        {
            XrHandTrackerCreateInfoEXT createInfo = { XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT };
            createInfo.next = OpenXRInstance::SupportsHandTrackingDataSource ? &dataSourceInfo : nullptr;
            createInfo.hand = i == 0 ? XR_HAND_LEFT_EXT : XR_HAND_RIGHT_EXT;
            createInfo.handJointSet = XR_HAND_JOINT_SET_DEFAULT_EXT;
            result = CreateHandTrackerEXT(session, &createInfo, &Hands[i].HandTracker);
            if (XR_FAILED(result))
            {
                LOG(Warning, "Failed to create OpenXR hand tracker ({0})", OpenXRInstance::GetResultString(result));
                Hands[i].HandTracker = XR_NULL_HANDLE;
            }
        }
        LOG(Info, "OpenXR hand tracking enabled (motion range: {0}, data source: {1})", OpenXRInstance::SupportsHandJointsMotionRange, OpenXRInstance::SupportsHandTrackingDataSource);
    }

    void LocateHandJoints(HandState& hand, XrTime time)
    {
        hand.ValidJoints = 0;
        hand.Source = XRHandTrackingSource::Unknown;
        if (hand.HandTracker == XR_NULL_HANDLE)
            return;
        XrHandJointsLocateInfoEXT locateInfo = { XR_TYPE_HAND_JOINTS_LOCATE_INFO_EXT };
        locateInfo.baseSpace = Space;
        locateInfo.time = time;
        XrHandJointsMotionRangeInfoEXT motionRangeInfo = { XR_TYPE_HAND_JOINTS_MOTION_RANGE_INFO_EXT };
        if (OpenXRInstance::SupportsHandJointsMotionRange)
        {
            motionRangeInfo.handJointsMotionRange = XR::GetHandMotionRange() == XRHandMotionRange::ConformingToController ? XR_HAND_JOINTS_MOTION_RANGE_CONFORMING_TO_CONTROLLER_EXT : XR_HAND_JOINTS_MOTION_RANGE_UNOBSTRUCTED_EXT;
            locateInfo.next = &motionRangeInfo;
        }
        XrHandJointLocationEXT jointLocations[XR_HAND_JOINT_COUNT_EXT];
        XrHandJointLocationsEXT locations = { XR_TYPE_HAND_JOINT_LOCATIONS_EXT };
        locations.jointCount = XR_HAND_JOINT_COUNT_EXT;
        locations.jointLocations = jointLocations;
        XrHandTrackingDataSourceStateEXT dataSourceState = { XR_TYPE_HAND_TRACKING_DATA_SOURCE_STATE_EXT };
        if (OpenXRInstance::SupportsHandTrackingDataSource)
            locations.next = &dataSourceState;
        if (XR_FAILED(LocateHandJointsEXT(hand.HandTracker, &locateInfo, &locations)) || !locations.isActive)
            return;
        const XrSpaceLocationFlags validFlags = XR_SPACE_LOCATION_ORIENTATION_VALID_BIT | XR_SPACE_LOCATION_POSITION_VALID_BIT;
        for (int32 i = 0; i < XR_HAND_JOINT_COUNT_EXT; i++)
        {
            const XrHandJointLocationEXT& location = jointLocations[i];
            if ((location.locationFlags & validFlags) != validFlags)
                continue;
            hand.JointPoses[i] = OpenXRConvert::ToTransform(location.pose);
            hand.JointRadii[i] = location.radius * METERS_TO_UNITS_SCALE;
            hand.ValidJoints |= 1u << i;
        }
        if (dataSourceState.isActive)
            hand.Source = dataSourceState.dataSource == XR_HAND_TRACKING_DATA_SOURCE_CONTROLLER_EXT ? XRHandTrackingSource::Controller : XRHandTrackingSource::Unobstructed;
    }
}

bool OpenXRInput::Init(XrInstance instance)
{
    // Get hand tracking functions
    if (OpenXRInstance::SupportsHandTracking)
    {
        xrGetInstanceProcAddr(instance, "xrCreateHandTrackerEXT", (PFN_xrVoidFunction*)&CreateHandTrackerEXT);
        xrGetInstanceProcAddr(instance, "xrDestroyHandTrackerEXT", (PFN_xrVoidFunction*)&DestroyHandTrackerEXT);
        xrGetInstanceProcAddr(instance, "xrLocateHandJointsEXT", (PFN_xrVoidFunction*)&LocateHandJointsEXT);
        if (DestroyHandTrackerEXT == nullptr || LocateHandJointsEXT == nullptr)
            CreateHandTrackerEXT = nullptr;
    }

    // Create action set
    XrActionSetCreateInfo actionSetInfo = { XR_TYPE_ACTION_SET_CREATE_INFO };
    StringUtils::Copy(actionSetInfo.actionSetName, TEXT("gameplay"), XR_MAX_ACTION_SET_NAME_SIZE - 1);
    StringUtils::Copy(actionSetInfo.localizedActionSetName, TEXT("Gameplay"), XR_MAX_LOCALIZED_ACTION_SET_NAME_SIZE - 1);
    XrResult result = xrCreateActionSet(instance, &actionSetInfo, &ActionSet);
    if (XR_FAILED(result))
    {
        LOG(Warning, "Failed to create OpenXR action set ({0})", OpenXRInstance::GetResultString(result));
        ActionSet = XR_NULL_HANDLE;
        return true;
    }

    // Create actions (each one is used by both hands)
    XrPath handPaths[2];
    for (int32 i = 0; i < 2; i++)
    {
        xrStringToPath(instance, HandPaths[i], &handPaths[i]);
        Hands[i].Path = handPaths[i];
    }
    for (int32 i = 0; i < (int32)ActionType::MAX; i++)
    {
        const ActionDesc& desc = Actions[i];
        XrActionCreateInfo actionInfo = { XR_TYPE_ACTION_CREATE_INFO };
        StringUtils::Copy(actionInfo.actionName, desc.Name, XR_MAX_ACTION_NAME_SIZE - 1);
        StringUtils::Copy(actionInfo.localizedActionName, desc.LocalizedName, XR_MAX_LOCALIZED_ACTION_NAME_SIZE - 1);
        actionInfo.actionType = desc.Type;
        actionInfo.countSubactionPaths = 2;
        actionInfo.subactionPaths = handPaths;
        result = xrCreateAction(ActionSet, &actionInfo, &ActionHandles[i]);
        if (XR_FAILED(result))
        {
            LOG(Warning, "Failed to create OpenXR action '{0}' ({1})", desc.Name, OpenXRInstance::GetResultString(result));
            Dispose();
            return true;
        }
    }

    // Suggest bindings for each supported controller (runtime uses the ones that match the connected controllers)
    Array<XrActionSuggestedBinding> suggestedBindings;
    StringAnsi path;
    for (const ProfileBindings& profile : Profiles)
    {
        if (profile.IsSupported && !*profile.IsSupported)
            continue;
        suggestedBindings.Clear();
        for (int32 i = 0; i < profile.BindingsCount; i++)
        {
            const ActionBinding& binding = profile.Bindings[i];
            for (int32 hand = 0; hand < 2; hand++)
            {
                if (binding.Path[0] == '/')
                {
                    if (hand != 0)
                        break;
                    path = binding.Path;
                }
                else
                {
                    path = HandPaths[hand];
                    path += '/';
                    path += binding.Path;
                }
                XrActionSuggestedBinding& suggestedBinding = suggestedBindings.AddOne();
                suggestedBinding.action = GetAction(binding.Action);
                xrStringToPath(instance, path.Get(), &suggestedBinding.binding);
            }
        }
        XrInteractionProfileSuggestedBinding profileInfo = { XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
        xrStringToPath(instance, profile.Profile, &profileInfo.interactionProfile);
        profileInfo.countSuggestedBindings = (uint32)suggestedBindings.Count();
        profileInfo.suggestedBindings = suggestedBindings.Get();
        result = xrSuggestInteractionProfileBindings(instance, &profileInfo);
        if (XR_FAILED(result))
            LOG(Warning, "Failed to suggest OpenXR bindings for {0} ({1})", String(profile.Profile), OpenXRInstance::GetResultString(result));
    }

    return false;
}

bool OpenXRInput::IsInitialized()
{
    return ActionSet != XR_NULL_HANDLE;
}

bool OpenXRInput::InitSession(XrSession session, XrSpace space)
{
    Session = session;
    Space = space;

    // Create head space
    XrReferenceSpaceCreateInfo headSpaceInfo = { XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
    headSpaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
    headSpaceInfo.poseInReferenceSpace.orientation.w = 1.0f;
    XrResult result = xrCreateReferenceSpace(session, &headSpaceInfo, &HeadSpace);
    if (XR_FAILED(result))
    {
        LOG(Warning, "Failed to create OpenXR head space ({0})", OpenXRInstance::GetResultString(result));
        HeadSpace = XR_NULL_HANDLE;
        return true;
    }

    // Setup hand tracking (works without the actions)
    InitHandTracking(session);
    if (ActionSet == XR_NULL_HANDLE)
        return true;

    // Attach action set (bindings cannot be changed after that)
    XrSessionActionSetsAttachInfo attachInfo = { XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO };
    attachInfo.countActionSets = 1;
    attachInfo.actionSets = &ActionSet;
    result = xrAttachSessionActionSets(session, &attachInfo);
    if (XR_FAILED(result))
    {
        LOG(Warning, "Failed to attach OpenXR action set ({0})", OpenXRInstance::GetResultString(result));
        return true;
    }

    // Create pose spaces for each hand
    for (auto& hand : Hands)
    {
        XrActionSpaceCreateInfo spaceInfo = { XR_TYPE_ACTION_SPACE_CREATE_INFO };
        spaceInfo.subactionPath = hand.Path;
        spaceInfo.poseInActionSpace.orientation.w = 1.0f;
        spaceInfo.action = GetAction(ActionType::GripPose);
        result = xrCreateActionSpace(session, &spaceInfo, &hand.GripSpace);
        if (XR_SUCCEEDED(result))
        {
            spaceInfo.action = GetAction(ActionType::AimPose);
            result = xrCreateActionSpace(session, &spaceInfo, &hand.AimSpace);
        }
        if (XR_FAILED(result))
        {
            LOG(Warning, "Failed to create OpenXR hand space ({0})", OpenXRInstance::GetResultString(result));
            return true;
        }
    }

    return false;
}

void OpenXRInput::DisposeSession()
{
    for (auto& hand : Hands)
    {
        if (hand.GripSpace != XR_NULL_HANDLE)
            xrDestroySpace(hand.GripSpace);
        if (hand.AimSpace != XR_NULL_HANDLE)
            xrDestroySpace(hand.AimSpace);
        if (hand.HandTracker != XR_NULL_HANDLE)
            DestroyHandTrackerEXT(hand.HandTracker);
        ClearHandState(hand);
    }
    Platform::MemoryClear(Buttons, sizeof(Buttons));
    if (HeadSpace != XR_NULL_HANDLE)
    {
        xrDestroySpace(HeadSpace);
        HeadSpace = XR_NULL_HANDLE;
    }
    IsHeadValid = false;
    Session = XR_NULL_HANDLE;
    Space = XR_NULL_HANDLE;
}

void OpenXRInput::Dispose()
{
    if (ActionSet != XR_NULL_HANDLE)
    {
        // Destroys the actions too
        xrDestroyActionSet(ActionSet);
        ActionSet = XR_NULL_HANDLE;
    }
    for (auto& action : ActionHandles)
        action = XR_NULL_HANDLE;
    CreateHandTrackerEXT = nullptr;
    DestroyHandTrackerEXT = nullptr;
    LocateHandJointsEXT = nullptr;
    for (auto& hand : Hands)
        hand.Path = XR_NULL_PATH;
}

void OpenXRInput::Update(XrTime time)
{
    if (Session == XR_NULL_HANDLE)
        return;
    PROFILE_CPU_NAMED("XR.Input");

    // Add gamepad for the controllers (removed when the session stops)
    if (GamepadDevice == nullptr && Input::Gamepads.Count() < MAX_GAMEPADS)
    {
        GamepadDevice = New<OpenXRGamepad>();
        Input::Gamepads.Add(GamepadDevice);
        Input::OnGamepadsChanged();
    }

    // Get actions state (inactive if the session is not focused)
    if (ActionSet != XR_NULL_HANDLE)
    {
        XrActiveActionSet activeActionSet = { ActionSet, XR_NULL_PATH };
        XrActionsSyncInfo syncInfo = { XR_TYPE_ACTIONS_SYNC_INFO };
        syncInfo.countActiveActionSets = 1;
        syncInfo.activeActionSets = &activeActionSet;
        xrSyncActions(Session, &syncInfo);
        for (auto& hand : Hands)
        {
            hand.Trigger = GetFloat(ActionType::Trigger, hand.Path);
            hand.Squeeze = GetFloat(ActionType::Squeeze, hand.Path);
            hand.Thumbstick = GetVector2(ActionType::Thumbstick, hand.Path);
            hand.ThumbstickClick = GetBool(ActionType::ThumbstickClick, hand.Path);
            hand.Bumper = GetBool(ActionType::Bumper, hand.Path);
        }

        // Get buttons of both hands
        Buttons[(int32)GamepadButton::A] = GetBool(ActionType::ButtonA, XR_NULL_PATH);
        Buttons[(int32)GamepadButton::B] = GetBool(ActionType::ButtonB, XR_NULL_PATH);
        Buttons[(int32)GamepadButton::X] = GetBool(ActionType::ButtonX, XR_NULL_PATH);
        Buttons[(int32)GamepadButton::Y] = GetBool(ActionType::ButtonY, XR_NULL_PATH);
        Buttons[(int32)GamepadButton::DPadUp] = GetBool(ActionType::DPadUp, XR_NULL_PATH);
        Buttons[(int32)GamepadButton::DPadDown] = GetBool(ActionType::DPadDown, XR_NULL_PATH);
        Buttons[(int32)GamepadButton::DPadLeft] = GetBool(ActionType::DPadLeft, XR_NULL_PATH);
        Buttons[(int32)GamepadButton::DPadRight] = GetBool(ActionType::DPadRight, XR_NULL_PATH);
        Buttons[(int32)GamepadButton::Start] = GetBool(ActionType::Menu, XR_NULL_PATH);
        Buttons[(int32)GamepadButton::Back] = GetBool(ActionType::View, XR_NULL_PATH);
    }

    LocatePoses(time);
}

void OpenXRInput::LocatePoses(XrTime time)
{
    if (Session == XR_NULL_HANDLE)
        return;
    IsHeadValid = LocateSpace(HeadSpace, time, HeadPose);
    for (auto& hand : Hands)
    {
        hand.IsGripValid = LocateSpace(hand.GripSpace, time, hand.GripPose);
        hand.IsAimValid = LocateSpace(hand.AimSpace, time, hand.AimPose);
        LocateHandJoints(hand, time);
    }
}

void OpenXRInput::LogInteractionProfiles()
{
    if (Session == XR_NULL_HANDLE || ActionSet == XR_NULL_HANDLE)
        return;
    for (int32 i = 0; i < 2; i++)
    {
        XrInteractionProfileState state = { XR_TYPE_INTERACTION_PROFILE_STATE };
        char profile[XR_MAX_PATH_LENGTH] = "none";
        uint32 length = 0;
        if (XR_SUCCEEDED(xrGetCurrentInteractionProfile(Session, Hands[i].Path, &state)) && state.interactionProfile != XR_NULL_PATH)
            xrPathToString(OpenXRInstance::Instance, state.interactionProfile, XR_MAX_PATH_LENGTH, &length, profile);
        LOG(Info, "OpenXR interaction profile for {0}: {1}", String(HandPaths[i]), String(profile));
    }
}

bool OpenXRInput::GetHeadPose(Transform& pose)
{
    if (!IsHeadValid)
        return false;
    pose = HeadPose;
    return true;
}

bool OpenXRInput::GetHandPose(XRHand hand, XRPoseType type, Transform& pose)
{
    if ((uint32)hand > (uint32)XRHand::Right)
        return false;
    const HandState& state = Hands[(int32)hand];
    if (type == XRPoseType::Aim ? !state.IsAimValid : !state.IsGripValid)
        return false;
    pose = type == XRPoseType::Aim ? state.AimPose : state.GripPose;
    return true;
}

bool OpenXRInput::IsHandTrackingSupported()
{
    return Hands[0].HandTracker != XR_NULL_HANDLE || Hands[1].HandTracker != XR_NULL_HANDLE;
}

bool OpenXRInput::GetHandJointPose(XRHand hand, XRHandJoint joint, Transform& pose, float& radius)
{
    if ((uint32)hand > (uint32)XRHand::Right || (uint32)joint >= (uint32)XRHandJoint::MAX)
        return false;
    const HandState& state = Hands[(int32)hand];
    if ((state.ValidJoints & (1u << (uint32)joint)) == 0)
        return false;
    pose = state.JointPoses[(int32)joint];
    radius = state.JointRadii[(int32)joint];
    return true;
}

XRHandTrackingSource OpenXRInput::GetHandTrackingSource(XRHand hand)
{
    if ((uint32)hand > (uint32)XRHand::Right || Hands[(int32)hand].ValidJoints == 0)
        return XRHandTrackingSource::Unknown;
    return Hands[(int32)hand].Source;
}

void OpenXRInput::Vibrate(XRHand hand, float amplitude, float duration, float frequency)
{
    if (Session == XR_NULL_HANDLE || ActionSet == XR_NULL_HANDLE || (uint32)hand > (uint32)XRHand::Right)
        return;
    XrHapticActionInfo hapticInfo = { XR_TYPE_HAPTIC_ACTION_INFO };
    hapticInfo.action = GetAction(ActionType::Haptic);
    hapticInfo.subactionPath = Hands[(int32)hand].Path;
    if (amplitude <= 0.0f)
    {
        xrStopHapticFeedback(Session, &hapticInfo);
        return;
    }
    XrHapticVibration vibration = { XR_TYPE_HAPTIC_VIBRATION };
    vibration.amplitude = Math::Saturate(amplitude);
    vibration.duration = duration > 0.0f ? (XrDuration)((double)duration * 1000000000.0) : XR_MIN_HAPTIC_DURATION;
    vibration.frequency = frequency > 0.0f ? frequency : XR_FREQUENCY_UNSPECIFIED;
    xrApplyHapticFeedback(Session, &hapticInfo, (const XrHapticBaseHeader*)&vibration);
}

Gamepad* OpenXRInput::GetGamepad()
{
    return GamepadDevice;
}

#endif
