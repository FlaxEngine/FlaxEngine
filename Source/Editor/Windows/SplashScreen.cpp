// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "SplashScreen.h"
#include "Engine/Core/Log.h"
#include "Engine/Engine/CommandLine.h"
#include "Engine/Render2D/FontAsset.h"
#include "Engine/Render2D/Font.h"
#include "Engine/Render2D/TextLayoutOptions.h"
#include "Engine/Render2D/Render2D.h"
#include "Engine/Content/Content.h"
#include "FlaxEngine.Gen.h"

// Randomly picked, limited to 50 characters width and 2 lines
const Char* SplashScreenQuotes[] =
{
    TEXT("Loading"),
    TEXT("Unloading"),
    TEXT("Reloading"),
    TEXT("Reloading gun"),
    TEXT("Consuming your RAM"),
    TEXT("Burning your CPU"),
    TEXT("#BetterThanUnity"),
    TEXT("Rendering buttons"),
    TEXT("Collecting crash data"),
    TEXT("Downloading porn"),
#if PLATFORM_WINDOWS
    TEXT("Removing 'C:\\Windows\\'"),
#elif PLATFORM_LINUX
    TEXT("Time to switch to Windows?"),
    TEXT("Installing Windows 10..."),
#endif
    TEXT("Kappa!"),
    TEXT("How you doin'?"),
    TEXT("Why so serious?"),
    TEXT("Bond. James Bond."),
    TEXT("To infinity and beyond!"),
    TEXT("Houston, we have a problem"),
    TEXT("NotImplementedEngineException"),
    TEXT("Made in Poland"),
    TEXT("We like you"),
    TEXT("Compiling the compiler"),
    TEXT("Flax it up!"),
    TEXT("Fun fact: Fortnite runs on Flax"),
    TEXT("Toss a coin to your Witcher!!!"),
    TEXT("Holy Moly!"),
    TEXT("Just Read the Instructions"),
    TEXT("Preparing for a team fight"),
    TEXT("Habemus Flaximus"),
    TEXT("Recruiting robot hamsters"),
    TEXT("This text has 23 characters"),
    TEXT("May the Loading be with you"),
    TEXT("The Eagle has landed"),
    TEXT("Supermassive Black Hole"),
    TEXT("Kept you loading, huh?"),
    TEXT("They see me loadin'"),
    TEXT("Loadin' loadin' and loadin' loadin'"),
    TEXT("Procedurally generating buttons"),
    TEXT("Out of Memory Exception!"),
    TEXT("Running Big Bang simulation"),
    TEXT("Calculating infinity"),
    TEXT("Dividing infinity by zero"),
    TEXT("I guess you guys aren't ready for that yet.\nBut your kids are gonna love it"),
    TEXT("Calculating the amount of atoms in the universe"),
    TEXT("Everything you can imagine is real.\n~Pablo Picasso"),
    TEXT("Whatever you do, do it well.\n~Walt Disney"),
    TEXT("Here's Johnny!"),
    TEXT("Did you see that? No... I don't think so"),
    TEXT("Collecting unreal power"),
    TEXT("Stay safe, friend"),
    TEXT("trolololololololololololo"),
    TEXT("xD"),
    TEXT("Come to the dark side"),
    TEXT("Flax Facts: This is a loading screen"),
    TEXT("Don't Stop Me Now"),
    TEXT("Pizza! We like pizza!"),
    TEXT("Made with Flax"),
    TEXT("This is the way"),
    TEXT("The quick brown fox jumps over the lazy dog"),
    TEXT("Hit The Road Jack"),
    TEXT("You have 7 lives left"),
    TEXT("May the Force be with you"),
    TEXT("A martini. Shaken, not stirred"),
    TEXT("Hasta la vista, baby"),
    TEXT("Winter is coming"),
    TEXT("You know nothing, Jon Snow"),
    TEXT("Create something awesome!"),
    TEXT("Well Polished Engine"),
    TEXT("Error 404: Joke Not Found"),
    TEXT("Rushing B"),
    TEXT("Putting pineapple on pizza"),
    TEXT("Loading Simulation"),
    TEXT("Get ready for a surprise!"),
    TEXT("Coffee is my fuel"),
    TEXT("Installing a free copy of Cyberpunk 2077"),
    TEXT("With great power comes great electricity bill"),
    TEXT("Flax was made in the same city as Witcher 3"),
    TEXT("So JavaScript is a scripting version of Java"),
    TEXT("Good things take time.\n~Someone"),
    TEXT("Get shit done"),
    TEXT("Hold Tight! Loading Flax"),
    TEXT("That's one small step for a man,\none giant leap for mankind"),
    TEXT("Remember to save your work frequently"),
    TEXT("In case of fire:\ngit commit, git push, leave building"),
    TEXT("Keep calm and make games"),
    TEXT("You're breathtaking!!!"),
    TEXT("Do regular dogs see police dogs & think,\nOh no it's a cop?"),
    TEXT("Dear Santa,\nDefine naughty."),
    TEXT("Blah, blah"),
    TEXT("My PRECIOUS!!!!"),
    TEXT("YOU SHALL NOT PASS!"),
    TEXT("You have my bow.\nAnd my axe!"),
    TEXT("To the bridge of Khazad-dum."),
    TEXT("One ring to rule them all.\nOne ring to find them."),
    TEXT("Ladies and gentelman, we got him"),
    TEXT("Cyberpunk of game engines"),
    TEXT("That's what she said"),
    TEXT("Compiling Shaders (93,788)"),
    TEXT("Hi There"),
    TEXT("BAGUETTE"),
    TEXT("All we had to do was follow the damn train, CJ"),
    TEXT("28 stab wounds"),
    TEXT("Here we go again"),
};

