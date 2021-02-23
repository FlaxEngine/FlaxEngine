// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "EditorAnalytics.h"
#include "EditorAnalyticsController.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Types/DateTime.h"
#include "Editor/Editor.h"
#include "Editor/ProjectInfo.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Utilities/StringConverter.h"
#include "FlaxEngine.Gen.h"
#include <ThirdParty/UniversalAnalytics/universal-analytics.h>

#define FLAX_EDITOR_GOOGLE_ID "UA-88357703-3"

// Helper doc: https://developers.google.com/analytics/devguides/collection/protocol/v1/parameters

namespace EditorAnalyticsImpl
{
    UATracker Tracker = nullptr;

    StringAnsi ClientId;
    StringAnsi ProjectName;
    StringAnsi ScreenResolution;
    StringAnsi UserLanguage;
    StringAnsi GPU;
    DateTime SessionStartTime;

    CriticalSection Locker;
    bool IsSessionActive = false;
    EditorAnalyticsController Controller;
    Array<char> TmpBuffer;
}

using namespace EditorAnalyticsImpl;

class EditorAnalyticsService : public EngineService
{
public:

    EditorAnalyticsService()
        : EngineService(TEXT("Editor Analytics"))
    {
    }

    bool Init() override;
    void Dispose() override;
};

EditorAnalyticsService EditorAnalyticsServiceInstance;

bool EditorAnalytics::IsSessionActive()
{
    return EditorAnalyticsImpl::IsSessionActive;
}

void EditorAnalytics::StartSession()
{
    ScopeLock lock(Locker);

    if (EditorAnalyticsImpl::IsSessionActive)
        return;

    // Prepare client metadata
    if (ClientId.IsEmpty())
    {
        ClientId = Platform::GetUniqueDeviceId().ToString(Guid::FormatType::N).ToStringAnsi();
    }
    if (ScreenResolution.IsEmpty())
    {
        const Vector2 desktopSize = Platform::GetDesktopSize();
        ScreenResolution = StringAnsi(StringUtils::ToString((int32)desktopSize.X)) + "x" + StringAnsi(StringUtils::ToString((int32)desktopSize.Y));
    }
    if (UserLanguage.IsEmpty())
    {
        UserLanguage = Platform::GetUserLocaleName().ToStringAnsi();
    }
    if (GPU.IsEmpty())
    {
        const auto gpu = GPUDevice::Instance;
        if (gpu && gpu->GetState() == GPUDevice::DeviceState::Ready)
            GPU = StringAsANSI<>(gpu->GetAdapter()->GetDescription().GetText()).Get();
    }
    if (ProjectName.IsEmpty())
    {
        ProjectName = Editor::Project->Name.ToStringAnsi();
    }
    SessionStartTime = DateTime::Now();

    // Initialize the analytics tracker
    Tracker = createTracker(FLAX_EDITOR_GOOGLE_ID, ClientId.Get(), nullptr);
    Tracker->user_agent = "Flax Editor";

    // Store these options permanently (for the lifetime of the tracker)
    setTrackerOption(Tracker, UA_OPTION_QUEUE, 1);
    UASettings GlobalSettings =
    {
        {
            { UA_DOCUMENT_PATH, 0, "Flax Editor" },
            { UA_DOCUMENT_TITLE, 0, "Flax Editor" },
#if PLATFORM_WINDOWS
            { UA_USER_AGENT, 0, "Windows " FLAXENGINE_VERSION_TEXT },
#elif PLATFORM_LINUX
            { UA_USER_AGENT, 0, "Linux " FLAXENGINE_VERSION_TEXT },
#else
#error "Unknown platform"
#endif
            { UA_ANONYMIZE_IP, 0, "0" },
            { UA_APP_ID, 0, "Flax Editor " FLAXENGINE_VERSION_TEXT },
            { UA_APP_INSTALLER_ID, 0, "Flax Editor" },
            { UA_APP_NAME, 0, "Flax Editor" },
            { UA_APP_VERSION, 0, FLAXENGINE_VERSION_TEXT },
            { UA_SCREEN_NAME, 0, "Flax Editor " FLAXENGINE_VERSION_TEXT },
            { UA_SCREEN_RESOLUTION, 0, ScreenResolution.Get() },
            { UA_USER_LANGUAGE, 0, UserLanguage.Get() },
        }
    };
    setParameters(Tracker, &GlobalSettings);

    // Send the initial session event
    UAOptions sessionViewOptions =
    {
        {
            { UA_EVENT_CATEGORY, 0, "Session" },
            { UA_EVENT_ACTION, 0, "Start Editor" },
            { UA_EVENT_LABEL, 0, "Start Editor" },
            { UA_SESSION_CONTROL, 0, "start" },
            { UA_DOCUMENT_TITLE, 0, ProjectName.Get() },
        }
    };
    sendTracking(Tracker, UA_SCREENVIEW, &sessionViewOptions);

    EditorAnalyticsImpl::IsSessionActive = true;

    Controller.Init();

    // Report GPU model
    if (GPU.HasChars())
    {
        SendEvent("Telemetry", "GPU.Model", GPU.Get());
    }
}

