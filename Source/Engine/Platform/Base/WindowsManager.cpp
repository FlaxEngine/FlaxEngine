// Copyright (c) Wojciech Figat. All rights reserved.

#include "../WindowsManager.h"
#include "Engine/Engine/Time.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Engine/EngineService.h"

class WindowsManagerService : public EngineService
{
public:

    WindowsManagerService()
        : EngineService(TEXT("Windows Manager"), -30)
    {
    }

    void Update() override;
    void Dispose() override;
};

CriticalSection WindowsManager::WindowsLocker;
Array<Window*> WindowsManager::Windows;
WindowsManagerService WindowsManagerServiceInstance;

Window* WindowsManager::GetByNativePtr(void* handle)
{
    Window* result = nullptr;
    WindowsLocker.Lock();
    for (int32 i = 0; i < Windows.Count(); i++)
    {
        if (Windows[i]->GetNativePtr() == handle)
        {
            result = Windows[i];
            break;
        }
    }
    WindowsLocker.Unlock();
    return result;
}

void WindowsManager::Register(Window* win)
{
    WindowsLocker.Lock();
    Windows.Add(win);
    WindowsLocker.Unlock();
}

void WindowsManager::Unregister(Window* win)
{
    WindowsLocker.Lock();
    Windows.Remove(win);
    WindowsLocker.Unlock();
}

void WindowsManagerService::Update()
{
    PROFILE_CPU();

    // Update windows
    const float deltaTime = Time::Update.UnscaledDeltaTime.GetTotalSeconds();
    WindowsManager::WindowsLocker.Lock();
    Array<Window*, InlinedAllocation<32>> windows;
    windows.Add(WindowsManager::Windows);
    for (Window* win : windows)
    {
        if (win->IsVisible())
            win->OnUpdate(deltaTime);
    }
    WindowsManager::WindowsLocker.Unlock();
}

void WindowsManagerService::Dispose()
{
    // Close remaining windows
    WindowsManager::WindowsLocker.Lock();
    Array<Window*, InlinedAllocation<32>> windows;
    windows.Add(WindowsManager::Windows);
    for (Window* win : windows)
    {
        win->Close(ClosingReason::EngineExit);
    }
    WindowsManager::WindowsLocker.Unlock();
}
