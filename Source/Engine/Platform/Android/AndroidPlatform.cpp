// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_ANDROID

#include "AndroidPlatform.h"
#include "AndroidWindow.h"
#include "AndroidFileSystem.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/Guid.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Types/Version.h"
#include "Engine/Core/Collections/HashFunctions.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Math/Rectangle.h"
#include "Engine/Platform/CPUInfo.h"
#include "Engine/Platform/MemoryStats.h"
#include "Engine/Platform/MessageBox.h"
#include "Engine/Platform/WindowsManager.h"
#include "Engine/Utilities/StringConverter.h"
#include "Engine/Graphics/GPUSwapChain.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Input/Input.h"
#include "Engine/Input/Gamepad.h"
#include "Engine/Input/Keyboard.h"
#include "Engine/Input/Mouse.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include <sys/resource.h>
#include <sys/sysinfo.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/unistd.h>
#include <pthread.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sched.h>
#include <string.h>
#include <time.h>
#include <net/if.h>
#include <errno.h>
#include <locale.h>
#include <inttypes.h>
#include <pwd.h>
#include <jni.h>
#if !BUILD_RELEASE
#include <android/log.h>
#endif
#include "Engine/Main/Android/android_native_app_glue.h"
#include <android/window.h>
#include <android/versioning.h>

#if CRASH_LOG_ENABLE

#include <unwind.h>
#include <cxxabi.h>

struct AndroidBacktraceState
{
    void** Current;
    void** End;
};

_Unwind_Reason_Code AndroidUnwindCallback(struct _Unwind_Context* context, void* arg)
{
    AndroidBacktraceState* state = (AndroidBacktraceState*)arg;
    uintptr_t pc = _Unwind_GetIP(context);
    if (pc)
    {
        if (state->Current == state->End)
            return _URC_END_OF_STACK;
        else
            *state->Current++ = reinterpret_cast<void*>(pc);
    }
    return _URC_NO_REASON;
}

#endif

struct AndroidKeyEventType
{
    uint32_t KeyCode;
    KeyboardKeys KeyboardKey;
    GamepadButton GamepadButton;
};