void EditorAnalytics::EndSession()
{
    ScopeLock lock(Locker);

    if (!EditorAnalyticsImpl::IsSessionActive)
        return;

    Controller.Cleanup();

    StringAnsi sessionLength = StringAnsi::Format("{0}", (int32)(DateTime::Now() - SessionStartTime).GetTotalSeconds());

    // Send the end session event
    UAOptions sessionEventOptions =
    {
        {
            { UA_EVENT_CATEGORY, 0, "Session" },
            { UA_EVENT_ACTION, 0, "Session Length" },
            { UA_EVENT_LABEL, 0, "Session Length" },
            { UA_EVENT_VALUE, 0, sessionLength.Get() },
            { UA_CUSTOM_DIMENSION, 1, "Session Length" },
            { UA_CUSTOM_METRIC, 1, sessionLength.Get() },
        }
    };
    sendTracking(Tracker, UA_EVENT, &sessionEventOptions);

    // Send the end session event
    UAOptions sessionViewOptions =
    {
        {
            { UA_EVENT_CATEGORY, 0, "Session" },
            { UA_EVENT_ACTION, 0, "End Editor" },
            { UA_EVENT_LABEL, 0, "End Editor" },
            { UA_EVENT_VALUE, 0, sessionLength.Get() },
            { UA_SESSION_CONTROL, 0, "end" },
        }
    };
    sendTracking(Tracker, UA_SCREENVIEW, &sessionViewOptions);

    // Cleanup
    removeTracker(Tracker);
    Tracker = nullptr;

    EditorAnalyticsImpl::IsSessionActive = false;
}

void EditorAnalytics::SendEvent(const char* category, const char* name, const char* label)
{
    ScopeLock lock(Locker);

    if (!EditorAnalyticsImpl::IsSessionActive)
        return;

    UAOptions opts =
    {
        {
            { UA_EVENT_CATEGORY, 0, (char*)category },
            { UA_EVENT_ACTION, 0, (char*)name },
            { UA_EVENT_LABEL, 0, (char*)label },
        }
    };

    sendTracking(Tracker, UA_EVENT, &opts);
}

void EditorAnalytics::SendEvent(const char* category, const char* name, const StringView& label)
{
    SendEvent(category, name, label.Get());
}

void EditorAnalytics::SendEvent(const char* category, const char* name, const Char* label)
{
    ScopeLock lock(Locker);

    if (!EditorAnalyticsImpl::IsSessionActive)
        return;

    ASSERT(category && name && label);

    const int32 labelLength = StringUtils::Length(label);
    TmpBuffer.Clear();
    TmpBuffer.Resize(labelLength + 1);
    StringUtils::ConvertUTF162ANSI(label, TmpBuffer.Get(), labelLength);
    TmpBuffer[labelLength] = 0;

    UAOptions opts =
    {
        {
            { UA_EVENT_CATEGORY, 0, (char*)category },
            { UA_EVENT_ACTION, 0, (char*)name },
            { UA_EVENT_LABEL, 0, (char*)TmpBuffer.Get() },
        }
    };

    sendTracking(Tracker, UA_EVENT, &opts);
}

bool EditorAnalyticsService::Init()
{
#if COMPILE_WITH_DEV_ENV
    // Disable analytics from the dev (internal) builds
    LOG(Info, "Editor analytics service is disabled in dev builds.");
    return false;
#endif

    // If file '<editor_install_root>/noTracking' exists then do not use analytics (per engine instance)
    // If file '%appdata/Flax/noTracking' exists then do not use analytics (globally)
    String appDataPath;
    FileSystem::GetSpecialFolderPath(SpecialFolder::AppData, appDataPath);
    if (FileSystem::FileExists(Globals::StartupFolder / TEXT("noTracking")) ||
        FileSystem::FileExists(appDataPath / TEXT("Flax/noTracking")))
    {
        LOG(Info, "Editor analytics service is disabled.");
        return false;
    }

    LOG(Info, "Editor analytics service is enabled. Curl version: {0}", TEXT(LIBCURL_VERSION));

    EditorAnalytics::StartSession();

    return false;
}

void EditorAnalyticsService::Dispose()
{
    EditorAnalytics::EndSession();
}
