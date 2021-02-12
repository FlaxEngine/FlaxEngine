// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "../Game.h"

#if !USE_EDITOR

#include "Engine/Engine/Time.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Platform/Window.h"
#include "Engine/Profiler/Profiler.h"
#include "Engine/Level/Level.h"
#include "Engine/Core/Config/GameSettings.h"
#include "Engine/Content/Content.h"
#include "Engine/Content/Assets/Texture.h"
#include "Engine/Content/JsonAsset.h"
#include "Engine/Content/AssetReference.h"
#include "Engine/Graphics/RenderTask.h"
#include "Engine/Render2D/Render2D.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Utilities/Encryption.h"
#include "Engine/Core/Log.h"
#include "FlaxEngine.Gen.h"

namespace GameBaseImpl
{
    GameHeaderFlags HeaderFlags = GameHeaderFlags::None;
    Guid SplashScreenId;
    float SplashScreenTime = 0.0f;
    AssetReference<JsonAsset> FirstScene;
    AssetReference<Texture> SplashScreen;

    void OnMainWindowClosed();
    void OnPostRender(GPUContext* context, RenderContext& renderContext);
    void OnSplashScreenEnd();
}

bool GameBase::IsShowingSplashScreen()
{
    return GameBaseImpl::SplashScreenTime > ZeroTolerance;
}

void GameBase::InitMainWindowSettings(CreateWindowSettings& settings)
{
}

int32 GameBase::LoadProduct()
{
    // Startup and project root paths are the same in build game
    Globals::ProjectFolder = Globals::StartupFolder;

    // Load build game header file
    {
        int32 tmp;
        Array<byte> data;
        FileReadStream* stream = nullptr;

#if 1
        // Open file
        stream = FileReadStream::Open(Globals::ProjectFolder / TEXT("Content/head"));
        if (stream == nullptr)
            goto LOAD_GAME_HEAD_FAILED;

        // Check header
        stream->ReadInt32(&tmp);
        if (tmp != ('x' + 'D') * 131)
            goto LOAD_GAME_HEAD_FAILED;

        // Don't allow to load game packaged by the other engine version
        stream->ReadInt32(&tmp);
        if (tmp != FLAXENGINE_VERSION_BUILD)
            goto LOAD_GAME_HEAD_FAILED;

        // Load game primary data
        stream->ReadArray(&data);
        if (data.Count() != 808 + sizeof(Guid))
            goto LOAD_GAME_HEAD_FAILED;
        Encryption::DecryptBytes(data.Get(), data.Count());
        Globals::ProductName = (const Char*)data.Get();
        Globals::CompanyName = (const Char*)(data.Get() + 400);
        GameBaseImpl::HeaderFlags = (GameHeaderFlags)*(int32*)(data.Get() + 800);
        Globals::ContentKey = *(int32*)(data.Get() + 804);
        GameBaseImpl::SplashScreenId = *(Guid*)(data.Get() + 808);

        goto LOAD_GAME_HEAD_DONE;

    LOAD_GAME_HEAD_FAILED:
        Platform::Fatal(TEXT("Cannot load game."));
        return -3;

    LOAD_GAME_HEAD_DONE:
        Delete(stream);
#else
        // Mock the settings to debug executable without content
        Globals::ProductName = TEXT("Flax");
        Globals::CompanyName = TEXT("Flax");
        GameBaseImpl::HeaderFlags = GameHeaderFlags::ShowSplashScreen;
        GameBaseImpl::SplashScreenId = Guid::Empty;
        Globals::ContentKey = 0;
#endif
    }

    return 0;
}

bool GameBase::Init()
{
    // Preload splash screen texture
    if (GameBaseImpl::HeaderFlags & GameHeaderFlags::ShowSplashScreen)
    {
        LOG(Info, "Loading splash screen");
        if (GameBaseImpl::SplashScreenId)
            GameBaseImpl::SplashScreen = Content::LoadAsync<Texture>(GameBaseImpl::SplashScreenId);
        else
            GameBaseImpl::SplashScreen = Content::LoadAsyncInternal<Texture>(TEXT("Engine/Textures/Logo"));
        if (!GameBaseImpl::SplashScreen)
        {
            LOG(Error, "Missing splash screen texture!");
        }
    }

    // Preload first scene asset data
    const auto gameSettings = GameSettings::Get();
    if (!gameSettings)
        return true;
    GameBaseImpl::FirstScene = gameSettings->FirstScene;

    return false;
}