AndroidKeyEventType AndroidKeyEventTypes[] =
{
    { AKEYCODE_HOME, KeyboardKeys::Home, GamepadButton::Start },
    { AKEYCODE_BACK, KeyboardKeys::None, GamepadButton::Back },
    { AKEYCODE_0, KeyboardKeys::Alpha0, GamepadButton::None },
    { AKEYCODE_1, KeyboardKeys::Alpha1, GamepadButton::None },
    { AKEYCODE_2, KeyboardKeys::Alpha2, GamepadButton::None },
    { AKEYCODE_3, KeyboardKeys::Alpha3, GamepadButton::None },
    { AKEYCODE_4, KeyboardKeys::Alpha4, GamepadButton::None },
    { AKEYCODE_5, KeyboardKeys::Alpha5, GamepadButton::None },
    { AKEYCODE_6, KeyboardKeys::Alpha6, GamepadButton::None },
    { AKEYCODE_7, KeyboardKeys::Alpha7, GamepadButton::None },
    { AKEYCODE_8, KeyboardKeys::Alpha8, GamepadButton::None },
    { AKEYCODE_9, KeyboardKeys::Alpha9, GamepadButton::None },
    { AKEYCODE_STAR, KeyboardKeys::NumpadMultiply, GamepadButton::None },
    { AKEYCODE_DPAD_UP, KeyboardKeys::None, GamepadButton::DPadUp },
    { AKEYCODE_DPAD_DOWN, KeyboardKeys::None, GamepadButton::DPadDown },
    { AKEYCODE_DPAD_LEFT, KeyboardKeys::None, GamepadButton::DPadLeft },
    { AKEYCODE_DPAD_RIGHT, KeyboardKeys::None, GamepadButton::DPadRight },
    { AKEYCODE_VOLUME_UP, KeyboardKeys::VolumeUp, GamepadButton::None },
    { AKEYCODE_VOLUME_DOWN, KeyboardKeys::VolumeDown, GamepadButton::None },
    { AKEYCODE_CLEAR, KeyboardKeys::Clear, GamepadButton::None },
    { AKEYCODE_A, KeyboardKeys::A, GamepadButton::None },
    { AKEYCODE_B, KeyboardKeys::B, GamepadButton::None },
    { AKEYCODE_C, KeyboardKeys::C, GamepadButton::None },
    { AKEYCODE_D, KeyboardKeys::D, GamepadButton::None },
    { AKEYCODE_E, KeyboardKeys::E, GamepadButton::None },
    { AKEYCODE_F, KeyboardKeys::F, GamepadButton::None },
    { AKEYCODE_G, KeyboardKeys::G, GamepadButton::None },
    { AKEYCODE_H, KeyboardKeys::H, GamepadButton::None },
    { AKEYCODE_I, KeyboardKeys::I, GamepadButton::None },
    { AKEYCODE_J, KeyboardKeys::J, GamepadButton::None },
    { AKEYCODE_K, KeyboardKeys::K, GamepadButton::None },
    { AKEYCODE_L, KeyboardKeys::L, GamepadButton::None },
    { AKEYCODE_M, KeyboardKeys::M, GamepadButton::None },
    { AKEYCODE_N, KeyboardKeys::N, GamepadButton::None },
    { AKEYCODE_O, KeyboardKeys::O, GamepadButton::None },
    { AKEYCODE_P, KeyboardKeys::P, GamepadButton::None },
    { AKEYCODE_Q, KeyboardKeys::Q, GamepadButton::None },
    { AKEYCODE_R, KeyboardKeys::R, GamepadButton::None },
    { AKEYCODE_S, KeyboardKeys::S, GamepadButton::None },
    { AKEYCODE_T, KeyboardKeys::T, GamepadButton::None },
    { AKEYCODE_U, KeyboardKeys::U, GamepadButton::None },
    { AKEYCODE_V, KeyboardKeys::V, GamepadButton::None },
    { AKEYCODE_W, KeyboardKeys::W, GamepadButton::None },
    { AKEYCODE_Y, KeyboardKeys::Y, GamepadButton::None },
    { AKEYCODE_Z, KeyboardKeys::Z, GamepadButton::None },
    { AKEYCODE_COMMA, KeyboardKeys::Comma, GamepadButton::None },
    { AKEYCODE_PERIOD, KeyboardKeys::Period, GamepadButton::None },
    { AKEYCODE_ALT_LEFT, KeyboardKeys::Alt, GamepadButton::None },
    { AKEYCODE_ALT_RIGHT, KeyboardKeys::Alt, GamepadButton::None },
    { AKEYCODE_SHIFT_LEFT, KeyboardKeys::Shift, GamepadButton::None },
    { AKEYCODE_SHIFT_RIGHT, KeyboardKeys::Shift, GamepadButton::None },
    { AKEYCODE_TAB, KeyboardKeys::Tab, GamepadButton::None },
    { AKEYCODE_SPACE, KeyboardKeys::Spacebar, GamepadButton::None },
    { AKEYCODE_ENTER, KeyboardKeys::Return, GamepadButton::None },
    { AKEYCODE_DEL, KeyboardKeys::Delete, GamepadButton::None },
    { AKEYCODE_GRAVE, KeyboardKeys::BackQuote, GamepadButton::None },
    { AKEYCODE_MINUS, KeyboardKeys::Minus, GamepadButton::None },
    { AKEYCODE_PLUS, KeyboardKeys::Plus, GamepadButton::None },
    { AKEYCODE_LEFT_BRACKET, KeyboardKeys::LeftBracket, GamepadButton::None },
    { AKEYCODE_RIGHT_BRACKET, KeyboardKeys::RightBracket, GamepadButton::None },
    { AKEYCODE_BACKSLASH, KeyboardKeys::Backslash, GamepadButton::None },
    { AKEYCODE_SEMICOLON, KeyboardKeys::Colon, GamepadButton::None },
    { AKEYCODE_SLASH, KeyboardKeys::Slash, GamepadButton::None },
    { AKEYCODE_NUM, KeyboardKeys::Numlock, GamepadButton::None },
    { AKEYCODE_MENU, KeyboardKeys::LeftMenu, GamepadButton::None },
    { AKEYCODE_MEDIA_PLAY_PAUSE, KeyboardKeys::MediaPlayPause, GamepadButton::None },
    { AKEYCODE_MEDIA_STOP, KeyboardKeys::MediaStop, GamepadButton::None },
    { AKEYCODE_MEDIA_NEXT, KeyboardKeys::MediaNextTrack, GamepadButton::None },
    { AKEYCODE_MEDIA_PREVIOUS, KeyboardKeys::MediaPrevTrack, GamepadButton::None },
    { AKEYCODE_MUTE, KeyboardKeys::VolumeMute, GamepadButton::None },
    { AKEYCODE_PAGE_UP, KeyboardKeys::PageUp, GamepadButton::None },
    { AKEYCODE_PAGE_DOWN, KeyboardKeys::PageDown, GamepadButton::None },
    { AKEYCODE_BUTTON_A, KeyboardKeys::None, GamepadButton::A },
    { AKEYCODE_BUTTON_A, KeyboardKeys::None, GamepadButton::B },
    { AKEYCODE_BUTTON_X, KeyboardKeys::None, GamepadButton::X },
    { AKEYCODE_BUTTON_Y, KeyboardKeys::None, GamepadButton::Y },
    { AKEYCODE_BUTTON_L1, KeyboardKeys::None, GamepadButton::LeftShoulder },
    { AKEYCODE_BUTTON_R1, KeyboardKeys::None, GamepadButton::RightShoulder },
    { AKEYCODE_BUTTON_L2, KeyboardKeys::None, GamepadButton::LeftTrigger },
    { AKEYCODE_BUTTON_R2, KeyboardKeys::None, GamepadButton::RightTrigger },
    { AKEYCODE_BUTTON_THUMBL, KeyboardKeys::None, GamepadButton::LeftThumb },
    { AKEYCODE_BUTTON_THUMBR, KeyboardKeys::None, GamepadButton::RightThumb },
    { AKEYCODE_BUTTON_START, KeyboardKeys::None, GamepadButton::Start },
    { AKEYCODE_BUTTON_SELECT, KeyboardKeys::None, GamepadButton::Start },
    { AKEYCODE_ESCAPE, KeyboardKeys::Escape, GamepadButton::None },
    { AKEYCODE_CTRL_LEFT, KeyboardKeys::Control, GamepadButton::None },
    { AKEYCODE_CTRL_RIGHT, KeyboardKeys::Control, GamepadButton::None },
    { AKEYCODE_SCROLL_LOCK, KeyboardKeys::Scroll, GamepadButton::None },
    { AKEYCODE_BREAK, KeyboardKeys::Pause, GamepadButton::None },
    { AKEYCODE_MOVE_HOME, KeyboardKeys::Home, GamepadButton::None },
    { AKEYCODE_MOVE_END, KeyboardKeys::End, GamepadButton::None },
    { AKEYCODE_INSERT, KeyboardKeys::Insert, GamepadButton::None },
    { AKEYCODE_MEDIA_EJECT, KeyboardKeys::LaunchMediaSelect, GamepadButton::None },
    { AKEYCODE_F1, KeyboardKeys::F1, GamepadButton::None },
    { AKEYCODE_F2, KeyboardKeys::F2, GamepadButton::None },
    { AKEYCODE_F3, KeyboardKeys::F3, GamepadButton::None },
    { AKEYCODE_F4, KeyboardKeys::F4, GamepadButton::None },
    { AKEYCODE_F5, KeyboardKeys::F5, GamepadButton::None },
    { AKEYCODE_F6, KeyboardKeys::F6, GamepadButton::None },
    { AKEYCODE_F7, KeyboardKeys::F7, GamepadButton::None },
    { AKEYCODE_F8, KeyboardKeys::F8, GamepadButton::None },
    { AKEYCODE_F9, KeyboardKeys::F9, GamepadButton::None },
    { AKEYCODE_F10, KeyboardKeys::F10, GamepadButton::None },
    { AKEYCODE_F11, KeyboardKeys::F11, GamepadButton::None },
    { AKEYCODE_F12, KeyboardKeys::F12, GamepadButton::None },
    { AKEYCODE_NUM_LOCK, KeyboardKeys::Numlock, GamepadButton::None },
    { AKEYCODE_NUMPAD_0, KeyboardKeys::Numpad0, GamepadButton::None },
    { AKEYCODE_NUMPAD_1, KeyboardKeys::Numpad1, GamepadButton::None },
    { AKEYCODE_NUMPAD_2, KeyboardKeys::Numpad2, GamepadButton::None },
    { AKEYCODE_NUMPAD_3, KeyboardKeys::Numpad3, GamepadButton::None },
    { AKEYCODE_NUMPAD_4, KeyboardKeys::Numpad4, GamepadButton::None },
    { AKEYCODE_NUMPAD_5, KeyboardKeys::Numpad5, GamepadButton::None },
    { AKEYCODE_NUMPAD_6, KeyboardKeys::Numpad6, GamepadButton::None },
    { AKEYCODE_NUMPAD_7, KeyboardKeys::Numpad7, GamepadButton::None },
    { AKEYCODE_NUMPAD_8, KeyboardKeys::Numpad8, GamepadButton::None },
    { AKEYCODE_NUMPAD_9, KeyboardKeys::Numpad9, GamepadButton::None },
    { AKEYCODE_NUMPAD_DIVIDE, KeyboardKeys::NumpadDivide, GamepadButton::None },
    { AKEYCODE_NUMPAD_MULTIPLY, KeyboardKeys::NumpadMultiply, GamepadButton::None },
    { AKEYCODE_NUMPAD_SUBTRACT, KeyboardKeys::NumpadSubtract, GamepadButton::None },
    { AKEYCODE_NUMPAD_ADD, KeyboardKeys::NumpadAdd, GamepadButton::None },
    { AKEYCODE_NUMPAD_DOT, KeyboardKeys::NumpadSeparator, GamepadButton::None },
    { AKEYCODE_NUMPAD_COMMA, KeyboardKeys::NumpadDecimal, GamepadButton::None },
    { AKEYCODE_NUMPAD_ENTER, KeyboardKeys::Return, GamepadButton::None },
    { AKEYCODE_VOLUME_MUTE, KeyboardKeys::VolumeMute, GamepadButton::None },
    { AKEYCODE_HELP, KeyboardKeys::Help, GamepadButton::None },
    { AKEYCODE_KANA, KeyboardKeys::Kana, GamepadButton::None },
};

