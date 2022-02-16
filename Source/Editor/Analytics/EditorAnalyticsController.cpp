// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#include "EditorAnalyticsController.h"
#include "Editor/Cooker/GameCooker.h"
#include "EditorAnalytics.h"
#include "Engine/ShadowsOfMordor/Builder.h"

void RegisterGameCookingStart(GameCooker::EventType type)
{
    auto& data = *GameCooker::GetCurrentData();
    auto platform = ToString(data.Platform);
    if (type == GameCooker::EventType::BuildStarted)
    {
        EditorAnalytics::SendEvent("Actions", "GameCooker.Start", platform);
    }
    else if (type == GameCooker::EventType::BuildFailed)
    {
        EditorAnalytics::SendEvent("Actions", "GameCooker.Failed", platform);
    }
    else if (type == GameCooker::EventType::BuildDone)
    {
        EditorAnalytics::SendEvent("Actions", "GameCooker.End", platform);
    }
}

void RegisterLightmapsBuildingStart()
{
    EditorAnalytics::SendEvent("Actions", "ShadowsOfMordor.Build", "ShadowsOfMordor.Build");
}

void RegisterError(LogType type, const StringView& msg)
{
    if (type == LogType::Error && false)
    {
        String value = msg.ToString();
        const int32 MaxLength = 300;
        if (msg.Length() > MaxLength)
            value = value.Substring(0, MaxLength);
        value.Replace('\n', ' ');
        value.Replace('\r', ' ');

        EditorAnalytics::SendEvent("Errors", "Log.Error", value);
    }
    else if (type == LogType::Fatal)
    {
        String value = msg.ToString();
        const int32 MaxLength = 300;
        if (msg.Length() > MaxLength)
            value = value.Substring(0, MaxLength);
        value.Replace('\n', ' ');
        value.Replace('\r', ' ');

        EditorAnalytics::SendEvent("Errors", "Log.Fatal", value);
    }
}

void EditorAnalyticsController::Init()
{
    GameCooker::OnEvent.Bind<RegisterGameCookingStart>();
    ShadowsOfMordor::Builder::Instance()->OnBuildStarted.Bind<RegisterLightmapsBuildingStart>();
    Log::Logger::OnError.Bind<RegisterError>();
}

void EditorAnalyticsController::Cleanup()
{
    GameCooker::OnEvent.Unbind<RegisterGameCookingStart>();
    ShadowsOfMordor::Builder::Instance()->OnBuildStarted.Unbind<RegisterLightmapsBuildingStart>();
    Log::Logger::OnError.Unbind<RegisterError>();
}
