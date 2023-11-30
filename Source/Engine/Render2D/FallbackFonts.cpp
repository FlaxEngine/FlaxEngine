#include "FallbackFonts.h"
#include "FontManager.h"
#include "Engine/Core/Math/Math.h"

FallbackFonts::FallbackFonts(const Array<FontAsset*>& fonts)
    : ManagedScriptingObject(SpawnParams(Guid::New(), Font::TypeInitializer)),
    _fontAssets(fonts)
{

}