class AndroidKeyboard : public Keyboard
{
public:

    explicit AndroidKeyboard()
        : Keyboard()
    {
    }
};

class AndroidDeviceGamepad : public Gamepad
{
public:

    explicit AndroidDeviceGamepad()
        : Gamepad(Guid(0, 0, 0, 1), TEXT("Android"))
    {
        CachedState.Clear();
    }

    ~AndroidDeviceGamepad()
    {
    }

    State CachedState;

public:

    void SetVibration(const GamepadVibrationState& state) override;

    bool UpdateState() override
    {
        Platform::MemoryCopy(&_state, &CachedState, sizeof(_state));
        return false;
    }
};

class AndroidTouchScreen : public InputDevice
{
public:

    explicit AndroidTouchScreen()
        : InputDevice(SpawnParams(Guid::New(), TypeInitializer), TEXT("Android Touch Screen"))
    {
    }

    void OnTouch(EventType type, float x, float y, int32 pointerId)
    {
        Event& e = _queue.AddOne();
        e.Type = type;
        e.Target = nullptr;
        e.TouchData.Position.X = x;
        e.TouchData.Position.Y = y;
        e.TouchData.PointerId = pointerId;
    }
};

namespace
{
    android_app* App = nullptr;
    ANativeWindow* AppWindow = nullptr;
    CPUInfo AndroidCpu;
    int ClockSource;
    bool HasFocus = false;
    bool IsStarted = false;
    bool IsPaused = true;
    bool IsVibrating = false;
    int32 ScreenWidth = 0, ScreenHeight = 0;
    uint64 ProgramSizeMemory = 0;
    Guid DeviceId;
    String AppPackageName, DeviceManufacturer, DeviceModel, DeviceBuildNumber;
    String SystemVersion, SystemLanguage, CacheDir, ExecutablePath;
    byte MacAddress[6];
    AndroidKeyboard* KeyboardImpl;
    AndroidDeviceGamepad* GamepadImpl;
    AndroidTouchScreen* TouchScreenImpl;
    ScreenOrientationType Orientation;

    void UnixGetMacAddress(byte result[6])
    {
        struct ifreq ifr;
        int fd = socket(AF_INET, SOCK_DGRAM, 0);
        ifr.ifr_addr.sa_family = AF_INET;
        strncpy((char*)ifr.ifr_name, "eth0", IFNAMSIZ - 1);
        ioctl(fd, SIOCGIFHWADDR, &ifr);
        close(fd);
        Platform::MemoryCopy(result, ifr.ifr_hwaddr.sa_data, 6);
    }

