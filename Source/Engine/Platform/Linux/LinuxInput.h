#if PLATFORM_LINUX

#include <string>
#include "/usr/include/linux/input-event-codes.h"
#include "/usr/include/linux/input.h"
#include "Engine/Input/Input.h"
#include "Engine/Input/Gamepad.h"

using namespace std;

#define LINUXINPUT_MAX_GAMEPADS 8
#define LINUXINPUT_MAX_GAMEPAD_EVENTS_PER_FRAME 10
#define TRIGGER_THRESHOLD 1000

class LinuxGamepad : public Gamepad
{
    struct State {
        bool Buttons[32];
        float Axis[32];
    };
    public:
    LinuxGamepad(uint32_t uid[], string name);
    ~LinuxGamepad();
    int fd;
    string dev;
    State _state;
    bool UpdateState();
};

class LinuxInput
{
    struct InputDevice {
        uint32_t uid[4];
        string name;
        string handler;
        bool isGamepad;
    };
    const char* InputDevicesFile = "/proc/bus/input/devices";
    InputDevice inputDevices[LINUXINPUT_MAX_GAMEPADS];
    LinuxGamepad *linuxGamepads[LINUXINPUT_MAX_GAMEPADS];
    int foundGamepads;
    public:
    void UpdateState();
    void DetectGamePads();
    void DumpDevices();
    static void Update();
    static LinuxInput *singleton;
};

#endif