SplashScreen::~SplashScreen()
{
    // Ensure to be closed
    Close();
}

void SplashScreen::Show()
{
    // Skip if already shown or in headless mode
    if (IsVisible() || CommandLine::Options.Headless)
        return;

    LOG(Info, "Showing splash screen");

    // Create window
    const float dpiScale = (float)Platform::GetDpi() / (float)DefaultDPI;
    CreateWindowSettings settings;
    settings.Title = TEXT("Flax Editor");
    settings.Size.X = 500 * dpiScale;
    settings.Size.Y = 170 * dpiScale;
    settings.HasBorder = false;
    settings.AllowInput = true;
    settings.AllowMinimize = false;
    settings.AllowMaximize = false;
    settings.AllowDragAndDrop = false;
    settings.IsTopmost = false;
    settings.IsRegularWindow = false;
    settings.HasSizingFrame = false;
    settings.ShowAfterFirstPaint = true;
    settings.StartPosition = WindowStartPosition::CenterScreen;
    _window = Platform::CreateWindow(settings);

    // Register window events
    _window->Closing.Bind([](ClosingReason reason, bool& cancel)
    {
        // Disable closing by user
        if (reason == ClosingReason::User)
            cancel = true;
    });
    _window->HitTest.Bind([](const Vector2& mouse, WindowHitCodes& hit, bool& handled)
    {
        // Allow to drag window by clicking anywhere
        hit = WindowHitCodes::Caption;
        handled = true;
    });
    _window->Shown.Bind<SplashScreen, &SplashScreen::OnShown>(this);
    _window->Draw.Bind<SplashScreen, &SplashScreen::OnDraw>(this);

    // Setup
    _dpiScale = dpiScale;
    _width = settings.Size.X;
    _height = settings.Size.Y;
    _startTime = DateTime::NowUTC();
    auto str = Globals::ProjectFolder;
#if PLATFORM_WIN32
    str.Replace('/', '\\');
#else
    str.Replace('\\', '/');
#endif
    _infoText = String::Format(TEXT("Flax Editor {0}\n{1}\nProject: {2}"), TEXT(FLAXENGINE_VERSION_TEXT), TEXT(FLAXENGINE_COPYRIGHT), str);
    _quote = SplashScreenQuotes[rand() % ARRAY_COUNT(SplashScreenQuotes)];

    // Load font
    auto font = Content::LoadAsyncInternal<FontAsset>(TEXT("Editor/Fonts/Roboto-Regular"));
    if (font == nullptr)
    {
        LOG(Fatal, "Cannot load GUI primary font.");
    }
    else
    {
        if (font->IsLoaded())
            OnFontLoaded(font);
        else
            font->OnLoaded.Bind<SplashScreen, &SplashScreen::OnFontLoaded>(this);
    }

    _window->Show();
}