    ScreenOrientationType getOrientation()
    {
        JNIEnv* jni;
        App->activity->vm->AttachCurrentThread(&jni, nullptr);
        ASSERT(jni);
        const jclass clazz = jni->GetObjectClass(App->activity->clazz);
        ASSERT(clazz);
        const jmethodID methodID = jni->GetMethodID(clazz, "getRotation", "()I");
        ASSERT(methodID);
        const jint rotation = jni->CallIntMethod(App->activity->clazz, methodID);
        App->activity->vm->DetachCurrentThread();
        ScreenOrientationType orientation;
        switch (rotation)
        {
        case 0:
            orientation = ScreenOrientationType::Portrait;
            break;
        case 1:
            orientation = ScreenOrientationType::LandscapeLeft;
            break;
        case 2:
            orientation = ScreenOrientationType::PortraitUpsideDown;
            break;
        case 3:
            orientation = ScreenOrientationType::LandscapeRight;
            break;
        default:
            orientation = ScreenOrientationType::Unknown;
        }
        return orientation;
    }

    Float2 GetWindowSize()
    {
        const float width = (float)ANativeWindow_getWidth(AppWindow);
        const float height = (float)ANativeWindow_getHeight(AppWindow);
        return Float2(width, height);
    }

    void UpdateOrientation()
    {
        Orientation = getOrientation();
        if (AppWindow && Engine::MainWindow)
        {
            Engine::MainWindow->SetClientSize(GetWindowSize());
        }
    }

    void OnAppCmd(android_app* app, int32_t cmd)
    {
        switch (cmd)
        {
        case APP_CMD_START:
            LOG(Info, "[Android] APP_CMD_START");
            IsStarted = true;
            UpdateOrientation();
            break;
        case APP_CMD_RESUME:
            LOG(Info, "[Android] APP_CMD_RESUME");
            IsPaused = false;
            UpdateOrientation();
            break;
        case APP_CMD_PAUSE:
            LOG(Info, "[Android] APP_CMD_PAUSE");
            IsPaused = true;
            break;
        case APP_CMD_STOP:
            LOG(Info, "[Android] APP_CMD_STOP");
            IsStarted = false;
            //Engine::OnExit();
            break;
        case APP_CMD_DESTROY:
            LOG(Info, "[Android] APP_CMD_DESTROY");
            break;
        case APP_CMD_INIT_WINDOW:
            LOG(Info, "[Android] APP_CMD_INIT_WINDOW");
            AppWindow = app->window;
            ANativeWindow_acquire(AppWindow);
            UpdateOrientation();
            if (Engine::MainWindow)
            {
                Engine::MainWindow->InitSwapChain();
            }
            break;
        case APP_CMD_WINDOW_RESIZED:
            LOG(Info, "[Android] APP_CMD_WINDOW_RESIZED");
            if (Engine::MainWindow)
            {
                Engine::MainWindow->SetClientSize(GetWindowSize());
            }
            break;
        case APP_CMD_TERM_WINDOW:
            LOG(Info, "[Android] APP_CMD_TERM_WINDOW");
            if (Engine::MainWindow && Engine::MainWindow->GetSwapChain())
            {
                Engine::MainWindow->GetSwapChain()->ReleaseGPU();
            }
            ANativeWindow_release(AppWindow);
            AppWindow = nullptr;
            break;
        case APP_CMD_CONFIG_CHANGED:
        {
            LOG(Info, "[Android] APP_CMD_CONFIG_CHANGED");
            UpdateOrientation();
            break;
        }
        case APP_CMD_GAINED_FOCUS:
            LOG(Info, "[Android] APP_CMD_GAINED_FOCUS");
            HasFocus = true;
            if (Engine::MainWindow)
                Engine::MainWindow->OnGotFocus();
            break;
        case APP_CMD_LOST_FOCUS:
            LOG(Info, "[Android] APP_CMD_LOST_FOCUS");
            HasFocus = false;
            if (Engine::MainWindow)
                Engine::MainWindow->OnLostFocus();
            break;
#if !BUILD_RELEASE
        default:
            __android_log_print(ANDROID_LOG_INFO, "Flax", "App Cmd not handled: %d", cmd);
#endif
        }
    }

