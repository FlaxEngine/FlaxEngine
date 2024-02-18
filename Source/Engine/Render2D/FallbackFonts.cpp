#include "FallbackFonts.h"
#include "FontManager.h"
#include "Engine/Core/Math/Math.h"

FontFallbackList::FontFallbackList(const Array<FontAsset*>& fonts)
    : ManagedScriptingObject(SpawnParams(Guid::New(), Font::TypeInitializer)),
    _fontAssets(fonts)
{

}

