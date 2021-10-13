#if PLATFORM_LINUX

#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <fcntl.h>
#include "LinuxInput.h"
#include "Engine/Core/Log.h"

using namespace std;

struct LinuxInputDevice {
    u_int32_t uid[4];
    string name;
    string handler;
    bool isGamepad;
};

static int foundGamepads;
static float lastUpdateTime;
static LinuxInputDevice inputDevices[LINUXINPUT_MAX_GAMEPADS];
static LinuxGamepad *linuxGamepads[LINUXINPUT_MAX_GAMEPADS];

void LinuxInput::Init() {
    for (int i = 0; i < LINUXINPUT_MAX_GAMEPADS; i++)
    {
        linuxGamepads[i] = nullptr;
    }
    foundGamepads = 0;
    // this will delay gamepad detection
    lastUpdateTime = Platform::GetTimeSeconds();
}

void LinuxInput::DetectGamePads()
{
    string line;
    std::ifstream devs("/proc/bus/input/devices");
    
    foundGamepads = 0;
    if (devs.is_open())
    {
        while (getline(devs, line))
        {
            if (line[0] == 'N')
            {
                int quoteIndex = line.find('"');
                inputDevices[foundGamepads].name = line.substr(quoteIndex+1, line.length() - quoteIndex - 2);
            } else if (line[0] == 'I')
            {
                int startIndex = 0;
                for (int i = 0; i < 4; i++) {
                    int equalsIndex = line.find('=', startIndex);
                    if (equalsIndex > 0) {
                        istringstream part(line.substr(equalsIndex+1, 4));
                        part >> std::hex >> inputDevices[foundGamepads].uid[i];
                    }
                    startIndex = equalsIndex+1;
                }
            } else if (line[0] == 'H')
            {
                int eventIndex = line.find("event");
                if (eventIndex > 0) {
                    int end = line.find(' ', eventIndex);
                    if (end > 0) {
                        inputDevices[foundGamepads].handler = "/dev/input/" + line.substr(eventIndex, end - eventIndex);
                    }
                }
            } else if (line[0] == 'B')
            {
                int keyIndex = line.find("KEY=");
                if (keyIndex >= 0) {
                    // there should be five groups of 64 bits each
                    std::istringstream group(line.substr(keyIndex+4));
                    unsigned long int n64, msb;
                    int foundGroups = 0;
                    for (int i = 0; i < 5; i++)
                    {
                        group >> std::hex >> n64;
                        if (i == 0) msb = n64;
                        foundGroups++;
                        if (group.eof()) break;
                    }
                    if (foundGroups < 5) msb = 0;
                    inputDevices[foundGamepads].isGamepad = (msb & 1lu<<48) > 0;
                    
                }
            } else if (line.length() == 0)
            {
                if (inputDevices[foundGamepads].isGamepad && foundGamepads < (LINUXINPUT_MAX_GAMEPADS-1))
                    {
                        foundGamepads++;
                    }
            }
        }
        devs.close();
    }
    //DumpDevices();
};

void LinuxInput::UpdateState()
{
    const float time = (float)Platform::GetTimeSeconds();
    if (time - lastUpdateTime > 5.0f)
    {
        DetectGamePads();
        lastUpdateTime = time;
        for (int i = 0; i < foundGamepads; i++)
        {
            if (linuxGamepads[i] == nullptr)
            {
                linuxGamepads[i] = new LinuxGamepad(inputDevices[i].uid, inputDevices[i].name);
                linuxGamepads[i]->dev = inputDevices[i].handler;
                linuxGamepads[i]->fd = -1;
                Input::Gamepads.Add(linuxGamepads[i]);
                Input::OnGamepadsChanged();
                LOG(Info, "Gamepad {} added", linuxGamepads[i]->GetName());
                //cout << "Gamepad added." << endl;
            }
            /*
            if (Input::GetGamepadsCount() <= i) {
                Input::Gamepads.Add(linuxGamepads[i]);
                Input::OnGamepadsChanged();
                LOG(Info, "Gamepad {} added again", linuxGamepads[i]->GetName());
            }
            */
        }
        //LOG(Info, "found gamepads: {}, known to Input: {}", foundGamepads, Input::GetGamepadsCount());
    }
    /*
    for (int i = 0; i < foundGamepads; i++)
    {
        linuxGamepads[i]->UpdateState();
    }*/
}