void GameBase::BeforeRun()
{
    // Headless more case (no window)
    if (Engine::IsHeadless())
    {
        GameBaseImpl::OnSplashScreenEnd();
        return;
    }

    // Show game window
    LOG(Info, "Showing game window");
    ASSERT(Engine::MainWindow);
    Engine::MainWindow->Show();

    // Show splash screen if it should be used
    if (GameBaseImpl::SplashScreen && GPUDevice::Instance && GPUDevice::Instance->GetRendererType() != RendererType::Null)
    {
        LOG(Info, "Showing splash screen");
        if (MainRenderTask::Instance == nullptr)
            Platform::Fatal(TEXT("Missing main rendering task object."));
        GameBaseImpl::SplashScreenTime = 0;
        MainRenderTask::Instance->PostRender.Bind(&GameBaseImpl::OnPostRender);
    }
    else
    {
        GameBaseImpl::OnSplashScreenEnd();
    }
}

void GameBase::BeforeExit()
{
}

Window* GameBase::CreateMainWindow()
{
    CreateWindowSettings settings;
    settings.Title = *Globals::ProductName;
    settings.AllowDragAndDrop = false;
    settings.Fullscreen = true;
    settings.HasSizingFrame = false;
    settings.HasBorder = false;
    settings.AllowMaximize = true;
    settings.AllowMinimize = true;
    settings.Size = Platform::GetDesktopSize();
    settings.Position = Vector2::Zero;

    Game::InitMainWindowSettings(settings);

    const auto window = Platform::CreateWindow(settings);
    if (window)
    {
        window->Closed.Bind(&GameBaseImpl::OnMainWindowClosed);
    }

    return window;
}

void GameBaseImpl::OnMainWindowClosed()
{
    const auto window = Engine::MainWindow;
    if (!window)
        return;

    // Clear field (window is deleting itself)
    Engine::MainWindow = nullptr;

    // Request engine exit
    Globals::IsRequestingExit = true;
}

void GameBaseImpl::OnPostRender(GPUContext* context, RenderContext& renderContext)
{
    // Handle missing splash screen texture case
    if (SplashScreen == nullptr)
    {
        OnSplashScreenEnd();
        return;
    }

    // Wait for texture loaded before showing splash screen
    if (!SplashScreen->IsLoaded() || SplashScreen.Get()->GetTexture()->MipLevels() != SplashScreen.Get()->StreamingTexture()->TotalMipLevels())
    {
        return;
    }
    const auto splashScreen = SplashScreen.Get()->GetTexture();

    // Configuration
    const float fadeTime = 0.5f;
    const float showTime = 1.0f;
    const float totalTime = fadeTime + showTime + fadeTime;

    // Update animation
    const auto deltaTime = static_cast<float>(Time::Draw.UnscaledDeltaTime.GetTotalSeconds());
    SplashScreenTime += deltaTime;
    if (SplashScreenTime >= totalTime)
    {
        OnSplashScreenEnd();
        return;
    }

    PROFILE_GPU_CPU_NAMED("Splash Screen");

    // Calculate visibility
    float fade = 1.0f;
    if (SplashScreenTime < fadeTime)
        fade = SplashScreenTime / fadeTime;
    else if (SplashScreenTime > fadeTime + showTime)
        fade = 1.0f - (SplashScreenTime - fadeTime - showTime) / fadeTime;

    // Calculate image area (fill screen, keep aspect ratio, and snap to pixels)
    const auto viewport = renderContext.Task->GetViewport();
    const Rectangle screenRect(viewport.X, viewport.Y, viewport.Width, viewport.Height);
    Rectangle imageArea = screenRect;
    imageArea.Scale(0.6f);
    const float aspectRatio = static_cast<float>(splashScreen->Width()) / splashScreen->Height();
    const float height = imageArea.GetWidth() / aspectRatio;
    imageArea.Location.Y += (imageArea.GetHeight() - height) * 0.5f;
    imageArea.Size.Y = height;
    imageArea.Location = Vector2::Ceil(imageArea.Location);
    imageArea.Size = Vector2::Ceil(imageArea.Size);

    // Draw
    Render2D::Begin(GPUDevice::Instance->GetMainContext(), renderContext.Task->GetOutputView(), nullptr, viewport);
    Render2D::FillRectangle(screenRect, Color::Black);
    Render2D::DrawTexture(splashScreen, imageArea, Color(1.0f, 1.0f, 1.0f, fade));
    Render2D::End();
}

void GameBaseImpl::OnSplashScreenEnd()
{
    // Hide splash screen
    SplashScreenTime = 0;
    SplashScreen = nullptr;
    MainRenderTask::Instance->PostRender.Unbind(&OnPostRender);

    // Load the first scene
    LOG(Info, "Loading the first scene");
    const auto sceneId = FirstScene ? FirstScene.GetID() : Guid::Empty;
    FirstScene = nullptr;
    if (Level::LoadSceneAsync(sceneId))
    {
        LOG(Fatal, "Cannot load the first scene.");
        return;
    }
}

#endif
