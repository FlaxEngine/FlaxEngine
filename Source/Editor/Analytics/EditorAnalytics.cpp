// Copyright (c) Wojciech Figat. All rights reserved.

#include "EditorAnalytics.h"
#include "Editor/Editor.h"
#include "Editor/ProjectInfo.h"
#include "Editor/Cooker/GameCooker.h"
#include "Engine/Threading/Task.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Platform/FileSystem.h"
#include "Engine/Platform/MemoryStats.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Types/DateTime.h"
#include "Engine/Core/Types/TimeSpan.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Engine/Globals.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Utilities/StringConverter.h"
#include "Engine/Utilities/TextWriter.h"
#include "Engine/ShadowsOfMordor/Builder.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "FlaxEngine.Gen.h"
#include <ThirdParty/UniversalAnalytics/http.h>

// Docs:
// https://developers.google.com/analytics/devguides/collection/ga4
// https://developers.google.com/analytics/devguides/collection/protocol/ga4

// [GA4] Flax Editor
#define GA_MEASUREMENT_ID "G-2SNY6RW6VX"
#define GA_API_SECRET "wFlau4khTPGFRnx-AIZ1zg"
#define GA_DEBUG 0
#if GA_DEBUG
#define GA_URL "https://www.google-analytics.com/debug/mp/collect"
#else
#define GA_URL "https://www.google-analytics.com/mp/collect"
#endif

namespace
{
    StringAnsi Url;
    StringAnsi ClientId;
    DateTime SessionStartTime;
    CriticalSection Locker;
    bool IsSessionActive = false;
    TextWriterANSI JsonBuffer;
    curl_slist* CurlHttpHeadersList = nullptr;
}

size_t curl_null_data_handler(char* ptr, size_t size, size_t nmemb, void* userdata)
{
    return nmemb * size;
}

void RegisterGameCookingStart(GameCooker::EventType type)
{
    if (type == GameCooker::EventType::BuildStarted)
    {
        auto& data = *GameCooker::GetCurrentData();
        StringAnsi name = "Build " + StringAnsi(ToString(data.Platform));
        const Pair<const char*, const char*> params[1] = { { "GameCooker", name.Get() } };
        EditorAnalytics::SendEvent("Actions", ToSpan(params, ARRAY_COUNT(params)));
    }
}

void RegisterLightmapsBuildingStart()
{
    const Pair<const char*, const char*> params[1] = { { "ShadowsOfMordor", "Build" }, };
    EditorAnalytics::SendEvent("Actions", ToSpan(params, ARRAY_COUNT(params)));
}

void RegisterError(LogType type, const StringView& msg)
{
    if (type == LogType::Error && false)
    {
        StringAnsi value(msg);
        const int32 MaxLength = 300;
        if (msg.Length() > MaxLength)
            value = value.Substring(0, MaxLength);
        value.Replace('\n', ' ');
        value.Replace('\r', ' ');
        const Pair<const char*, const char*> params[1] = { { "Error", value.Get() }, };
        EditorAnalytics::SendEvent("Errors", ToSpan(params, ARRAY_COUNT(params)));
    }
    else if (type == LogType::Fatal)
    {
        StringAnsi value(msg);
        const int32 MaxLength = 300;
        if (msg.Length() > MaxLength)
            value = value.Substring(0, MaxLength);
        value.Replace('\n', ' ');
        value.Replace('\r', ' ');
        const Pair<const char*, const char*> params[1] = { { "Fatal", value.Get() }, };
        EditorAnalytics::SendEvent("Errors", ToSpan(params, ARRAY_COUNT(params)));
    }
}

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
    return ::IsSessionActive;
}

