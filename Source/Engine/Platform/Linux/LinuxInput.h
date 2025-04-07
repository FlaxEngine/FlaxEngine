// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_LINUX

#include "Engine/Input/Input.h"
#include "Engine/Input/Gamepad.h"

#define LINUXINPUT_MAX_GAMEPADS 8
#define LINUXINPUT_MAX_GAMEPAD_EVENTS_PER_FRAME 32
#define TRIGGER_THRESHOLD 1000

class FLAXENGINE_API LinuxGamepad : public Gamepad
{
public:
    int fd;
    StringAnsi dev;

    LinuxGamepad(uint32 uid[], const String& name);
    ~LinuxGamepad();
    bool UpdateState();
};

class LinuxInput
{
public:
    static void UpdateState();
    static void DetectGamePads();
#if BUILD_DEBUG
    static void DumpDevices();
#endif
    static void Init();
};

#endif