    int32_t OnAppInput(android_app* app, AInputEvent* inputEvent)
    {
        switch (AInputEvent_getType(inputEvent))
        {
        case AINPUT_EVENT_TYPE_MOTION:
        {
            const int32_t action = AMotionEvent_getAction(inputEvent);
            switch (action & AMOTION_EVENT_ACTION_MASK)
            {
            case AMOTION_EVENT_ACTION_DOWN:
            {
                const int32 pointerCount = (int32)AMotionEvent_getPointerCount(inputEvent);
                for (int32 i = 0; i < pointerCount; i++)
                {
                    const int32 pointerId = AMotionEvent_getPointerId(inputEvent, i);
                    const float x = AMotionEvent_getX(inputEvent, i);
                    const float y = AMotionEvent_getY(inputEvent, i);
                    TouchScreenImpl->OnTouch(InputDevice::EventType::TouchDown, x, y, pointerId);
                }
                break;
            }
            case AMOTION_EVENT_ACTION_UP:
            {
                const int32 pointerCount = (int32)AMotionEvent_getPointerCount(inputEvent);
                for (int32 i = 0; i < pointerCount; i++)
                {
                    const int32 pointerId = AMotionEvent_getPointerId(inputEvent, i);
                    const float x = AMotionEvent_getX(inputEvent, i);
                    const float y = AMotionEvent_getY(inputEvent, i);
                    TouchScreenImpl->OnTouch(InputDevice::EventType::TouchUp, x, y, pointerId);
                }
                break;
            }
            case AMOTION_EVENT_ACTION_MOVE:
            {
                const int32 pointerCount = (int32)AMotionEvent_getPointerCount(inputEvent);
                for (int32 i = 0; i < pointerCount; i++)
                {
                    const int32 pointerId = AMotionEvent_getPointerId(inputEvent, i);
                    const float x = AMotionEvent_getX(inputEvent, i);
                    const float y = AMotionEvent_getY(inputEvent, i);
                    TouchScreenImpl->OnTouch(InputDevice::EventType::TouchMove, x, y, pointerId);
                }
                break;
            }
            case AMOTION_EVENT_ACTION_POINTER_DOWN:
            {
                const int32 pointerIndex = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
                const int32 pointerId = AMotionEvent_getPointerId(inputEvent, pointerIndex);
                const float x = AMotionEvent_getX(inputEvent, pointerIndex);
                const float y = AMotionEvent_getY(inputEvent, pointerIndex);
                TouchScreenImpl->OnTouch(InputDevice::EventType::TouchDown, x, y, pointerId);
                break;
            }
            case AMOTION_EVENT_ACTION_POINTER_UP:
            {
                const int32 pointerIndex = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
                const int32 pointerId = AMotionEvent_getPointerId(inputEvent, pointerIndex);
                const float x = AMotionEvent_getX(inputEvent, pointerIndex);
                const float y = AMotionEvent_getY(inputEvent, pointerIndex);
                TouchScreenImpl->OnTouch(InputDevice::EventType::TouchUp, x, y, pointerId);
                break;
            }
            }
            break;
        }
        case AINPUT_EVENT_TYPE_KEY:
        {
            const int32_t keyCode = AKeyEvent_getKeyCode(inputEvent);
            for (int32 i = 0; i < ARRAY_COUNT(AndroidKeyEventTypes); i++)
            {
                auto& eventType = AndroidKeyEventTypes[i];
                if (eventType.KeyCode == keyCode)
                {
                    bool isDown;
                    const int32_t action = AKeyEvent_getAction(inputEvent);
                    switch (action)
                    {
                    case AKEY_EVENT_ACTION_DOWN:
                        isDown = true;
                        break;
                    case AKEY_EVENT_ACTION_MULTIPLE:
                        isDown = (AKeyEvent_getRepeatCount(inputEvent) % 2) == 0;
                        break;
                    case AKEY_EVENT_ACTION_UP:
                    default:
                        isDown = false;
                        break;
                    }
                    LOG(Warning, "Input Event: KeyCode={}, KeyboardKey={}, GamepadButton={}, IsDown={}", eventType.KeyCode, (int32)eventType.KeyboardKey, (int32)eventType.GamepadButton, isDown);

                    // Keyboard
                    if (eventType.KeyboardKey != KeyboardKeys::None)
                    {
                        if (isDown)
                            KeyboardImpl->OnKeyDown(eventType.KeyboardKey);
                        else
                            KeyboardImpl->OnKeyUp(eventType.KeyboardKey);
                    }

                    // Gamepad
                    if (eventType.GamepadButton != GamepadButton::None)
                    {
                        GamepadImpl->CachedState.Buttons[(int32)eventType.GamepadButton] = isDown;
                    }

                    return 1;
                }
            }
            break;
        }
        }
        return 0;
    }

    String ToString(JNIEnv* env, jstring str)
    {
        const char* chars = env->GetStringUTFChars(str, nullptr);
        String result(chars);
        env->ReleaseStringUTFChars(str, chars);
        return result;
    }
}

extern "C" {
JNIEXPORT
void JNICALL Java_com_flaxengine_GameActivity_nativeSetPlatformInfo(JNIEnv* env, jobject thiz, jstring appPackageName, jstring deviceManufacturer, jstring deviceModel, jstring deviceBuildNumber, jstring systemVersion, jstring systemLanguage, int32 screenWidth, int32 screenHeight, jstring cacheDir, jstring executablePath)
{
    AppPackageName = ToString(env, appPackageName);
    DeviceManufacturer = ToString(env, deviceManufacturer);
    DeviceModel = ToString(env, deviceModel);
    DeviceBuildNumber = ToString(env, deviceBuildNumber);
    SystemVersion = ToString(env, systemVersion);
    SystemLanguage = ToString(env, systemLanguage);
    SystemLanguage.Replace('_', '-');
    ScreenWidth = screenWidth;
    ScreenHeight = screenHeight;
    CacheDir = ToString(env, cacheDir);
    ExecutablePath = ToString(env, executablePath);
}
}

DialogResult MessageBox::Show(Window* parent, const StringView& text, const StringView& caption, MessageBoxButtons buttons, MessageBoxIcon icon)
{
    JNIEnv* jni;
    App->activity->vm->AttachCurrentThread(&jni, nullptr);
    ASSERT(jni);
    const jclass clazz = jni->GetObjectClass(App->activity->clazz);
    ASSERT(clazz);
    const jmethodID methodID = jni->GetMethodID(clazz, "showAlert", "(Ljava/lang/String;Ljava/lang/String;)V");
    ASSERT(methodID);
    const StringAsANSI<> textAnsi(*text, text.Length());
    const jstring jtext = jni->NewStringUTF(textAnsi.Get());
    const StringAsANSI<> captionAnsi(*caption, caption.Length());
    const jstring jcaption = jni->NewStringUTF(captionAnsi.Get());

    jni->CallVoidMethod(App->activity->clazz, methodID, jtext, jcaption);

    jni->DeleteLocalRef(jtext);
    jni->DeleteLocalRef(jcaption);
    App->activity->vm->DetachCurrentThread();

    // TODO: implement message box buttons and handle it better in Java

    return DialogResult::OK;
}

