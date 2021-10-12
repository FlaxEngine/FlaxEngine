#if PLATFORM_LINUX

#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <fcntl.h>
#include "LinuxInput.h"

using namespace std;

void LinuxInput::DetectGamePads()
{
    string line;
    std::ifstream devs(LinuxInput::InputDevicesFile);
    
    InputDevice * inputDevice = new InputDevice();
    foundGamepads = 0;
    if (devs.is_open())
    {
        while (getline(devs, line))
        {
            if (line[0] == 'N')
            {
                int quoteIndex = line.find('"');
                inputDevice->name = line.substr(quoteIndex+1, line.length() - quoteIndex - 2);
            } else if (line[0] == 'I')
            {
                int startIndex = 0;
                for (int i = 0; i < 4; i++) {
                    int equalsIndex = line.find('=', startIndex);
                    if (equalsIndex > 0) {
                        istringstream part(line.substr(equalsIndex+1, 4));
                        part >> std::hex >> inputDevice->uid[i];
                    }
                    startIndex = equalsIndex+1;
                }
            } else if (line[0] == 'H')
            {
                int eventIndex = line.find("event");
                if (eventIndex > 0) {
                    int end = line.find(' ', eventIndex);
                    if (end > 0) {
                        inputDevice->handler = "/dev/input/" + line.substr(eventIndex, end - eventIndex);
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
                    inputDevice->isGamepad = (msb & 1lu<<48) > 0;
                    if (inputDevice->isGamepad)
                    {
                        inputDevices[foundGamepads++] = *inputDevice;
                    }
                }
            }
            else if (line.size() == 0)
            {
                if (!inputDevice->isGamepad) delete inputDevice;
                inputDevice = new InputDevice();
            }
        }
        devs.close();
    }
};

void LinuxInput::UpdateState()
{
    for (int i = 0; i < foundGamepads; i++)
    {
        if (linuxGamepads[i] == NULL)
        {
            linuxGamepads[i] = new LinuxGamepad(inputDevices[i].uid, inputDevices[i].name);
            linuxGamepads[i]->dev = inputDevices->handler;
            linuxGamepads[i]->fd = -1;
            Input::Gamepads.Add(linuxGamepads[i]);
            Input::OnGamepadsChanged();
        }
    }
    for (int i = 0; i < foundGamepads; i++)
    {
        linuxGamepads[i]->UpdateState();
    }
}

static void Update()
{
    if (LinuxInput::singleton == nullptr)
    {
        LinuxInput::singleton = new LinuxInput();
    }
    LinuxInput::singleton->UpdateState();
}

// from WindowsInput.cpp
float NormalizeInputAxis(const int axisVal)
{
    // Normalize [-32768..32767] -> [-1..1]
    const float norm = axisVal <= 0 ? 32768.0f : 32767.0f;
    return float(axisVal) / norm;
}

LinuxGamepad::LinuxGamepad(u_int32_t uid[4], string name) : Gamepad(Guid(uid[0], uid[1], uid[2], uid[3]), String(name.c_str()))
{
    fd = -1;
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
    }
    input_event event;
    for (int i = 0; i < LINUXINPUT_MAX_GAMEPAD_EVENTS_PER_FRAME; i++)
    {
        ssize_t r = read(fd, &event, sizeof(event));
        if (r <= 0) break;
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
                    _state.Axis[(int32)GamepadAxis::LeftTrigger] = NormalizeInputAxis(event.value);
                    _state.Buttons[(int32)GamepadButton::LeftTrigger] = event.value > TRIGGER_THRESHOLD;
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
                    _state.Axis[(int32)GamepadAxis::RightTrigger] = NormalizeInputAxis(event.value);
                    _state.Buttons[(int32)GamepadButton::RightTrigger] = event.value > TRIGGER_THRESHOLD;
                    break;
            }
        }
    }
    return false;
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