void EditorAnalytics::StartSession()
{
    ScopeLock lock(Locker);
    if (::IsSessionActive)
        return;
    PROFILE_CPU();

    // Prepare client metadata
    ClientId = Platform::GetUniqueDeviceId().ToString(Guid::FormatType::N).ToStringAnsi();
    StringAnsi ProjectName = Editor::Project->Name.ToStringAnsi();
    const auto desktopSize = Platform::GetDesktopSize();
    StringAnsi ScreenResolution = StringAnsi::Format("{0}x{1}", (int32)desktopSize.X, (int32)desktopSize.Y);
    const auto memoryStats = Platform::GetMemoryStats();
    StringAnsi Memory = StringAnsi::Format("{0} GB", (int32)(memoryStats.TotalPhysicalMemory / 1024 / 1024 / 1000));
    StringAnsi UserLocale = Platform::GetUserLocaleName().ToStringAnsi();
    StringAnsi GPU;
    if (GPUDevice::Instance && GPUDevice::Instance->GetState() == GPUDevice::DeviceState::Ready)
        GPU = StringAsANSI<>(GPUDevice::Instance->GetAdapter()->GetDescription().GetText()).Get();
    SessionStartTime = DateTime::Now();
    StringAnsiView EngineVersion = FLAXENGINE_VERSION_TEXT;
#if PLATFORM_WINDOWS
    StringAnsiView PlatformName = "Windows";
#elif PLATFORM_LINUX
    StringAnsiView PlatformName = "Linux";
#elif PLATFORM_MAC
    StringAnsiView PlatformName = "Mac";
#else
#error "Unknown platform"
#endif

    // Initialize HTTP
    Url = StringAnsi::Format("{0}?measurement_id={1}&api_secret={2}", GA_URL, GA_MEASUREMENT_ID, GA_API_SECRET);
    curl_global_init(CURL_GLOBAL_ALL);
    CurlHttpHeadersList = curl_slist_append(nullptr, "Content-Type: application/json");
    ::IsSessionActive = true;

    // Start session
    {
        const Pair<const char*, const char*> params[1] = { { "Project", ProjectName.Get() }, };
        SendEvent("Session", ToSpan(params, ARRAY_COUNT(params)));
    }

    // Report telemetry stats
#define SEND_TELEMETRY(name, value) \
    if (value.HasChars()) \
    { \
        const Pair<const char*, const char*> params[1] = { { name, value.Get() } }; \
        SendEvent("Telemetry", ToSpan(params, ARRAY_COUNT(params))); \
    }
    SEND_TELEMETRY("Platform", PlatformName);
    SEND_TELEMETRY("GPU", GPU);
    SEND_TELEMETRY("Memory", Memory);
    SEND_TELEMETRY("Locale", UserLocale);
    SEND_TELEMETRY("Screen", ScreenResolution);
    SEND_TELEMETRY("Version", EngineVersion);
#undef SEND_TELEMETRY

    // Bind events
    GameCooker::OnEvent.Bind<RegisterGameCookingStart>();
    ShadowsOfMordor::Builder::Instance()->OnBuildStarted.Bind<RegisterLightmapsBuildingStart>();
    Log::Logger::OnError.Bind<RegisterError>();
}

void EditorAnalytics::EndSession()
{
    ScopeLock lock(Locker);
    if (!::IsSessionActive)
        return;
    PROFILE_CPU();

    // Unbind events
    GameCooker::OnEvent.Unbind<RegisterGameCookingStart>();
    ShadowsOfMordor::Builder::Instance()->OnBuildStarted.Unbind<RegisterLightmapsBuildingStart>();
    Log::Logger::OnError.Unbind<RegisterError>();

    // End session
    {
        StringAnsi sessionLength = StringAnsi::Format("{}", (int32)(DateTime::Now() - SessionStartTime).GetTotalSeconds());
        const Pair<const char*, const char*> params[1] =
        {
            { "Duration", sessionLength.Get() },
        };
        SendEvent("Session", ToSpan(params, ARRAY_COUNT(params)));
    }

    // Cleanup
    curl_slist_free_all(CurlHttpHeadersList);
    CurlHttpHeadersList = nullptr;
    curl_global_cleanup();
    ::IsSessionActive = false;
}

void EditorAnalytics::SendEvent(const char* name, Span<Pair<const char*, const char*>> parameters)
{
    ScopeLock lock(Locker);
    if (!::IsSessionActive)
        return;
    PROFILE_CPU();

    // Create Json request contents
    JsonBuffer.Clear();
    JsonBuffer.Write("{ \"client_id\": \"");
    JsonBuffer.Write(ClientId);
    JsonBuffer.Write("\", \"events\": [ { \"name\": \"");
    JsonBuffer.Write(name);
    JsonBuffer.Write("\", \"params\": {");
    for (int32 i = 0; i < parameters.Length(); i++)
    {
        if (i != 0)
            JsonBuffer.Write(",");
        const auto& e = parameters[i];
        JsonBuffer.Write("\"");
        JsonBuffer.Write(e.First);
        JsonBuffer.Write("\":\"");
        JsonBuffer.Write(e.Second);
        JsonBuffer.Write("\"");
    }
    JsonBuffer.Write("}}]}");
    const StringAnsiView json((const char*)JsonBuffer.GetBuffer()->GetHandle(), (int32)JsonBuffer.GetBuffer()->GetPosition());

    // Send HTTP request
    CURL* curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl, CURLOPT_URL, Url.Get());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, CurlHttpHeadersList);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json.Get());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, json.Length());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Flax Editor");
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, curl);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_null_data_handler);
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
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
    Task::StartNew(EditorAnalytics::StartSession);

    return false;
}

void EditorAnalyticsService::Dispose()
{
    EditorAnalytics::EndSession();
}