void AndroidFileSystem::GetSpecialFolderPath(const SpecialFolder type, String& result)
{
    switch (type)
    {
    case SpecialFolder::Desktop:
        result = TEXT("/storage/self/primary");
        break;
    case SpecialFolder::Documents:
        result = TEXT("/storage/self/primary/Documents");
        break;
    case SpecialFolder::Pictures:
        result = TEXT("/storage/self/primary/DCIM");
        break;
    case SpecialFolder::AppData:
        result = TEXT("/usr/share");
        break;
    case SpecialFolder::LocalAppData:
    case SpecialFolder::ProgramData:
        result = String(AndroidPlatform::GetApp()->activity->externalDataPath);
        break;
    case SpecialFolder::Temporary:
        result = CacheDir;
        break;
    default:
    CRASH;
        break;
    }
}

void AndroidDeviceGamepad::SetVibration(const GamepadVibrationState& state)
{
    Gamepad::SetVibration(state);

    JNIEnv* jni;
    App->activity->vm->AttachCurrentThread(&jni, nullptr);
    ASSERT(jni);
    const jclass clazz = jni->GetObjectClass(App->activity->clazz);
    ASSERT(clazz);
    const jmethodID methodID = jni->GetMethodID(clazz, "vibrate", "(I)V");
    ASSERT(methodID);
    const float max = Math::Max(Math::Max(state.LeftLarge, state.LeftSmall), Math::Max(state.RightLarge, state.RightSmall));
    if (IsVibrating)
    {
        if (max < 0.25f)
        {
            jni->CallVoidMethod(App->activity->clazz, methodID, 0);
            IsVibrating = false;
        }
    }
    else
    {
        if (max >= 0.25f)
        {
            jni->CallVoidMethod(App->activity->clazz, methodID, 30000);
            IsVibrating = true;
        }
    }
    App->activity->vm->DetachCurrentThread();
}

android_app* AndroidPlatform::GetApp()
{
    return App;
}

String AndroidPlatform::GetAppPackageName()
{
    return AppPackageName;
}

String AndroidPlatform::GetDeviceManufacturer()
{
    return DeviceManufacturer;
}

String AndroidPlatform::GetDeviceModel()
{
    return DeviceModel;
}

String AndroidPlatform::GetDeviceBuildNumber()
{
    return DeviceBuildNumber;
}

void AndroidPlatform::PreInit(android_app* app)
{
    App = app;
    app->onAppCmd = OnAppCmd;
    app->onInputEvent = OnAppInput;
    ANativeActivity_setWindowFlags(app->activity, AWINDOW_FLAG_KEEP_SCREEN_ON | AWINDOW_FLAG_TURN_SCREEN_ON | AWINDOW_FLAG_FULLSCREEN | AWINDOW_FLAG_DISMISS_KEYGUARD, 0);
    ANativeActivity_setWindowFormat(app->activity, WINDOW_FORMAT_RGBA_8888);
    pthread_setname_np(pthread_self(), "Main");
}

bool AndroidPlatform::Is64BitPlatform()
{
#ifdef PLATFORM_64BITS
    return true;
#else
#error "Implement AndroidPlatform::Is64BitPlatform for 32-bit builds."
#endif
}

CPUInfo AndroidPlatform::GetCPUInfo()
{
    return AndroidCpu;
}

MemoryStats AndroidPlatform::GetMemoryStats()
{
    const uint64 pageSize = getpagesize();
    const uint64 totalPages = get_phys_pages();
    const uint64 availablePages = get_avphys_pages();
    MemoryStats result;
    result.TotalPhysicalMemory = totalPages * pageSize;
    result.UsedPhysicalMemory = (totalPages - availablePages) * pageSize;
    result.TotalVirtualMemory = result.TotalPhysicalMemory;
    result.UsedVirtualMemory = result.UsedPhysicalMemory;
    result.ProgramSizeMemory = ProgramSizeMemory;
    return result;
}

ProcessMemoryStats AndroidPlatform::GetProcessMemoryStats()
{
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    ProcessMemoryStats result;
    result.UsedPhysicalMemory = usage.ru_maxrss;
    result.UsedVirtualMemory = result.UsedPhysicalMemory;
    return result;
}

void AndroidPlatform::SetThreadPriority(ThreadPriority priority)
{
    // TODO: impl this
}

void AndroidPlatform::SetThreadAffinityMask(uint64 affinityMask)
{
    pid_t tid = gettid();
    int mask = (int)affinityMask;
    syscall(__NR_sched_setaffinity, tid, sizeof(mask), &mask);
}

void AndroidPlatform::Sleep(int32 milliseconds)
{
    usleep(milliseconds * 1000);
}

double AndroidPlatform::GetTimeSeconds()
{
    struct timespec ts;
    clock_gettime(ClockSource, &ts);
    return static_cast<double>(ts.tv_sec) + static_cast<double>(ts.tv_nsec) / 1e9;
}

uint64 AndroidPlatform::GetTimeCycles()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return static_cast<uint64>(static_cast<uint64>(ts.tv_sec) * 1000000ULL + static_cast<uint64>(ts.tv_nsec) / 1000ULL);
}

void AndroidPlatform::GetSystemTime(int32& year, int32& month, int32& dayOfWeek, int32& day, int32& hour, int32& minute, int32& second, int32& millisecond)
{
    // Get the calendar time
    struct timeval time;
    gettimeofday(&time, nullptr);

    // Convert calendar time to local time
    struct tm localTime;
    localtime_r(&time.tv_sec, &localTime);

    // Extract time from Unix date
    year = localTime.tm_year + 1900;
    month = localTime.tm_mon + 1;
    dayOfWeek = localTime.tm_wday;
    day = localTime.tm_mday;
    hour = localTime.tm_hour;
    minute = localTime.tm_min;
    second = localTime.tm_sec;
    millisecond = time.tv_usec / 1000;
}