void SplashScreen::Close()
{
    if (!IsVisible())
        return;

    LOG(Info, "Closing splash screen");

    // Close window
    _window->Close(ClosingReason::CloseEvent);
    _window = nullptr;
}

void SplashScreen::OnShown()
{
    // Focus on shown
    _window->Focus();
    _window->BringToFront(false);
}

void SplashScreen::OnDraw()
{
    const float s = _dpiScale;
    const float width = _width;
    const float height = _height;

    // Peek time
    const float time = static_cast<float>((DateTime::NowUTC() - _startTime).GetTotalSeconds());

    // Background
    const float lightBarHeight = 112 * s;
    Render2D::FillRectangle(Rectangle(0, 0, width, 150 * s), Color::FromRGB(0x1C1C1C));
    Render2D::FillRectangle(Rectangle(0, lightBarHeight, width, height), Color::FromRGB(0x0C0C0C));

    // Animated border
    const float anim = Math::Sin(time * 4.0f) * 0.5f + 0.5f;
    Render2D::DrawRectangle(Rectangle(0, 0, width, height), Math::Lerp(Color::Gray * 0.8f, Color::FromRGB(0x007ACC), anim));

    // Check fonts
    if (!HasLoadedFonts())
        return;

    // Title
    const Vector2 titleLength = _titleFont->MeasureText(GetTitle());
    TextLayoutOptions layout;
    layout.Bounds = Rectangle(10 * s, 10 * s, width - 10 * s, 50 * s);
    layout.HorizontalAlignment = TextAlignment::Near;
    layout.VerticalAlignment = TextAlignment::Near;
    layout.Scale = Math::Min((width - 20 * s) / titleLength.X, 1.0f);
    Render2D::DrawText(_titleFont, GetTitle(), Color::White, layout);

    // Subtitle
    String subtitle(_quote);
    if (!subtitle.EndsWith(TEXT('!')) && !subtitle.EndsWith(TEXT('?')))
    {
        for (int32 i = static_cast<int32>(time * 2.0f) % 4; i > 0; i--)
            subtitle += TEXT('.');
        for (int32 i = 0; i < 4 - static_cast<int32>(time * 2.0f) % 4; i++)
            subtitle += TEXT(' ');
    }
    layout.Bounds = Rectangle(width - 224 * s, lightBarHeight - 39 * s, 220 * s, 35 * s);
    layout.Scale = 1.0f;
    layout.HorizontalAlignment = TextAlignment::Far;
    layout.VerticalAlignment = TextAlignment::Far;
    Render2D::DrawText(_subtitleFont, subtitle, Color::FromRGB(0x8C8C8C), layout);

    // Additional info
    const float infoMargin = 6 * s;
    layout.Bounds = Rectangle(infoMargin, lightBarHeight + infoMargin, width - (2 * infoMargin), height - lightBarHeight - (2 * infoMargin));
    layout.HorizontalAlignment = TextAlignment::Near;
    layout.VerticalAlignment = TextAlignment::Center;
    Render2D::DrawText(_subtitleFont, _infoText, Color::FromRGB(0xFFFFFF) * 0.9f, layout);
}

bool SplashScreen::HasLoadedFonts() const
{
    return _titleFont && _subtitleFont;
}

void SplashScreen::OnFontLoaded(Asset* asset)
{
    ASSERT(asset && asset->IsLoaded());
    auto font = (FontAsset*)asset;

    font->OnLoaded.Unbind<SplashScreen, &SplashScreen::OnFontLoaded>(this);

    // Create fonts
    const float s = _dpiScale;
    _titleFont = font->CreateFont((uint32)(35 * s));
    _subtitleFont = font->CreateFont((uint32)(9 * s));
}
