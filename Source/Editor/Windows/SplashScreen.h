// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Content/Assets/Texture.h"
#include "Engine/Content/AssetReference.h"
#include "Engine/Core/Types/DateTime.h"
#include "Engine/Platform/Window.h"

class Asset;
class Font;

/// <summary>
/// Splash Screen popup
/// </summary>
class SplashScreen
{
private:

    Window* _window = nullptr;
    Font* _titleFont = nullptr;
    Font* _subtitleFont = nullptr;
    AssetReference<Texture> _splashTexture;
    String _title;
    DateTime _startTime;
    String _infoText;
    float _dpiScale;
    Float2 _size;
    StringView _quote;

public:

    /// <summary>
    /// Finalizes an instance of the <see cref="SplashScreen"/> class.
    /// </summary>
    ~SplashScreen();

public:

    /// <summary>
    /// Sets the title text.
    /// </summary>
    /// <param name="title">The title text.</param>
    void SetTitle(const String& title)
    {
        _title = title;
    }

    /// <summary>
    /// Gets the title text.
    /// </summary>
    /// <returns>Title text.</returns>
    FORCE_INLINE const String& GetTitle() const
    {
        return _title;
    }

    /// <summary>
    /// Determines whether this popup is visible.
    /// </summary>
    /// <returns>True if this popup is visible, otherwise false</returns>
    FORCE_INLINE bool IsVisible() const
    {
        return _window != nullptr;
    }

public:

    /// <summary>
    /// Shows popup.
    /// </summary>
    void Show();

    /// <summary>
    /// Closes popup.
    /// </summary>
    void Close();

private:

    void OnShown();
    void OnDraw();
    bool HasLoadedFonts() const;
    void OnFontLoaded(Asset* asset);
    void OnSplashLoaded();
};
