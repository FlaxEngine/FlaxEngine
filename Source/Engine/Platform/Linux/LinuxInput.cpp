// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_LINUX

#include "LinuxInput.h"
#include "Engine/Core/Log.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include <linux/input.h>
#include <linux/input-event-codes.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <fcntl.h>

struct LinuxInputDevice
{
    uint32 uid[4];
    std::string name;
    std::string handler;
    bool isGamepad;
};

static int foundGamepads;
static float lastUpdateTime;
static LinuxInputDevice inputDevices[LINUXINPUT_MAX_GAMEPADS];

void LinuxInput::Init()
{
    foundGamepads = 0;
    lastUpdateTime = -1000;
}

void LinuxInput::DetectGamePads()
{
    std::string line;
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
            }
            else if (line[0] == 'I')
            {
                int startIndex = 0;
                for (int i = 0; i < 4; i++)
                {
                    int equalsIndex = line.find('=', startIndex);
                    if (equalsIndex > 0)
                    {
                        std::istringstream part(line.substr(equalsIndex + 1, 4));
                        part >> std::hex >> inputDevices[foundGamepads].uid[i];
                    }
                    startIndex = equalsIndex + 1;
                }
            }
            else if (line[0] == 'H')
            {
                int eventIndex = line.find("event");
                if (eventIndex > 0)
                {
                    int end = line.find(' ', eventIndex);
                    if (end > 0)
                    {
                        inputDevices[foundGamepads].handler = "/dev/input/" + line.substr(eventIndex, end - eventIndex);
                    }
                }
            }
            else if (line[0] == 'B')
            {
                int keyIndex = line.find("KEY=");
                if (keyIndex >= 0)
                {
                    // There should be five groups of 64 bits each
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
            }
            else if (line.length() == 0)
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
}

void LinuxInput::UpdateState()
{
    const float time = (float)Platform::GetTimeSeconds();
    if (time - lastUpdateTime > 1.0f)
    {
        PROFILE_CPU_NAMED("Input.ScanGamepads");
        DetectGamePads();
        lastUpdateTime = time;
        for (int i = 0; i < foundGamepads; i++)
        {
            auto& inputDevice = inputDevices[i];

            // Skip duplicates
            bool duplicate = false;
            for (Gamepad* g : Input::Gamepads)
            {
                if (((LinuxGamepad*)g)->dev == inputDevice.handler.c_str())
                {
                    duplicate = true;
                    break;
                }
            }
            if (duplicate)
                continue;

            // Add gamepad
            auto linuxGamepad = New<LinuxGamepad>(inputDevice.uid, String(inputDevice.name.c_str()));
            linuxGamepad->dev = StringAnsi(inputDevice.handler.c_str());
            linuxGamepad->fd = -1;
            Input::Gamepads.Add(linuxGamepad);
            Input::OnGamepadsChanged();
            LOG(Info, "Added gamepad '{}'", linuxGamepad->GetName());
        }
    }
}

float NormalizeInputAxis(const int axisVal)
{
    // Normalize [-32768..32767] -> [-1..1]
    const float norm = axisVal <= 0 ? 32768.0f : 32767.0f;
    return float(axisVal) / norm;
}

float NormalizeInputTrigger(const int axisVal)
{
    // Normalize [-1023..1023] -> [-1..1]
    return float(axisVal) / 1023.0f;
}

LinuxGamepad::LinuxGamepad(uint32 uid[4], const String& name)
    : Gamepad(Guid(uid[0], uid[1], uid[2], uid[3]), name)
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
    if (fd >= 0)
        close(fd);
}

bool LinuxGamepad::UpdateState()
{
    if (fd < 0)
    {
        fd = open(dev.Get(), O_RDONLY | O_NONBLOCK);
        //std::cout << "opened " << dev.Get() << std::endl;
    }
    input_event event;
    int caughtEvents = 0;
    for (int i = 0; i < LINUXINPUT_MAX_GAMEPAD_EVENTS_PER_FRAME; i++)
    {
        ssize_t r = read(fd, &event, sizeof(event));
        if (r < 0)
        {
            if (errno != EAGAIN)
            {
                LOG(Warning, "Lost connection to gamepad '{1}', errno={0}", errno, GetName());
                close(fd);
                fd = -1;
                return true;
            }
            break;
        }
        if (r == 0) break;
        if (r < sizeof(event) || r != 24)
        {
            LOG(Warning, "Gamepad '{1}' got a shortened package from the kernel {0}", r, GetName());
            break;
        }
        if (event.type > EV_MAX)
        {
            LOG(Warning, "Gamepad '{1}' got an invalid event type from the kernel {0}", event.type, GetName());
            break;
        }
        caughtEvents++;
        //std::cout << "got an event " << event.type << ", code " << event.code << ", value " << event.value << std::endl;
        if (event.type == EV_KEY)
        {
            switch (event.code)
            {
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
        }
        else if (event.type == EV_ABS)
        {
            switch (event.code)
            {
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

    /*if (caughtEvents > 0)
    {
        LOG(Info, "Caught events: {}", caughtEvents);
        std::cout << "left stick x: " << _state.Axis[(int32)GamepadAxis::LeftStickX] << std::endl;
        std::cout << "left stick y: " << _state.Axis[(int32)GamepadAxis::LeftStickY] << std::endl;
        std::cout << "left trigger: " << _state.Axis[(int32)GamepadAxis::LeftTrigger] << std::endl;
        std::cout << "right stick x: " << _state.Axis[(int32)GamepadAxis::RightStickX] << std::endl;
        std::cout << "right stick y: " << _state.Axis[(int32)GamepadAxis::RightStickY] << std::endl;
        std::cout << "right trigger: " << _state.Axis[(int32)GamepadAxis::RightTrigger] << std::endl;
        std::cout << "button A: " << _state.Buttons[(int32)GamepadButton::A] << std::endl;
        std::cout << "layout A: " << (int32)Layout.Buttons[(int32)GamepadButton::A] << std::endl;
    }*/

    return false;
}

#if BUILD_DEBUG

void LinuxInput::DumpDevices()
{
    for (int i = 0; i < foundGamepads; i++)
    {
        char buf[36];
        snprintf(buf, 36, "%04x %04x %04x %04x", inputDevices[i].uid[0], inputDevices[i].uid[1], inputDevices[i].uid[2], inputDevices[i].uid[3]);
        std::cout << buf << std::endl;
        std::cout << inputDevices[i].name << std::endl;
        std::cout << inputDevices[i].handler << std::endl;
        std::cout << (inputDevices[i].isGamepad ? "Gamepad" : "other") << std::endl;
    }
}

#endif

#endif
