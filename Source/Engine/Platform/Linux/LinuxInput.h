#if PLATFORM_LINUX

#include <string>
#include "/usr/include/linux/input-event-codes.h"
#include "/usr/include/linux/input.h"
#include "Engine/Input/Input.h"
#include "Engine/Input/Gamepad.h"

using namespace std;

#define LINUXINPUT_MAX_GAMEPADS 8
#define LINUXINPUT_MAX_GAMEPAD_EVENTS_PER_FRAME 32
#define TRIGGER_THRESHOLD 1000

class LinuxGamepad : public Gamepad
{
    struct State {
        bool Buttons[32];
        float Axis[32];
    };
    public:
    LinuxGamepad(u_int32_t uid[], string name);
    ~LinuxGamepad();
    int fd;
    string dev;
    State _state;
    bool UpdateState();
};

class LinuxInput
{
    public:
    static void UpdateState();
    static void DetectGamePads();
    static void DumpDevices();
    static void Init();
};

#endif