void AndroidPlatform::GetUTCTime(int32& year, int32& month, int32& dayOfWeek, int32& day, int32& hour, int32& minute, int32& second, int32& millisecond)
{
    // Get the calendar time
    struct timeval time;
    gettimeofday(&time, nullptr);

    // Convert to UTC time
    struct tm localTime;
    gmtime_r(&time.tv_sec, &localTime);

    // Extract time
    year = localTime.tm_year + 1900;
    month = localTime.tm_mon + 1;
    dayOfWeek = localTime.tm_wday;
    day = localTime.tm_mday;
    hour = localTime.tm_hour;
    minute = localTime.tm_min;
    second = localTime.tm_sec;
    millisecond = time.tv_usec / 1000;
}

bool AndroidPlatform::Init()
{
    if (UnixPlatform::Init())
        return true;

    // Init timing
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1)
    {
        ClockSource = CLOCK_REALTIME;
    }
    else
    {
        ClockSource = CLOCK_MONOTONIC;
    }

	// Estimate program size by checking physical memory usage on start
	ProgramSizeMemory = Platform::GetProcessMemoryStats().UsedPhysicalMemory;

    // Set info about the CPU
    cpu_set_t cpus;
    CPU_ZERO(&cpus);
    if (sched_getaffinity(0, sizeof(cpus), &cpus) == 0)
    {
        AndroidCpu.ProcessorCoreCount = AndroidCpu.LogicalProcessorCount = CPU_COUNT(&cpus);
    }
    else
    {
        AndroidCpu.ProcessorCoreCount = AndroidCpu.LogicalProcessorCount = 1;
    }
    AndroidCpu.ProcessorPackageCount = 1;
    AndroidCpu.L1CacheSize = 0;
    AndroidCpu.L2CacheSize = 0;
    AndroidCpu.L3CacheSize = 0;
    AndroidCpu.PageSize = sysconf(_SC_PAGESIZE);
    AndroidCpu.ClockSpeed = GetClockFrequency();
    AndroidCpu.CacheLineSize = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
    if (!AndroidCpu.CacheLineSize)
    {
        AndroidCpu.CacheLineSize = PLATFORM_CACHE_LINE_SIZE;
    }

    UnixGetMacAddress(MacAddress);

    // Generate unique device ID
    {
        DeviceId = Guid::Empty;

        // A - Computer Name and User Name
        uint32 hash = GetHash(Platform::GetComputerName());
        CombineHash(hash, GetHash(Platform::GetUserName()));
        DeviceId.A = hash;

        // B - MAC address
        hash = MacAddress[0];
        for (uint32 i = 0; i < 6; i++)
            CombineHash(hash, MacAddress[i]);
        DeviceId.B = hash;

        // C - memory
        DeviceId.C = (uint32)Platform::GetMemoryStats().TotalPhysicalMemory;

        // D - cpuid
        DeviceId.D = (uint32)AndroidCpu.ClockSpeed * AndroidCpu.LogicalProcessorCount * AndroidCpu.ProcessorCoreCount * AndroidCpu.CacheLineSize;
    }

    // Setup native platform input devices
    Input::Keyboard = KeyboardImpl = New<AndroidKeyboard>();
    Input::Gamepads.Add(GamepadImpl = New<AndroidDeviceGamepad>());
    Input::OnGamepadsChanged();
    Input::CustomDevices.Add(TouchScreenImpl = New<AndroidTouchScreen>());

    // Perform initial app messages pump
    Tick();

    return false;
}

void AndroidPlatform::LogInfo()
{
    UnixPlatform::LogInfo();

    LOG(Info, "App Package: {0}", AppPackageName);
    LOG(Info, "Android {0}", SystemVersion);
    LOG(Info, "Device: {0} {1}, {2}", DeviceManufacturer, DeviceModel, DeviceBuildNumber);
}

void AndroidPlatform::BeforeRun()
{
    // Perform initial app messages pump
    Tick();
}

void AndroidPlatform::Tick()
{
    UnixPlatform::Tick();

    // Pool app events
    int events;
    android_poll_source* source;
    while (ALooper_pollAll(0, nullptr, &events, reinterpret_cast<void**>(&source)) >= 0)
    {
        // Process event
        if (source != nullptr)
        {
            source->process(App, source);
        }

        // Check if exit
        if (App->destroyRequested != 0)
        {
            Engine::RequestExit();
            return;
        }
    }

    UpdateOrientation();
}

void AndroidPlatform::BeforeExit()
{
}

void AndroidPlatform::Exit()
{
}

#if !BUILD_RELEASE

void AndroidPlatform::Log(const StringView& msg)
{
    const StringAsANSI<512> msgAnsi(*msg, msg.Length());
    const char* str = msgAnsi.Get();
    __android_log_write(ANDROID_LOG_INFO, "Flax", str);
}

#endif

String AndroidPlatform::GetSystemName()
{
    return String::Format(TEXT("Android {}"), SystemVersion);
}

Version AndroidPlatform::GetSystemVersion()
{
    Version version(0, 0);
    Version::Parse(SystemVersion, &version);
    return version;
}

int32 AndroidPlatform::GetDpi()
{
    return AConfiguration_getScreenWidthDp(App->config);
}

