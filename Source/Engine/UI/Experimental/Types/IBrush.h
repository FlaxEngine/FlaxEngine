#pragma once
#include "Engine/Scripting/ScriptingObject.h"
#include "UIElement.h"
#include "Engine/Content/AssetReference.h"
#include "Engine/Content/Assets/Texture.h"
#include "Engine/Core/Math/Color.h"

API_INTERFACE() class FLAXENGINE_API IBrush
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(IBrush);
protected:
    UIElement* Owner;
public:
    /// <summary>
    /// Caled when IBrush is Cunstructed
    /// Called in editor and game
    /// Warning 
    /// The PreCunstruct can run before any game/editor related states are ready
    /// use only for IBrush
    /// </summary>
    API_FUNCTION() virtual void OnPreCunstruct(bool isInDesigner) {};

    /// <summary>
    /// Caled when IBrush is created
    /// can be called mupltiple times
    /// </summary>
    API_FUNCTION() virtual void OnCunstruct() {};

    /// <summary>
    /// Draws the IBrush
    /// </summary>
    API_FUNCTION() virtual void OnDraw(const Float2& At);

    /// <summary>
    /// Called when IBrush is destroyed
    /// can be called mupltiple times
    /// </summary>
    API_FUNCTION() virtual void OnDestruct() {};

    /// <summary>
    /// Gets desired size for this element
    /// </summary>
    API_FUNCTION() virtual Float2 GetDesiredSize() { return Owner->GetSlot()->GetSize(); };
};

API_CLASS() class FLAXENGINE_API ImageBrush : public ScriptingObject , public IBrush
{
    DECLARE_SCRIPTING_TYPE(ImageBrush);
public:
    API_FIELD()
        AssetReference<Texture> Image;
    API_FIELD()
        Int2 ImageSize;
    API_FIELD()
        Color Tint = Color::White;

    // Inherited via IBrush

    API_FUNCTION() virtual void OnPreCunstruct(bool isInDesigner) override 
    {
        if (Image && !Image->WaitForLoaded())
        {
            OnImageAssetChanged();
        }
    };

    API_FUNCTION() virtual void OnCunstruct() override {Image.Changed.Bind(OnImageAssetChanged);};

    API_FUNCTION() virtual void OnDraw(const Float2& At) override;

    API_FUNCTION() virtual void OnDestruct() override {Image.Changed.Unbind(OnImageAssetChanged);};

    API_FUNCTION() virtual Float2 GetDesiredSize() override {return ImageSize;};

private:
    void OnImageAssetChanged() 
    {
        ImageSize = Image.Get()->Size();
    }
};