// from WindowsInput.cpp
float NormalizeInputAxis(const int axisVal)
{
    // Normalize [-32768..32767] -> [-1..1]
    const float norm = axisVal <= 0 ? 32768.0f : 32767.0f;
    return float(axisVal) / norm;
}

float NormalizeInputTrigger(const int axisVal)
{
    // Normalize [-32768..32767] -> [-1..1]
    return float(axisVal) / 1023.0f;
}

LinuxGamepad::LinuxGamepad(u_int32_t uid[4], string name) : Gamepad(Guid(uid[0], uid[1], uid[2], uid[3]), String(name.c_str()))
{
    fd = -1;
    for (int i = 0; i < (int32)GamepadButton::MAX; i++)
    {
        _state.Buttons[i] = 0;
    }
    for (int i = 0; i < (int32)GamepadAxis::MAX; i++)
    {
        _state.Axis[i] = 0;
    }
}

LinuxGamepad::~LinuxGamepad()
{
    if (fd >= 0) close(fd);
}

bool LinuxGamepad::UpdateState()
{
    if (fd < 0)
    {
        fd = open(dev.c_str(), O_RDONLY|O_NONBLOCK);
        //cout << "opened " << dev << endl;
    }
    input_event event;
    int caughtEvents = 0;
    for (int i = 0; i < LINUXINPUT_MAX_GAMEPAD_EVENTS_PER_FRAME; i++)
    {
        ssize_t r = read(fd, &event, sizeof(event));
        if (r < 0)
        {
            if (errno != EAGAIN) {
                LOG(Warning, "Lost connection to gamepad, errno={0}", errno);
                close(fd);
                fd = -1;
            }
            break;
        }
        if (r == 0) break;
        if (r < sizeof(event) || r != 24)
        {
            LOG(Warning, "got a shortened package from the kernel {0}", r);
            break;
        }
        if (event.type > EV_MAX)
        {
            LOG(Warning, "got an invalid event type from the kernel {0}", event.type);
            break;
        }
        caughtEvents++;
        //cout << "got an event " << event.type << ", code " << event.code << ", value " << event.value << endl;
        if (event.type == EV_KEY)
        {
            switch (event.code) {
                case BTN_A: _state.Buttons[(int32)GamepadButton::A] = !!event.value; break;
                case BTN_B: _state.Buttons[(int32)GamepadButton::B] = !!event.value; break;
                case BTN_X: _state.Buttons[(int32)GamepadButton::X] = !!event.value; break;
                case BTN_Y: _state.Buttons[(int32)GamepadButton::Y] = !!event.value; break;
                case BTN_TL: _state.Buttons[(int32)GamepadButton::LeftShoulder] = !!event.value; break;
                case BTN_TR: _state.Buttons[(int32)GamepadButton::RightShoulder] = !!event.value; break;
                case BTN_BACK: _state.Buttons[(int32)GamepadButton::Back] = !!event.value; break;
                case BTN_START: _state.Buttons[(int32)GamepadButton::Start] = !!event.value; break;
                case BTN_THUMBL: _state.Buttons[(int32)GamepadButton::LeftThumb] = !!event.value; break;
                case BTN_THUMBR: _state.Buttons[(int32)GamepadButton::RightThumb] = !!event.value; break;
                case BTN_DPAD_UP: _state.Buttons[(int32)GamepadButton::DPadUp] = !!event.value; break;
                case BTN_DPAD_DOWN: _state.Buttons[(int32)GamepadButton::DPadDown] = !!event.value; break;
                case BTN_DPAD_LEFT: _state.Buttons[(int32)GamepadButton::DPadLeft] = !!event.value; break;
                case BTN_DPAD_RIGHT: _state.Buttons[(int32)GamepadButton::DPadRight] = !!event.value; break;
            }
        } else if (event.type == EV_ABS)
        {
            switch (event.code) {
                case ABS_X:
                    _state.Axis[(int32)GamepadAxis::LeftStickX] = NormalizeInputAxis(event.value);
                    _state.Buttons[(int32)GamepadButton::LeftStickLeft] = event.value < -TRIGGER_THRESHOLD;
                    _state.Buttons[(int32)GamepadButton::LeftStickRight] = event.value > TRIGGER_THRESHOLD;
                    break;
                case ABS_Y:
                    _state.Axis[(int32)GamepadAxis::LeftStickY] = NormalizeInputAxis(event.value);
                    _state.Buttons[(int32)GamepadButton::LeftStickUp] = event.value < -TRIGGER_THRESHOLD;
                    _state.Buttons[(int32)GamepadButton::LeftStickDown] = event.value > TRIGGER_THRESHOLD;
                    break;
                case ABS_Z:
                    _state.Axis[(int32)GamepadAxis::LeftTrigger] = NormalizeInputTrigger(event.value);
                    _state.Buttons[(int32)GamepadButton::LeftTrigger] = event.value > 2;
                    break;
                case ABS_RX:
                    _state.Axis[(int32)GamepadAxis::RightStickX] = NormalizeInputAxis(event.value);
                    _state.Buttons[(int32)GamepadButton::RightStickLeft] = event.value < -TRIGGER_THRESHOLD;
                    _state.Buttons[(int32)GamepadButton::RightStickRight] = event.value > TRIGGER_THRESHOLD;
                    break;
                case ABS_RY:
                    _state.Axis[(int32)GamepadAxis::RightStickY] = NormalizeInputAxis(event.value);
                    _state.Buttons[(int32)GamepadButton::RightStickUp] = event.value < -TRIGGER_THRESHOLD;
                    _state.Buttons[(int32)GamepadButton::RightStickDown] = event.value > TRIGGER_THRESHOLD;
                    break;
                case ABS_RZ:
                    _state.Axis[(int32)GamepadAxis::RightTrigger] = NormalizeInputTrigger(event.value);
                    _state.Buttons[(int32)GamepadButton::RightTrigger] = event.value > 2;
                    break;
            }
        }
    }
    if (caughtEvents > 0)
    {
        LOG(Info, "Caught events: {}", caughtEvents);
        cout << "left stick x: " << _state.Axis[(int32)GamepadAxis::LeftStickX] << endl;
        cout << "left stick y: " << _state.Axis[(int32)GamepadAxis::LeftStickY] << endl;
        cout << "left trigger: " << _state.Axis[(int32)GamepadAxis::LeftTrigger] << endl;
        cout << "right stick x: " << _state.Axis[(int32)GamepadAxis::RightStickX] << endl;
        cout << "right stick y: " << _state.Axis[(int32)GamepadAxis::RightStickY] << endl;
        cout << "right trigger: " << _state.Axis[(int32)GamepadAxis::RightTrigger] << endl;
        cout << "button A: " << _state.Buttons[(int32)GamepadButton::A] << endl;
        cout << "layout A: " << (int32)Layout.Buttons[(int32)GamepadButton::A] << endl;
    }
    return false; //caughtEvents == 0;
}

void LinuxInput::DumpDevices()
{
    for (int i = 0; i < foundGamepads; i++) {
        char buf[36];
        snprintf(buf, 36, "%04x %04x %04x %04x", inputDevices[i].uid[0], inputDevices[i].uid[1], inputDevices[i].uid[2], inputDevices[i].uid[3]);
        cout << buf << endl;
        cout << inputDevices[i].name << endl;
        cout << inputDevices[i].handler << endl;
        cout << (inputDevices[i].isGamepad ? "Gamepad" : "other") << endl;
    }
}

#endif