NetworkConnectionType AndroidPlatform::GetNetworkConnectionType()
{
    JNIEnv* jni;
    App->activity->vm->AttachCurrentThread(&jni, nullptr);
    ASSERT(jni);
    const jclass clazz = jni->GetObjectClass(App->activity->clazz);
    ASSERT(clazz);
    const jmethodID methodID = jni->GetMethodID(clazz, "getNetworkConnectionType", "()I");
    ASSERT(methodID);
    const jint type = jni->CallIntMethod(App->activity->clazz, methodID);
    App->activity->vm->DetachCurrentThread();
    return (NetworkConnectionType)type;
}

ScreenOrientationType AndroidPlatform::GetScreenOrientationType()
{
    return Orientation;
}

String AndroidPlatform::GetUserLocaleName()
{
    return SystemLanguage;
}

String AndroidPlatform::GetComputerName()
{
    return DeviceModel;
}

bool AndroidPlatform::GetHasFocus()
{
    return HasFocus;
}

bool AndroidPlatform::GetIsPaused()
{
    return IsPaused || App->window == nullptr;
}

bool AndroidPlatform::CanOpenUrl(const StringView& url)
{
    return true;
}

void AndroidPlatform::OpenUrl(const StringView& url)
{
    JNIEnv* jni;
    App->activity->vm->AttachCurrentThread(&jni, nullptr);
    ASSERT(jni);
    const jclass clazz = jni->GetObjectClass(App->activity->clazz);
    ASSERT(clazz);
    const jmethodID methodID = jni->GetMethodID(clazz, "openUrl", "(Ljava/lang/String;)V");
    ASSERT(methodID);
    const StringAsANSI<> urlAnsi(*url, url.Length());
    const jstring jurl = jni->NewStringUTF(urlAnsi.Get());
    jni->CallVoidMethod(App->activity->clazz, methodID, jurl);
    jni->DeleteLocalRef(jurl);
    App->activity->vm->DetachCurrentThread();
}

Float2 AndroidPlatform::GetMousePosition()
{
    return Float2::Zero;
}

void AndroidPlatform::SetMousePosition(const Float2& pos)
{
}

Float2 AndroidPlatform::GetDesktopSize()
{
    return Float2((float)ScreenWidth, (float)ScreenHeight);
}

String AndroidPlatform::GetMainDirectory()
{
    return String(App->activity->internalDataPath);
}

String AndroidPlatform::GetExecutableFilePath()
{
    return ExecutablePath;
}

Guid AndroidPlatform::GetUniqueDeviceId()
{
    return DeviceId;
}

String AndroidPlatform::GetWorkingDirectory()
{
    char buffer[256];
    getcwd(buffer, ARRAY_COUNT(buffer));
    return String(buffer);
}

bool AndroidPlatform::SetWorkingDirectory(const String& path)
{
    return chdir(StringAsANSI<>(*path, path.Length()).Get()) != 0;
}

Window* AndroidPlatform::CreateWindow(const CreateWindowSettings& settings)
{
    return New<AndroidWindow>(settings);
}

bool AndroidPlatform::GetEnvironmentVariable(const String& name, String& value)
{
    char* env = getenv(StringAsANSI<>(*name, name.Length()).Get());
    if (env)
    {
        value = String(env);
        return false;
    }
    return true;
}

bool AndroidPlatform::SetEnvironmentVariable(const String& name, const String& value)
{
    return setenv(StringAsANSI<>(*name, name.Length()).Get(), StringAsANSI<>(*value, value.Length()).Get(), true) != 0;
}

void* AndroidPlatform::LoadLibrary(const Char* filename)
{
    PROFILE_CPU();
    ZoneText(filename, StringUtils::Length(filename));
    const StringAsANSI<> filenameANSI(filename);
    void* result = dlopen(filenameANSI.Get(), RTLD_LAZY);
    if (!result)
    {
        LOG(Error, "Failed to load {0} because {1}", filename, String(dlerror()));
    }
    return result;
}

void AndroidPlatform::FreeLibrary(void* handle)
{
    dlclose(handle);
}

void* AndroidPlatform::GetProcAddress(void* handle, const char* symbol)
{
    return dlsym(handle, symbol);
}

Array<AndroidPlatform::StackFrame> AndroidPlatform::GetStackFrames(int32 skipCount, int32 maxDepth, void* context)
{
    Array<StackFrame> result;
#if CRASH_LOG_ENABLE
    // Reference: https://stackoverflow.com/questions/8115192/android-ndk-getting-the-backtrace
    void* callstack[120];
    skipCount = Math::Min<int32>(skipCount, ARRAY_COUNT(callstack));
    int32 maxCount = Math::Min<int32>(ARRAY_COUNT(callstack), skipCount + maxDepth);
    AndroidBacktraceState state;
    state.Current = callstack;
    state.End = callstack + maxCount;
    _Unwind_Backtrace(AndroidUnwindCallback, &state);
    int32 count = (int32)(state.Current - callstack);
    int32 useCount = count - skipCount;
    if (useCount > 0)
    {
        result.Resize(useCount);
        for (int32 i = 0; i < useCount; i++)
        {
            StackFrame& frame = result[i];
            frame.ProgramCounter = callstack[skipCount + i];
            frame.ModuleName[0] = 0;
            frame.FileName[0] = 0;
            frame.LineNumber = 0;
            Dl_info info;
            const char* symbol = "";
            if (dladdr(frame.ProgramCounter, &info) && info.dli_sname) 
                symbol = info.dli_sname;
            int status = 0; 
            char* demangled = __cxxabiv1::__cxa_demangle(symbol, 0, 0, &status); 
            int32 nameLen = Math::Min<int32>(StringUtils::Length(demangled), ARRAY_COUNT(frame.FunctionName) - 1);
            Platform::MemoryCopy(frame.FunctionName, demangled, nameLen);
            frame.FunctionName[nameLen] = 0;
            if (demangled)
                free(demangled);
        }
    }
#endif
    return result;
}

#endif
